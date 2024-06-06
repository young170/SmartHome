#include <zephyr/kernel.h>
#include <dk_buttons_and_leds.h>
#include "buttons.h"
#include "bluetooth.h"
#include "sensors.h"
#include "led.h"

#define RUN_STATUS_LED          DK_LED1
#define RUN_LED_BLINK_INTERVAL  1000

static struct gpio_dt_spec gpio_sw_list[] = {
    GPIO_DT_SPEC_GET_OR(DT_ALIAS(sw0), gpios, {0}),
    GPIO_DT_SPEC_GET_OR(DT_ALIAS(sw1), gpios, {0}),
    GPIO_DT_SPEC_GET_OR(DT_ALIAS(sw2), gpios, {0}),
    GPIO_DT_SPEC_GET_OR(DT_ALIAS(sw3), gpios, {0}),
};

int main(void) {
    int blink_status = 0;
    int err;

    err = configure_gpio_directions(gpio_sw_list, ARRAY_SIZE(gpio_sw_list));
    if (err) {
        printk("Error: config GPIO\n");
        return 0;
    }

    err = configure_gpio_interrupts(gpio_sw_list, ARRAY_SIZE(gpio_sw_list));
    if (err) {
        printk("Error: interrupt config GPIO\n");
        return 0;
    }

    err = dk_leds_init();
    if (err) {
        printk("LEDs init failed (err %d)\n", err);
        return 0;
    }

    err = init_buttons();
    if (err) {
        printk("Button init failed (err %d)\n", err);
        return 0;
    }

    err = init_led_ht16k33();
    if (err) {
        printk("HT16K33 LED init failed\n");
        return 0;
    }

    setup_bluetooth();

    for (;;) {
        dk_set_led(RUN_STATUS_LED, (++blink_status) % 2);
        k_sleep(K_MSEC(RUN_LED_BLINK_INTERVAL));
    }

    // clean up
    
}
