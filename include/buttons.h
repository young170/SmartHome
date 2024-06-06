#ifndef BUTTONS_H
#define BUTTONS_H

#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>

// Function declarations for button and GPIO setup
int init_buttons(void);
int configure_gpio_interrupts(struct gpio_dt_spec *sw_list, int sw_list_len);
int configure_gpio_directions(struct gpio_dt_spec *sw_list, int sw_list_len);

// GPIO callback data
static struct gpio_callback button1_cb_data;
static struct gpio_callback button2_cb_data;
static struct gpio_callback button3_cb_data;
static struct gpio_callback button4_cb_data;

#endif // BUTTONS_H
