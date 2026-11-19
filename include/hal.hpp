#pragma once
// Hardware access: 3x position + 3x current ADC channels, 3x FlexPWM servo
// outputs. All devicetree bindings come from app.overlay; until
// the overlay has real pins, the stubs keep the logic building and testable.
#include <cstdint>

namespace leg::hal {

struct Sensors {
  float pos_rad[3];     // calibrated joint angles
  float current_a[3];   // calibrated joint currents
  bool  valid;
};

bool init();                       // ADC + PWM + watchdog devices ready?
Sensors read_sensors();            // one synchronous sample of all 6 channels

// v1 servo mode: write position targets straight to PWM (diagram:
// "servos now: direct · motors later: PID").
void write_servos(const float target_rad[3]);
void disable_outputs();            // zero pulse width = servos released

// Hardware watchdog (i.MX RT WDOG): supervisor feeds it, nobody else.
bool wdt_start();
void wdt_feed();

}  // namespace leg::hal
