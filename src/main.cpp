// Per-leg firmware entry point. The four threads (rx, control, tx,
// supervisor) are defined in their own files via K_THREAD_DEFINE and wait on
// init_done; main() brings up the hardware, starts the watchdog, then
// releases them. Boot-safe by construction: PWM outputs stay disabled until
// the master's ENABLE command arrives over the acked channel.
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "config.hpp"
#include "hal.hpp"
#include "shared.hpp"

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

int main() {
  static constexpr const char* kLegName[4] = {"FR", "FL", "RR", "RL"};  // by LEG_ID
  LOG_INF("quadruped leg %s (id %u, ip %s) — Zephyr, 1 kHz control",
          kLegName[leg::cfg::kLegId], leg::cfg::kLegId, leg::cfg::kMyIp);

  leg::hal::disable_outputs();  // never twitch at boot

  if (!leg::hal::init())
    LOG_WRN("HAL degraded (see overlay warnings) — continuing for bench use");

  if (!leg::hal::wdt_start())
    LOG_ERR("hardware watchdog unavailable");

  // Release rx, control, tx, supervisor.
  for (int i = 0; i < 4; ++i) k_sem_give(&leg::init_done);

  LOG_INF("threads released; waiting for master heartbeat + ENABLE");
  return 0;
}
