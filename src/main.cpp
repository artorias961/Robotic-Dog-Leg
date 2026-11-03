// Robotic Dog - legs firmware
// Board: Teensy 4.1   RTOS: Zephyr   Language: C++20
//
// Blinks the Teensy's orange LED and logs a heartbeat on the USB serial port,
// so you can see both that flashing worked and that the console works.

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

#include <array>
#include <span>

static_assert(__cplusplus >= 202002L, "Build as C++20 (CONFIG_STD_CPP20=y in prj.conf)");

LOG_MODULE_REGISTER(legs, LOG_LEVEL_INF);

namespace {

// Orange LED on pin 13, from the board's devicetree alias "led0"
const gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);

struct Leg {
    const char *name;
    int angle_deg;
};

// C++20 designated initializers; std::array deduces its size (4)
std::array legs{
    Leg{.name = "front-left", .angle_deg = 0},
    Leg{.name = "front-right", .angle_deg = 0},
    Leg{.name = "back-left", .angle_deg = 0},
    Leg{.name = "back-right", .angle_deg = 0},
};

// C++20 std::span: takes any contiguous group of legs without copying
void log_legs(std::span<const Leg> all)
{
    for (const Leg &leg : all) {
        LOG_INF("  %s: %d deg", leg.name, leg.angle_deg);
    }
}

} // namespace

int main()
{
    LOG_INF("Robotic Dog legs firmware, C++ standard %ld", static_cast<long>(__cplusplus));

    if (!gpio_is_ready_dt(&led)) {
        LOG_ERR("LED GPIO is not ready");
        return 0;
    }
    gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);

    for (unsigned int tick = 0;; ++tick) {
        gpio_pin_toggle_dt(&led);

        if (tick % 4 == 0) { // every 2 s (4 x 500 ms)
            LOG_INF("uptime %lld ms, legs:", k_uptime_get());
            log_legs(legs);
        }
        k_msleep(500);
    }
    return 0;
}
