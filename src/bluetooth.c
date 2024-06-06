#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <bluetooth/services/lbs.h>
#include <zephyr/settings/settings.h>
#include <dk_buttons_and_leds.h>
#include "bluetooth.h"

static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, sizeof(CONFIG_BT_DEVICE_NAME) - 1),
};

static const struct bt_data sd[] = {
    BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_LBS_VAL),
};

static bool app_button_state;

static void connected(struct bt_conn *conn, uint8_t err) {
    if (err) {
        printk("Connection failed (err %u)\n", err);
        return;
    }
    printk("Connected\n");
    dk_set_led_on(DK_LED2);
}

static void disconnected(struct bt_conn *conn, uint8_t reason) {
    printk("Disconnected (reason %u)\n", reason);
    dk_set_led_off(DK_LED2);
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected        = connected,
	.disconnected     = disconnected,
};

static void app_led_cb(bool led_state) {
    dk_set_led(DK_LED3, led_state);
}

static bool app_button_cb(void) {
    return app_button_state;
}

static struct bt_lbs_cb lbs_callbacs = {
    .led_cb = app_led_cb,
    .button_cb = app_button_cb,
};

void setup_bluetooth(void) {
    int err;

    printk("Starting Bluetooth Peripheral LBS example\n");

    if (IS_ENABLED(CONFIG_BT_LBS_SECURITY_ENABLED)) {
        static struct bt_conn_auth_cb conn_auth_callbacks = {
            .passkey_display = NULL,
            .cancel = NULL,
        };
        static struct bt_conn_auth_info_cb conn_auth_info_callbacks = {
            .pairing_complete = NULL,
            .pairing_failed = NULL,
        };

        err = bt_conn_auth_cb_register(&conn_auth_callbacks);
        if (err) {
            printk("Failed to register authorization callbacks.\n");
            return;
        }

        err = bt_conn_auth_info_cb_register(&conn_auth_info_callbacks);
        if (err) {
            printk("Failed to register authorization info callbacks.\n");
            return;
        }
    }

    err = bt_enable(NULL);
    if (err) {
        printk("Bluetooth init failed (err %d)\n", err);
        return;
    }

    printk("Bluetooth initialized\n");

    if (IS_ENABLED(CONFIG_SETTINGS)) {
        settings_load();
    }

    err = bt_lbs_init(&lbs_callbacs);
    if (err) {
        printk("Failed to init LBS (err:%d)\n", err);
        return;
    }

    err = bt_le_adv_start(BT_LE_ADV_CONN, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
    if (err) {
        printk("Advertising failed to start (err %d)\n", err);
        return;
    }

    printk("Advertising successfully started\n");
}
