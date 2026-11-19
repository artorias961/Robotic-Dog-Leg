#pragma once
// The green boxes from the leg diagram — the ONLY places data crosses
// threads. No mutexes anywhere on the hot path.
//
//   setpoint slot   seqlock, latest wins        RX  -> control
//   command queue   k_msgq, depth 8             RX  -> control
//   state snapshot  double buffer               control -> TX
//   health flag     atomic, set every tick      control -> supervisor
//   loss/CRC counters, t_hBeat, t_setpoint      atomics, RX writes
#include <zephyr/kernel.h>

#include <atomic>
#include <cstdint>

namespace leg {

// ---- setpoint slot (seqlock — latest wins) ---------------------------------
struct Setpoints {
  float target_rad[3]{};
};

class SetpointSlot {
 public:
  void store(const Setpoints& s) {
    const std::uint32_t v = seq_.load(std::memory_order_relaxed);
    seq_.store(v + 1, std::memory_order_release);  // odd: write in progress
    value_ = s;
    seq_.store(v + 2, std::memory_order_release);  // even: stable
  }
  Setpoints load() const {
    Setpoints out;
    std::uint32_t a, b;
    do {
      a = seq_.load(std::memory_order_acquire);
      out = value_;
      b = seq_.load(std::memory_order_acquire);
    } while (a != b || (a & 1u));
    return out;
  }

 private:
  std::atomic<std::uint32_t> seq_{0};
  Setpoints value_{};
};

// ---- command queue (k_msgq is defined in shared.cpp, depth 8) --------------
struct CmdMsg {
  std::uint8_t  cmd;    // proto::Cmd
  float         param;
  std::uint32_t seq;    // echoed in the ACK
};
extern k_msgq cmd_queue;

// ---- state snapshot (double buffer: control writes, TX reads) --------------
struct Snapshot {
  float pos_rad[3]{};
  float current_a[3]{};
  std::uint16_t fault_flags{};
  std::uint8_t  armed{};
  std::uint8_t  state{};   // LegState below
};

class SnapshotBuffer {
 public:
  void publish(const Snapshot& s) {
    const int next = 1 - active_.load(std::memory_order_relaxed);
    buf_[next] = s;
    active_.store(next, std::memory_order_release);
  }
  Snapshot read() const {
    return buf_[active_.load(std::memory_order_acquire)];
  }

 private:
  Snapshot buf_[2]{};
  std::atomic<int> active_{0};
};

// ---- timestamps ------------------------------------------------------------
// Milliseconds from k_uptime_get_32(). 32-bit on purpose: the Cortex-M7 has no
// 64-bit atomics, so std::atomic<int64_t> fails to link (__atomic_load_8).
// Unsigned subtraction keeps ages right across the 49.7-day wraparound; always
// go through age_ms() so the math stays 32-bit unsigned.
constexpr std::uint32_t age_ms(std::uint32_t now, std::uint32_t then) {
  return now - then;
}
// "Never received": starts about 24 days in the past, so nothing is fresh
// until the first packet arrives.
inline constexpr std::uint32_t kNeverMs = 0x8000'0000u;

// ---- atomics shared across threads -----------------------------------------
struct SharedState {
  SetpointSlot   setpoints;
  SnapshotBuffer snapshot;

  std::atomic<std::uint32_t> t_setpoint_ms{kNeverMs};  // last valid setpoint
  std::atomic<std::uint32_t> t_hbeat_ms{kNeverMs};     // last heartbeat (t_hBeat)
  std::atomic<bool>         armed{false};
  std::atomic<bool>         health_flag{false};         // control sets, supervisor clears
  std::atomic<std::uint16_t> fault_flags{0};            // supervisor-latched faults

  // link stats: RX writes, TX reports, supervisor watches the trend
  std::atomic<std::uint16_t> lost_packets{0};
  std::atomic<std::uint16_t> crc_errors{0};
  std::atomic<std::uint32_t> rx_ok{0};

  std::atomic<std::int32_t>  supervisor_period_ms{0};   // 0 -> use default
};
extern SharedState g;

// Startup gate: main() gives this 4 times once HAL + watchdog are up.
extern k_sem init_done;

// Fault bits (telemetry fault_flags)
inline constexpr std::uint16_t kFaultOverCurrent = 1u << 0;
inline constexpr std::uint16_t kFaultRange       = 1u << 1;
inline constexpr std::uint16_t kFaultCommsLost   = 1u << 2;
inline constexpr std::uint16_t kFaultI2t         = 1u << 3;
inline constexpr std::uint16_t kFaultTracking    = 1u << 4;
inline constexpr std::uint16_t kFaultStack       = 1u << 5;

enum class LegState : std::uint8_t { Disarmed = 0, Armed = 1, Failsafe = 2 };

}  // namespace leg
