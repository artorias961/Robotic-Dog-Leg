// Diagram: "Supervisor (10–50 Hz, tunable)".
//   k_sleep(cfg.period) -> slow checks (I²t window, tracking error,
//   link loss trend, stack margins) -> on fault: flag telemetry, disarm ->
//   health flag fresh? -> feed WDOG / don't feed -> HW reset
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <algorithm>
#include <cmath>

#include "config.hpp"
#include "hal.hpp"
#include "shared.hpp"

LOG_MODULE_REGISTER(supervisor, LOG_LEVEL_INF);

namespace leg {
namespace {

void supervisor_main(void*, void*, void*) {
  k_sem_take(&init_done, K_FOREVER);
  LOG_INF("supervisor at %d ms (tunable)", cfg::kSupervisorDefaultMs);

  float i2t_acc[3] = {0.f, 0.f, 0.f};
  std::uint16_t last_lost = 0;
  std::int32_t tracking_err_ms = 0;
  [[maybe_unused]] int pass = 0;

  for (;;) {
    // Clamped to 10-50 Hz so no tuned value can starve the watchdog.
    const std::int32_t override_ms = g.supervisor_period_ms.load();
    const std::int32_t period_ms =
        std::clamp(override_ms > 0 ? override_ms : cfg::kSupervisorDefaultMs,
                   cfg::kSupervisorMinMs, cfg::kSupervisorMaxMs);
    k_sleep(K_MSEC(period_ms));
    const float dt = static_cast<float>(period_ms) / 1000.f;

    const Snapshot snap = g.snapshot.read();
    std::uint16_t faults = 0;

    // I²t window: a motor survives brief spikes but not sustained overload.
    for (int i = 0; i < 3; ++i) {
      const auto& c = cfg::kJoint[i];
      const float i2 = snap.current_a[i] * snap.current_a[i];
      const float rated2 = c.rated_a * c.rated_a;
      i2t_acc[i] = std::max(0.f, i2t_acc[i] + (i2 - rated2) * dt);
      if (i2t_acc[i] > c.i2t_limit) faults |= kFaultI2t;
    }

    // Tracking error must PERSIST (diagram: "tracking error persists?") —
    // one bad sample during a foot strike is normal; half a second of the
    // joint ignoring its target means jammed or broken.
    const Setpoints sp = g.setpoints.load();
    bool erring = false;
    if (snap.armed) {
      for (int i = 0; i < 3; ++i)
        if (std::abs(snap.pos_rad[i] - sp.target_rad[i]) > cfg::kTrackingErrRad)
          erring = true;
    }
    tracking_err_ms = erring ? tracking_err_ms + period_ms : 0;
    if (tracking_err_ms >= cfg::kTrackingPersistMs) faults |= kFaultTracking;

    // Link loss trend: on a healthy dedicated switch this stays zero, so a
    // rising counter is a bug/cable to investigate, not noise to tolerate.
    const std::uint16_t lost = g.lost_packets.load();
    if (lost != last_lost)
      LOG_WRN("packet loss rising: %u total", lost);
    last_lost = lost;

    // Stack margins (diagram: "stack margins"): about once a second, flag any
    // of OUR four threads with less than kStackMarginBytes of stack left.
    // Zephyr's own threads are left out on purpose: it sizes them tightly
    // (the idle thread has 320 bytes in total), so a fixed margin would fault
    // a healthy leg, and the MPU stack guard (CONFIG_MPU_STACK_GUARD) already
    // catches a real overflow in any thread. The scan runs with interrupts
    // enabled, so it cannot delay the 1 kHz control tick.
#ifdef CONFIG_INIT_STACKS
    if (++pass % cfg::kStackCheckEvery == 0) {
      static constexpr const char* kName[] = {"rx", "control", "tx", "supervisor"};
      const k_tid_t tids[] = {rx_tid, control_tid, tx_tid, supervisor_tid};
      for (int i = 0; i < 4; ++i) {
        std::size_t unused = 0;
        if (k_thread_stack_space_get(tids[i], &unused) == 0 &&
            unused < cfg::kStackMarginBytes) {
          faults |= kFaultStack;
          LOG_ERR("%s thread stack nearly full: %u bytes left", kName[i],
                  static_cast<unsigned>(unused));
        }
      }
    }
#endif

    if (faults) {
      g.fault_flags.fetch_or(faults);   // flagged in telemetry
      g.armed.store(false);             // derate/disable the leg
      LOG_ERR("supervisor fault 0x%04x -> disarm", faults);
    }

    // Deadman: feed the hardware watchdog ONLY if the control thread set its
    // health flag since our last pass. A hung control loop starves the WDOG
    // and the i.MX RT resets — a reset leg is safer than a frozen one.
    if (g.health_flag.exchange(false)) {
      hal::wdt_feed();
    } else {
      LOG_ERR("control thread unresponsive: letting WDOG starve");
      // no feed -> hardware reset within cfg::kWdtTimeoutMs
    }
  }
}

}  // namespace

// Outside the anonymous namespace so shared.hpp's supervisor_tid names this thread.
K_THREAD_DEFINE(supervisor_tid, 2048, supervisor_main, NULL, NULL, NULL, 9, 0, 0);

}  // namespace leg
