// Robotic Dog - legs firmware: blink test
// Board: Teensy 4.1   RTOS: Zephyr   Language: C++20
//
// Turns the orange LED on and off and prints each change on the USB serial
// port. If the LED blinks and the lines appear, your setup works.

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>

static_assert(__cplusplus >= 202002L, "Build as C++20 (CONFIG_STD_CPP20=y in prj.conf)");

namespace {

constexpr int kBlinkMs = 500; // how long the LED stays on, then off

// Orange LED, from the board's devicetree alias "led0"
const gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);

} // namespace

int main()
{
    if (!gpio_is_ready_dt(&led)) {
        printk("ERROR: the LED is not ready\n");
        return 0;
    }
    gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);

    for (bool on = true;; on = !on) {
        gpio_pin_set_dt(&led, on);
        printk("LED %s (%lld ms)\n", on ? "on " : "off", static_cast<long long>(k_uptime_get()));
        k_msleep(kBlinkMs);
    }
}
