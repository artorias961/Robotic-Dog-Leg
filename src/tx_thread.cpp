// Diagram: "Telemetry TX thread (prio low)".
//   k_sleep(2 ms) -> copy snapshot + link stats -> build packet
//   (seq++ · timestamp · CRC16) -> sendto(master :5006) -> loop
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/socket.h>

#include <cstdint>

#include "config.hpp"
#include "net.hpp"
#include "protocol.hpp"
#include "shared.hpp"

LOG_MODULE_REGISTER(tx, LOG_LEVEL_INF);

namespace leg {
namespace {

void tx_main(void*, void*, void*) {
  k_sem_take(&init_done, K_FOREVER);

  const int sock = net::make_tx_socket();
  if (sock < 0) {
    LOG_ERR("tx socket failed");
    return;
  }
  const sockaddr_in master = net::master_endpoint();
  LOG_INF("telemetry -> %s:%u every %d ms", cfg::kMasterIp, cfg::kPortTx,
          cfg::kTelemetryPeriodMs);

  std::uint32_t seq = 0;
  for (;;) {
    const Snapshot snap = g.snapshot.read();

    proto::TelemetryPacket p{
        .header = {.magic = proto::kMagic,
                   .version = proto::kVersion,
                   .type = static_cast<std::uint8_t>(proto::Type::Telemetry),
                   .leg_id = cfg::kLegId,
                   .flags = 0,
                   .seq = ++seq,
                   .t_ms = k_uptime_get_32()},
        .pos_rad = {snap.pos_rad[0], snap.pos_rad[1], snap.pos_rad[2]},
        .current_a = {snap.current_a[0], snap.current_a[1], snap.current_a[2]},
        .fault_flags = snap.fault_flags,
        .lost_packets = g.lost_packets.load(),
        .crc_errors = g.crc_errors.load(),
        .armed = snap.armed,
        .state = snap.state,
        .crc = 0};
    proto::seal(p);

    zsock_sendto(sock, &p, sizeof(p), 0,
                 reinterpret_cast<const sockaddr*>(&master), sizeof(master));

    k_sleep(K_MSEC(cfg::kTelemetryPeriodMs));
  }
}

K_THREAD_DEFINE(tx_tid, 4096, tx_main, NULL, NULL, NULL, 7, 0, 0);

}  // namespace
}  // namespace leg
