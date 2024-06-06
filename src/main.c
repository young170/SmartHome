#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <soc.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <bluetooth/services/lbs.h>
#include <zephyr/settings/settings.h>
#include <dk_buttons_and_leds.h>

#define DEVICE_NAME             CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN         (sizeof(DEVICE_NAME) - 1)

#define RUN_STATUS_LED          DK_LED1
#define CON_STATUS_LED          DK_LED2
#define RUN_LED_BLINK_INTERVAL  1000
#define USER_LED                DK_LED3
#define USER_BUTTON             DK_BTN1_MSK

// GPIO info
#define SW0_NODE DT_ALIAS(sw0)
#define SW1_NODE DT_ALIAS(sw1)
#define SW2_NODE DT_ALIAS(sw2)
#define SW3_NODE DT_ALIAS(sw3)

static const struct gpio_dt_spec gpio_sw0 = GPIO_DT_SPEC_GET(SW0_NODE, gpios);
static const struct gpio_dt_spec gpio_sw1 = GPIO_DT_SPEC_GET(SW1_NODE, gpios);
static const struct gpio_dt_spec gpio_sw2 = GPIO_DT_SPEC_GET(SW2_NODE, gpios);
static const struct gpio_dt_spec gpio_sw3 = GPIO_DT_SPEC_GET(SW3_NODE, gpios);

static struct gpio_callback button1_cb_data;
static struct gpio_callback button2_cb_data;
static struct gpio_callback button3_cb_data;
static struct gpio_callback button4_cb_data;

static bool app_button_state;
uint32_t button_state = 0;

static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

static const struct bt_data sd[] = {
    BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_LBS_VAL),
};

struct k_work button1_work;
void button1_work_handler(struct k_work *work) {
    printk("BTN1\n");
}
K_WORK_DEFINE(button1_work, button1_work_handler);

struct k_work button2_work;
void button2_work_handler(struct k_work *work) {
    printk("BTN2\n");
}
K_WORK_DEFINE(button2_work, button2_work_handler);

struct k_work button3_work;
void button3_work_handler(struct k_work *work) {
    printk("BTN3\n");
}
K_WORK_DEFINE(button3_work, button3_work_handler);

struct k_work button4_work;
void button4_work_handler(struct k_work *work) {
    printk("BTN4\n");
}
K_WORK_DEFINE(button4_work, button4_work_handler);

// Work queue
struct k_work sensor_work_que;
void sensor_work_handler_cb(struct k_work *sensor_worker) {
    // publish measured sensor values
    printk("Publish sensor values\n");
}
K_WORK_DEFINE(sensor_work_que, sensor_work_handler_cb);

// LED timer
struct k_timer sensor_update_timer;
void sensor_update_timer_expiry_cb(struct k_timer *timer_id) {
    if (k_work_submit(&sensor_work_que) < 0) {
        printk("Error: work not submitted to queue\n");
        return;
    }
}
K_TIMER_DEFINE(sensor_update_timer, sensor_update_timer_expiry_cb, NULL);

static void connected(struct bt_conn *conn, uint8_t err)
{
    if (err) {
        printk("Connection failed (err %u)\n", err);
        return;
    }

    printk("Connected\n");
    dk_set_led_on(CON_STATUS_LED);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
    printk("Disconnected (reason %u)\n", reason);
    dk_set_led_off(CON_STATUS_LED);
}

#ifdef CONFIG_BT_LBS_SECURITY_ENABLED
static void security_changed(struct bt_conn *conn, bt_security_t level, enum bt_security_err err)
{
    char addr[BT_ADDR_LE_STR_LEN];
    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    if (!err) {
        printk("Security changed: %s level %u\n", addr, level);
    } else {
        printk("Security failed: %s level %u err %d\n", addr, level, err);
    }
}
#endif

BT_CONN_CB_DEFINE(conn_callbacks) = {
    .connected        = connected,
    .disconnected     = disconnected,
#ifdef CONFIG_BT_LBS_SECURITY_ENABLED
    .security_changed = security_changed,
#endif
};

#if defined(CONFIG_BT_LBS_SECURITY_ENABLED)
static void auth_passkey_display(struct bt_conn *conn, unsigned int passkey)
{
    char addr[BT_ADDR_LE_STR_LEN];
    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
    printk("Passkey for %s: %06u\n", addr, passkey);
}

static void auth_cancel(struct bt_conn *conn)
{
    char addr[BT_ADDR_LE_STR_LEN];
    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
    printk("Pairing cancelled: %s\n", addr);
}

static void pairing_complete(struct bt_conn *conn, bool bonded)
{
    char addr[BT_ADDR_LE_STR_LEN];
    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
    printk("Pairing completed: %s, bonded: %d\n", addr, bonded);
}

static void pairing_failed(struct bt_conn *conn, enum bt_security_err reason)
{
    char addr[BT_ADDR_LE_STR_LEN];
    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
    printk("Pairing failed conn: %s, reason %d\n", addr, reason);
}

static struct bt_conn_auth_cb conn_auth_callbacks = {
    .passkey_display = auth_passkey_display,
    .cancel = auth_cancel,
};

static struct bt_conn_auth_info_cb conn_auth_info_callbacks = {
    .pairing_complete = pairing_complete,
    .pairing_failed = pairing_failed
};
#else
static struct bt_conn_auth_cb conn_auth_callbacks;
static struct bt_conn_auth_info_cb conn_auth_info_callbacks;
#endif

static void app_led_cb(bool led_state)
{
    dk_set_led(USER_LED, led_state);
}

static bool app_button_cb(void)
{
    return app_button_state;
}

static struct bt_lbs_cb lbs_callbacs = {
    .led_cb    = app_led_cb,
    .button_cb = app_button_cb,
};

static void button_changed(uint32_t button_state, uint32_t has_changed)
{
    if (has_changed & USER_BUTTON) {
        uint32_t user_button_state = button_state & USER_BUTTON;

        int err = bt_lbs_send_button_state(user_button_state);
        if (err == -EACCES) {
            printk("Notify not enabled\n");
        } else if (err != 0) {
            printk("Send button state error\n");
        }

        app_button_state = user_button_state ? true : false;
    }
}

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

static int init_button(void)
{
    int err;
    err = dk_buttons_init(button_changed);
    if (err) {
        printk("Cannot init buttons (err: %d)\n", err);
    }
    return err;
}

int int_configure_gpio(struct gpio_dt_spec *sw_list, int sw_list_len) {
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

int dir_configure_gpio(struct gpio_dt_spec *sw_list, int sw_list_len) {
    // set GPIO SW (BTN) as INPUT
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

int main(void)
{
    int blink_status = 0;
    int err;

    printk("Starting Bluetooth Peripheral LBS example\n");

    struct gpio_dt_spec gpio_sw_list[4] = {gpio_sw0, gpio_sw1, gpio_sw2, gpio_sw3};
    int sw_list_len = (sizeof(gpio_sw_list)) / (sizeof(gpio_sw_list[0]));

    err = dir_configure_gpio(gpio_sw_list, sw_list_len);
    if (err != 0) {
        printk("Error: config GPIO\n");
        return 0;
    }

    err = int_configure_gpio(gpio_sw_list, sw_list_len);
    if (err != 0) {
        printk("Error: interrupt config GPIO\n");
        return 0;
    }

    // LDS-related settings
    err = dk_leds_init();
    if (err) {
        printk("LEDs init failed (err %d)\n", err);
        return 0;
    }

    err = init_button();
    if (err) {
        printk("Button init failed (err %d)\n", err);
        return 0;
    }

    // BLUETOOTH
    if (IS_ENABLED(CONFIG_BT_LBS_SECURITY_ENABLED)) {
        err = bt_conn_auth_cb_register(&conn_auth_callbacks);
        if (err) {
            printk("Failed to register authorization callbacks.\n");
            return 0;
        }

        err = bt_conn_auth_info_cb_register(&conn_auth_info_callbacks);
        if (err) {
            printk("Failed to register authorization info callbacks.\n");
            return 0;
        }
    }

    err = bt_enable(NULL);
    if (err) {
        printk("Bluetooth init failed (err %d)\n", err);
        return 0;
    }

    printk("Bluetooth initialized\n");

    if (IS_ENABLED(CONFIG_SETTINGS)) {
        settings_load();
    }

    err = bt_lbs_init(&lbs_callbacs);
    if (err) {
        printk("Failed to init LBS (err:%d)\n", err);
        return 0;
    }

    err = bt_le_adv_start(BT_LE_ADV_CONN, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
    if (err) {
        printk("Advertising failed to start (err %d)\n", err);
        return 0;
    }

    printk("Advertising successfully started\n");

    // Initialize sensor measurement update timer
    // k_timer_start(&sensor_update_timer, K_SECONDS(5), K_SECONDS(5));

    for (;;) {
        dk_set_led(RUN_STATUS_LED, (++blink_status) % 2);
        k_sleep(K_MSEC(RUN_LED_BLINK_INTERVAL));
    }
}
