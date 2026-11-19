// Diagram: "Net RX thread (prio mid)".
//   recvfrom(:5005) -> magic+CRC ok? -> seq > last_seq? -> count gaps ->
//   dispatch: setpoints -> slot, commands -> k_msgq (+ACK), heartbeat -> t_hBeat
//
// Two rules keep the sequence check from deafening the leg:
//   * Restart-aware: if the master's uptime (header t_ms) jumps backwards,
//     the master restarted and sequence tracking starts over.
//   * Commands are never dropped as "old": a retransmitted command carries
//     its original sequence number, which newer setpoints may already have
//     overtaken. Commands are de-duplicated by their own sequence instead.
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/socket.h>

#include <cstdint>
#include <span>

#include "config.hpp"
#include "net.hpp"
#include "protocol.hpp"
#include "shared.hpp"

LOG_MODULE_REGISTER(rx, LOG_LEVEL_INF);

namespace leg {
namespace {

std::uint32_t last_seq = 0;        // newest master packet seen this session
std::uint32_t last_cmd_seq = 0;    // newest command already queued
std::uint32_t last_master_t = 0;   // newest master uptime seen (ms)
bool          have_master_t = false;
std::uint32_t ack_seq = 0;

void send_ack(int tx_sock, std::uint32_t acked, std::uint32_t t_ms) {
  proto::AckPacket p{
      .header = {.magic = proto::kMagic,
                 .version = proto::kVersion,
                 .type = static_cast<std::uint8_t>(proto::Type::Ack),
                 .leg_id = cfg::kLegId,
                 .flags = 0,
                 .seq = ++ack_seq,
                 .t_ms = t_ms},
      .acked_seq = acked,
      .crc = 0};
  proto::seal(p);
  const sockaddr_in master = net::master_endpoint();
  zsock_sendto(tx_sock, &p, sizeof(p), 0,
               reinterpret_cast<const sockaddr*>(&master), sizeof(master));
}

// Every header carries the sender's uptime. A backwards jump larger than
// kRestartSlackMs means a new master session: forget the old sequence
// numbers instead of rejecting the new session for as long as the old ran.
void track_master_session(const proto::Header& h) {
  if (have_master_t && h.t_ms + cfg::kRestartSlackMs < last_master_t) {
    LOG_WRN("master restart detected (uptime %u -> %u ms): sequence tracking reset",
            last_master_t, h.t_ms);
    last_seq = 0;
    last_cmd_seq = 0;
    last_master_t = h.t_ms;
  } else if (!have_master_t || h.t_ms > last_master_t) {
    last_master_t = h.t_ms;
  }
  have_master_t = true;
}

// Records seq if it is the newest seen, counting any gap as lost packets.
// Returns false for duplicates and reordered (older) packets.
bool record_seq(std::uint32_t seq) {
  if (last_seq != 0 && seq <= last_seq) return false;
  if (last_seq != 0 && seq > last_seq + 1)
    g.lost_packets.fetch_add(static_cast<std::uint16_t>(seq - last_seq - 1));
  last_seq = seq;
  return true;
}

void dispatch(std::span<const std::uint8_t> raw, int tx_sock) {
  const std::uint32_t now_ms = k_uptime_get_32();

  // Streams (setpoints, heartbeats): newest wins, older packets are stale.
  if (auto p = proto::parse<proto::SetpointPacket>(raw, proto::Type::Setpoint)) {
    if (p->header.leg_id != cfg::kLegId) return;
    track_master_session(p->header);
    if (!record_seq(p->header.seq)) return;
    g.setpoints.store(Setpoints{{p->target_rad[0], p->target_rad[1], p->target_rad[2]}});
    g.t_setpoint_ms.store(now_ms);
    g.rx_ok.fetch_add(1);
    return;
  }
  if (auto p = proto::parse<proto::HeartbeatPacket>(raw, proto::Type::Heartbeat)) {
    if (p->header.leg_id != cfg::kLegId) return;
    track_master_session(p->header);
    if (!record_seq(p->header.seq)) return;
    g.t_hbeat_ms.store(now_ms);   // t_hBeat: master is alive (any FSM state)
    g.rx_ok.fetch_add(1);
    return;
  }

  // Commands: never rejected as old; de-duplicated by command sequence.
  if (auto p = proto::parse<proto::CommandPacket>(raw, proto::Type::Command)) {
    if (p->header.leg_id != cfg::kLegId) return;
    track_master_session(p->header);
    const std::uint32_t seq = p->header.seq;
    record_seq(seq);  // loss statistics only
    if (seq <= last_cmd_seq) {          // retransmit of a command already queued
      send_ack(tx_sock, seq, now_ms);   // our earlier ACK was lost: re-ACK
      return;
    }
    CmdMsg msg{.cmd = p->cmd, .param = p->param, .seq = seq};
    if (k_msgq_put(&cmd_queue, &msg, K_NO_WAIT) != 0) {
      LOG_WRN("command queue full: cmd %u not ACKed, master will retry", msg.cmd);
      return;
    }
    last_cmd_seq = seq;
    send_ack(tx_sock, seq, now_ms);     // ACK only once the command is queued
    g.rx_ok.fetch_add(1);
    return;
  }
  g.crc_errors.fetch_add(1);  // wrong magic/CRC/size: drop, count
}

void rx_main(void*, void*, void*) {
  k_sem_take(&init_done, K_FOREVER);

  const int rx = net::make_rx_socket();
  const int tx = net::make_tx_socket();
  if (rx < 0 || tx < 0) {
    LOG_ERR("socket setup failed (rx=%d tx=%d)", rx, tx);
    return;
  }
  LOG_INF("listening on :%u as leg %u", cfg::kPortRx, cfg::kLegId);

  std::uint8_t buf[128];
  for (;;) {
    sockaddr_in from{};
    socklen_t from_len = sizeof(from);
    const auto n = zsock_recvfrom(rx, buf, sizeof(buf), 0,
                                  reinterpret_cast<sockaddr*>(&from), &from_len);
    if (n <= 0) continue;
    if (!net::from_master(from)) continue;  // only the master may command us
    dispatch({buf, static_cast<std::size_t>(n)}, tx);
  }
}

}  // namespace

// Outside the anonymous namespace so shared.hpp's rx_tid names this thread.
K_THREAD_DEFINE(rx_tid, 4096, rx_main, NULL, NULL, NULL, 5, 0, 0);

}  // namespace leg
