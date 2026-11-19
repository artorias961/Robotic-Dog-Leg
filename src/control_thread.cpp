// Diagram: "Control thread (1 kHz, prio high)".
//   k_sem_take(tick) <- k_timer ISR @ 1 kHz
//   read sensors -> copy setpoints + drain commands -> comms fresh?
//   (setpoint < 50 ms, heartbeat < 500 ms) -> failsafe/disarm
//   per-tick checks (armed? I > limit? pos in range?) -> targets -> FlexPWM
//   publish snapshot + set health flag -> loop
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <algorithm>

#include "config.hpp"
#include "hal.hpp"
#include "protocol.hpp"
#include "shared.hpp"

LOG_MODULE_REGISTER(control, LOG_LEVEL_INF);

namespace leg {
namespace {

K_SEM_DEFINE(tick_sem, 0, 1);

void tick_isr(k_timer*) { k_sem_give(&tick_sem); }  // ISR: capture + signal only
K_TIMER_DEFINE(tick_timer, tick_isr, nullptr);

// Returns true when an ENABLE armed the leg this tick, so the caller can
// re-seed targets from the measured pose (motionless arming).
bool drain_commands() {
  bool just_armed = false;
  CmdMsg msg;
  while (k_msgq_get(&cmd_queue, &msg, K_NO_WAIT) == 0) {
    switch (static_cast<proto::Cmd>(msg.cmd)) {
      case proto::Cmd::Enable:
        // Faults clear only through an operator-acknowledged CLEAR_FAULTS,
        // never as a side effect of arming.
        if (const auto f = g.fault_flags.load(); f != 0) {
          LOG_WRN("ENABLE refused: latched faults 0x%04x (cmd seq %u)", f, msg.seq);
          break;
        }
        if (!g.armed.exchange(true)) just_armed = true;
        LOG_INF("ENABLE (cmd seq %u)", msg.seq);
        break;
      case proto::Cmd::Disable:
        g.armed.store(false);
        LOG_INF("DISABLE (cmd seq %u)", msg.seq);
        break;
      case proto::Cmd::ClearFaults:
        if (g.armed.load()) {
          LOG_WRN("CLEAR_FAULTS ignored while armed (cmd seq %u)", msg.seq);
          break;
        }
        g.fault_flags.store(0);  // a still-present cause re-latches on its next check
        LOG_INF("faults cleared by operator (cmd seq %u)", msg.seq);
        break;
    }
  }
  return just_armed;
}

void control_main(void*, void*, void*) {
  k_sem_take(&init_done, K_FOREVER);
  k_timer_start(&tick_timer, K_USEC(cfg::kControlPeriodUs),
                K_USEC(cfg::kControlPeriodUs));
  LOG_INF("control loop at %d us", cfg::kControlPeriodUs);

  float targets[3] = {0.f, 0.f, 0.f};  // last good targets (hold on brief gaps)

  for (;;) {
    k_sem_take(&tick_sem, K_FOREVER);
    const std::uint32_t now_ms = k_uptime_get_32();

    // 1. sensors: 3x position, 3x current
    const hal::Sensors s = hal::read_sensors();

    // 2. latest setpoints + any pending commands
    const Setpoints sp = g.setpoints.load();
    const bool just_armed = drain_commands();

    // Motionless arming: the instant ENABLE lands, seed targets from the
    // MEASURED joint positions so the servos hold where the leg already is
    // instead of snapping to the zero pose. The master's stand-up ramp also
    // starts from the reported pose, so the first setpoints match this.
    if (just_armed && s.valid) {
      targets[0] = s.pos_rad[0];
      targets[1] = s.pos_rad[1];
      targets[2] = s.pos_rad[2];
    }

    // 3. comms fresh?  setpoint < 50 ms (hold), heartbeat < 500 ms (disarm)
    const bool sp_fresh =
        age_ms(now_ms, g.t_setpoint_ms.load()) < cfg::kSetpointTimeoutMs;
    const bool hb_fresh =
        age_ms(now_ms, g.t_hbeat_ms.load()) < cfg::kHeartbeatTimeoutMs;

    bool armed = g.armed.load();
    LegState state = armed ? LegState::Armed : LegState::Disarmed;

    if (armed && !hb_fresh) {           // master silent: failsafe / disarm
      g.armed.store(false);
      armed = false;
      state = LegState::Failsafe;
      g.fault_flags.fetch_or(kFaultCommsLost);
      LOG_WRN("heartbeat lost -> disarm");
    } else if (armed && sp_fresh) {
      targets[0] = sp.target_rad[0];
      targets[1] = sp.target_rad[1];
      targets[2] = sp.target_rad[2];
    }
    // armed && !sp_fresh: hold last targets (brief gap costs one stale tick)

    // 4. per-tick checks: cheap compares only — the expensive ones live in
    //    the supervisor. Range clamps the target; overcurrent latches fault.
    std::uint16_t faults = 0;
    for (int i = 0; i < 3; ++i) {
      const auto& c = cfg::kJoint[i];
      if (targets[i] < c.min_rad || targets[i] > c.max_rad) {
        targets[i] = std::clamp(targets[i], c.min_rad, c.max_rad);
        faults |= kFaultRange;
      }
      if (s.valid && s.current_a[i] > c.current_limit_a)
        faults |= kFaultOverCurrent;
    }
    if (faults & kFaultOverCurrent) {   // hard limit: cut now, stay cut
      g.armed.store(false);
      armed = false;
      g.fault_flags.fetch_or(faults);
    }

    // 5. outputs: servos now (direct), motors later (PID goes right here)
    if (armed) hal::write_servos(targets);
    else hal::disable_outputs();

    // 6. publish snapshot for TX, set health flag for the supervisor
    Snapshot snap{};
    for (int i = 0; i < 3; ++i) {
      snap.pos_rad[i] = s.pos_rad[i];
      snap.current_a[i] = s.current_a[i];
    }
    snap.fault_flags = g.fault_flags.load() | faults;
    snap.armed = armed ? 1 : 0;
    snap.state = static_cast<std::uint8_t>(state);
    g.snapshot.publish(snap);
    g.health_flag.store(true);          // supervisor feeds WDOG only on this
  }
}

K_THREAD_DEFINE(control_tid, 4096, control_main, NULL, NULL, NULL, 2, 0, 0);

}  // namespace
}  // namespace leg
