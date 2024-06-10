#include "ht16k33_led.h"

const struct device *ht16k33_led;

int init_led_ht16k33(void) {
    // Get a device reference from a devicetree node identifier.
	ht16k33_led = DEVICE_DT_GET(LED_NODE);

	if (!device_is_ready(ht16k33_led)) {
		printk("Error: LED not rdy\n");
		return -1;
	}

    return 0;
}

int display_value_ht16k33(int value) {
    if (value < 0 || value > 99) {
        printk("Error: Invalid value\n");
        return -1;
    }

    if (!device_is_ready(ht16k33_led)) {
        printk("Error: LED not ready\n");
        return -1;
    }

    int tens = value / 10;
    int units = value % 10;
    int num_arr_idx = 0;

    // Display tens digit
    for (int i = 0; i < MAX_LED_MATRIX_NUM; i += 16) {
        for (int j = i; j < (i + 8); j++) {
            if (number_led_matrix_arr[tens][num_arr_idx] == 1) {
                if (led_on(ht16k33_led, j) != 0) {
                    return -1;
                }
            } else {
                if (led_off(ht16k33_led, j) != 0) {
                    return -1;
                }
            }
            num_arr_idx++;
        }
    }

    num_arr_idx = 0;

    // Display units digit
    for (int i = 0; i < MAX_LED_MATRIX_NUM; i += 16) {
        for (int j = (i + 8); j < (i + 16); j++) {
            if (number_led_matrix_arr[units][num_arr_idx] == 1) {
                if (led_on(ht16k33_led, j) != 0) {
                    return -1;
                }
            } else {
                if (led_off(ht16k33_led, j) != 0) {
                    return -1;
                }
            }
            num_arr_idx++;
        }
    }

    return 0;
}
