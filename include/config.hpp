#pragma once
// Per-leg firmware tunables. Values mirror Conventions.md — change there first.
// LEG_ID and this leg's IP come from the build (scripts/02_build, using
// scripts/legs.csv): 0 = Front Right, 1 = Front Left, 2 = Rear Right,
// 3 = Rear Left.
#include <cstddef>
#include <cstdint>

namespace leg::cfg {

#ifndef LEG_ID
#define LEG_ID 0
#endif
inline constexpr std::uint8_t kLegId = LEG_ID;

// ---- network (static IPs; this leg's own IP is a Kconfig value) ------------
inline constexpr const char* kMasterIp   = "192.168.1.1";
inline constexpr std::uint16_t kPortRx   = 5005;  // we listen: setpoints, commands, heartbeat
inline constexpr std::uint16_t kPortTx   = 5006;  // master listens: telemetry, acks

#ifdef CONFIG_NET_CONFIG_MY_IPV4_ADDR
inline constexpr const char* kMyIp = CONFIG_NET_CONFIG_MY_IPV4_ADDR;

// Build sanity: leg N must use 192.168.1.(11 + N), on the master's subnet
// (Conventions.md). The build scripts pass LEG_ID and the IP together; if the
// two ever disagree, the build fails here instead of producing a leg that
// answers to the wrong address.
namespace detail {
constexpr int last_octet(const char* ip) {
  int v = 0;
  for (; *ip; ++ip) v = (*ip == '.') ? 0 : v * 10 + (*ip - '0');
  return v;
}
constexpr bool same_subnet24(const char* a, const char* b) {
  for (int dots = 0; *a && *b && *a == *b; ++a, ++b)
    if (*a == '.' && ++dots == 3) return true;
  return false;
}
}  // namespace detail
static_assert(detail::last_octet(kMyIp) == 11 + LEG_ID,
              "LEG_ID and IP disagree: leg N must use 192.168.1.(11 + N)");
static_assert(detail::same_subnet24(kMyIp, kMasterIp),
              "this leg's IP is not on the master's 192.168.1.x subnet");
#else
inline constexpr const char* kMyIp = "(not set)";
#endif

// ---- timing (diagram: control 1 kHz, TX ~500 Hz, supervisor tunable) -------
inline constexpr int kControlPeriodUs    = 1000;  // 1 kHz control tick
inline constexpr int kTelemetryPeriodMs  = 2;     // ~500 Hz
inline constexpr int kSupervisorDefaultMs = 50;   // tunable via command later
inline constexpr int kSupervisorMinMs     = 20;   // 50 Hz
inline constexpr int kSupervisorMaxMs     = 100;  // 10 Hz; must stay well under kWdtTimeoutMs

// ---- comms freshness (diagram: "comms fresh?") ------------------------------
inline constexpr std::uint32_t kSetpointTimeoutMs  = 50;   // hold/failsafe
inline constexpr std::uint32_t kHeartbeatTimeoutMs = 500;  // disarm

// Sender uptime (header t_ms) going backwards by more than this means the
// master restarted: sequence tracking resets instead of rejecting it.
inline constexpr std::uint32_t kRestartSlackMs = 1000;

// ---- watchdog ---------------------------------------------------------------
// Zephyr's i.MX WDOG driver rejects timeouts under 500 ms (0.5 s steps).
// The supervisor feeds every 20-100 ms, so 1 s leaves a 10x margin.
inline constexpr std::uint32_t kWdtTimeoutMs = 1000;

// ---- supervisor slow-check thresholds ---------------------------------------
inline constexpr float        kTrackingErrRad    = 0.5f;  // |pos - target| limit
inline constexpr std::int32_t kTrackingPersistMs = 500;   // must persist this long
inline constexpr std::size_t  kStackMarginBytes  = 256;   // min free stack headroom
inline constexpr int          kStackCheckEvery   = 20;    // supervisor passes (~1 s)

// ---- joints: calibration placeholders until the frame is measured ----------
// Conventions.md: master-side robot_config.yaml is the real source; the master
// will push these at ENABLE (SET_PARAM). These defaults keep v1 standalone.
struct JointCalib {
  float raw_at_zero;     // ADC counts at joint zero
  float rad_per_count;   // sign folded in
  float min_rad, max_rad;
  float current_limit_a; // hard per-tick limit
  float i2t_limit;       // supervisor sustained (A^2 * s above rated)
  float rated_a;
};
inline constexpr JointCalib kJoint[3] = {
    // tilt                                           (placeholders)
    {2048.f, 0.001534f, -0.6f, 0.6f, 3.0f, 4.0f, 1.5f},
    // hip
    {2048.f, 0.001534f, -1.2f, 1.2f, 4.0f, 6.0f, 2.0f},
    // knee
    {2048.f, 0.001534f, 0.0f, 2.4f, 4.0f, 6.0f, 2.0f},
};

// Current sensor: amps = (raw - zero) * kAmpsPerCount   (placeholder)
inline constexpr float kCurrentZeroCounts = 2048.f;
inline constexpr float kAmpsPerCount      = 0.004f;

// Servo mapping (v1: hobby servos, direct PWM; motors+PID later).
inline constexpr std::uint32_t kServoPeriodNs  = 20'000'000;  // 50 Hz frame
inline constexpr std::uint32_t kServoCenterNs  = 1'500'000;
inline constexpr float kServoNsPerRad = 1'000'000.0f / 1.5708f;  // ±90° -> ±1000 µs

}  // namespace leg::cfg
