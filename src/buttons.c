#include <zephyr/drivers/gpio.h>
#include "buttons.h"
#include "ht16k33_led.h"
#include "my_service.h"
#include "dht22_sensor.h"

extern struct bt_conn *my_connection;

///////////////// BTN Work Queue Callback Handlers /////////////////
struct k_work button1_work;
void button1_work_handler(struct k_work *work) {
    int humidity = get_humidity();
    if (humidity < 0) {
        return;
    }
    display_value_ht16k33(humidity);
    my_service_send(my_connection, &humidity, (uint16_t)sizeof(humidity));
}
K_WORK_DEFINE(button1_work, button1_work_handler);

struct k_work button2_work;
void button2_work_handler(struct k_work *work) {
    int temperature = get_temperature();
    if (temperature < 0) {
        return;
    }
    display_value_ht16k33(temperature);
    my_service_send(my_connection, &temperature, (uint16_t)sizeof(temperature));
}
K_WORK_DEFINE(button2_work, button2_work_handler);

struct k_work button3_work;
void button3_work_handler(struct k_work *work) {
    
	display_value_ht16k33(3);
}
K_WORK_DEFINE(button3_work, button3_work_handler);

struct k_work button4_work;
void button4_work_handler(struct k_work *work) {
	display_value_ht16k33(4);
}
K_WORK_DEFINE(button4_work, button4_work_handler);

///////////////// Sensor Timer Work Queue Callback Handler /////////////////
struct k_work sensor_work_que;
void sensor_work_handler_cb(struct k_work *sensor_worker) {
	// publish measured sensor values
    int temperature = get_temperature();
    if (temperature < 0) {
        return;
    }
    my_service_send(my_connection, &temperature, (uint16_t)sizeof(temperature));
}
K_WORK_DEFINE(sensor_work_que, sensor_work_handler_cb);

// sensor timer
struct k_timer sensor_update_timer;
void sensor_update_timer_expiry_cb(struct k_timer *timer_id) {
    if (k_work_submit(&sensor_work_que) < 0) {
        printk("Error: work not submitted to queue\n");
        return;
    }
}
K_TIMER_DEFINE(sensor_update_timer, sensor_update_timer_expiry_cb, NULL);

///////////////// BTN INT Service Routine /////////////////
void button1_isr(const struct device *dev, struct gpio_callback *cb, uint32_t pins) {
    k_work_submit(&button1_work);
}

void button2_isr(const struct device *dev, struct gpio_callback *cb, uint32_t pins) {
    k_work_submit(&button2_work);
}

void button3_isr(const struct device *dev, struct gpio_callback *cb, uint32_t pins) {
    k_work_submit(&button3_work);
}

void button4_isr(const struct device *dev, struct gpio_callback *cb, uint32_t pins) {
    k_work_submit(&button4_work);
}

// check ready and configure as input
int configure_gpio_directions(struct gpio_dt_spec *sw_list, int sw_list_len) {
    for (int i = 0; i < sw_list_len; i++) {
        if (!gpio_is_ready_dt(&sw_list[i])) {
            printk("Error: sw not rdy\n");
            return 1;
        }

        if ((gpio_pin_configure_dt(&sw_list[i], GPIO_INPUT)) != 0) {
            printk("Error: sw OUTPUT config\n");
            return 1;
        }
    }
    return 0;
}

// register/map routines and handlers for BTNs
int configure_gpio_interrupts(struct gpio_dt_spec *sw_list, int sw_list_len) {
    void (*btn_isr_arr[])(const struct device *dev, struct gpio_callback *cb, uint32_t pins) = {
        button1_isr,
        button2_isr,
        button3_isr,
        button4_isr
    };

    struct gpio_callback *button_cb_data_arr[] = {
        &button1_cb_data,
        &button2_cb_data,
        &button3_cb_data,
        &button4_cb_data
    };

    for (int i = 0; i < sw_list_len; i++) {
        int err = gpio_pin_interrupt_configure_dt(&sw_list[i], GPIO_INT_EDGE_TO_ACTIVE);
        if (err != 0) {
            printk("Error %d: failed to configure interrupt on %s pin %d\n", err, sw_list[i].port->name, sw_list[i].pin);
            return err;
        }

        gpio_init_callback(button_cb_data_arr[i], btn_isr_arr[i], BIT(sw_list[i].pin));
        gpio_add_callback(sw_list[i].port, button_cb_data_arr[i]);
    }
    return 0;
}
