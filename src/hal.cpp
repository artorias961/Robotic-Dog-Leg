// Devicetree-backed HAL. Channels/pins come from the zephyr,user node in
// app.overlay. Guarded so the code still compiles while the
// overlay is a template — stubbed paths log once and return safe values.
#include "hal.hpp"

#include <zephyr/device.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/watchdog.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <algorithm>

#include "config.hpp"

LOG_MODULE_REGISTER(hal, LOG_LEVEL_INF);

namespace leg::hal {
namespace {

#define USER_NODE DT_PATH(zephyr_user)

#if DT_NODE_HAS_PROP(USER_NODE, io_channels)
constexpr bool kHasAdc = true;
const adc_dt_spec adc_ch[6] = {
    ADC_DT_SPEC_GET_BY_IDX(USER_NODE, 0), ADC_DT_SPEC_GET_BY_IDX(USER_NODE, 1),
    ADC_DT_SPEC_GET_BY_IDX(USER_NODE, 2), ADC_DT_SPEC_GET_BY_IDX(USER_NODE, 3),
    ADC_DT_SPEC_GET_BY_IDX(USER_NODE, 4), ADC_DT_SPEC_GET_BY_IDX(USER_NODE, 5),
};
#else
constexpr bool kHasAdc = false;
#endif

#if DT_NODE_HAS_PROP(USER_NODE, pwms)
constexpr bool kHasPwm = true;
const pwm_dt_spec servo[3] = {
    PWM_DT_SPEC_GET_BY_IDX(USER_NODE, 0),
    PWM_DT_SPEC_GET_BY_IDX(USER_NODE, 1),
    PWM_DT_SPEC_GET_BY_IDX(USER_NODE, 2),
};
#else
constexpr bool kHasPwm = false;
#endif

// A node can exist but be disabled (Zephyr ships both i.MX WDOGs disabled),
// and DEVICE_DT_GET on a disabled node fails at link time — so require
// status "okay", which the overlay sets.
#define LEG_HAS_WDT DT_NODE_HAS_STATUS(DT_ALIAS(watchdog0), okay)

#if LEG_HAS_WDT
const device* wdt_dev = DEVICE_DT_GET(DT_ALIAS(watchdog0));
int wdt_channel = -1;
#endif

#if DT_NODE_HAS_PROP(USER_NODE, io_channels)
float counts_to_rad(int joint, float raw) {
  const auto& c = cfg::kJoint[joint];
  return (raw - c.raw_at_zero) * c.rad_per_count;
}
#endif

}  // namespace

bool init() {
  bool ok = true;
#if DT_NODE_HAS_PROP(USER_NODE, io_channels)
  for (const auto& ch : adc_ch) {
    if (!adc_is_ready_dt(&ch)) { LOG_ERR("ADC channel not ready"); ok = false; }
    else if (adc_channel_setup_dt(&ch) != 0) {
      LOG_ERR("ADC channel %u setup failed (check its channel@ node in app.overlay)",
              ch.channel_id);
      ok = false;
    }
  }
#else
  LOG_WRN("overlay has no io-channels: sensor reads are STUBBED");
#endif
#if DT_NODE_HAS_PROP(USER_NODE, pwms)
  for (const auto& s : servo)
    if (!pwm_is_ready_dt(&s)) { LOG_ERR("PWM channel not ready"); ok = false; }
#else
  LOG_WRN("overlay has no pwms: servo outputs are STUBBED");
#endif
  return ok;
}

Sensors read_sensors() {
  Sensors out{};
#if DT_NODE_HAS_PROP(USER_NODE, io_channels)
  std::int16_t sample = 0;
  adc_sequence seq{};  // adc_sequence_init_dt fills channels/resolution
  seq.buffer = &sample;
  seq.buffer_size = sizeof(sample);
  for (int i = 0; i < 6; ++i) {
    adc_sequence_init_dt(&adc_ch[i], &seq);
    if (adc_read(adc_ch[i].dev, &seq) != 0) return out;  // valid stays false
    const float raw = static_cast<float>(sample);
    if (i < 3) out.pos_rad[i] = counts_to_rad(i, raw);
    else out.current_a[i - 3] = (raw - cfg::kCurrentZeroCounts) * cfg::kAmpsPerCount;
  }
  out.valid = true;
#else
  out.valid = true;  // stub: zeros, so the state machine still runs on a bench
#endif
  return out;
}

void write_servos(const float target_rad[3]) {
#if DT_NODE_HAS_PROP(USER_NODE, pwms)
  for (int i = 0; i < 3; ++i) {
    const float ns = static_cast<float>(cfg::kServoCenterNs) +
                     target_rad[i] * cfg::kServoNsPerRad;
    const auto pulse = static_cast<std::uint32_t>(
        std::clamp(ns, 500'000.0f, 2'500'000.0f));
    pwm_set_pulse_dt(&servo[i], pulse);
  }
#else
  ARG_UNUSED(target_rad);
#endif
}

void disable_outputs() {
#if DT_NODE_HAS_PROP(USER_NODE, pwms)
  for (const auto& s : servo) pwm_set_pulse_dt(&s, 0);  // released, not driven
#endif
}

bool wdt_start() {
#if LEG_HAS_WDT
  if (!device_is_ready(wdt_dev)) { LOG_ERR("watchdog not ready"); return false; }
  wdt_timeout_cfg tcfg{
      .window = {.min = 0, .max = cfg::kWdtTimeoutMs},
      .callback = nullptr,
      .flags = WDT_FLAG_RESET_SOC,
  };
  wdt_channel = wdt_install_timeout(wdt_dev, &tcfg);
  if (wdt_channel < 0) {
    LOG_ERR("wdt_install_timeout(%u ms) failed: %d", cfg::kWdtTimeoutMs, wdt_channel);
    return false;
  }
  const int rc = wdt_setup(wdt_dev, WDT_OPT_PAUSE_HALTED_BY_DBG);
  if (rc != 0) {
    LOG_ERR("wdt_setup failed: %d", rc);
    return false;
  }
  LOG_INF("hardware watchdog armed: %u ms", cfg::kWdtTimeoutMs);
  return true;
#else
  LOG_WRN("watchdog0 alias missing or disabled: HW watchdog OFF (bench mode)");
  return true;
#endif
}

void wdt_feed() {
#if LEG_HAS_WDT
  if (wdt_channel >= 0) ::wdt_feed(wdt_dev, wdt_channel);
#endif
}

}  // namespace leg::hal
