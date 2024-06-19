#include "ht16k33_led.h"
#include "buttons.h"
#include <stdlib.h>
#include <string.h>

#define STACK_SIZE 1024
#define PRIORITY 5

const struct device *ht16k33_led;
int **led_matrix;

// Declare thread stacks
K_THREAD_STACK_DEFINE(tens_stack, STACK_SIZE);
K_THREAD_STACK_DEFINE(units_stack, STACK_SIZE);

// Declare thread data structures
struct k_thread tens_thread;
struct k_thread units_thread;

struct display_params {
    int value;
    int start_idx;
};

int init_led_ht16k33(void) {
    // Get a device reference from a devicetree node identifier.
	ht16k33_led = DEVICE_DT_GET(LED_NODE);

	if (!device_is_ready(ht16k33_led)) {
		printk("Error: LED not rdy\n");
		return -1;
	}

    return 0;
}

void off_led_ht16k33(void) {
    int num_arr_idx = 0;
    for (int i = 0; i < MAX_LED_MATRIX_NUM; i++) {
        led_off(ht16k33_led, i);
    }
}

void display_digit_thread(void *param, void *unused1, void *unused2) {
    struct display_params *params = (struct display_params *)param;
    int value = params->value;
    int start_idx = params->start_idx;
    int num_arr_idx = 0;

    for (int i = 0; i < MAX_LED_MATRIX_NUM; i += 16) {
        for (int j = (i + start_idx); j < ((i + start_idx) + 8); j++) {
            if (led_matrix[value][num_arr_idx] == 1) {
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
}

int display_value_ht16k33(int value) {
    if (value < 0 || value > 999) {
        printk("Error: Invalid value\n");
        return -1;
    }
    if (value > 99) {
        value = value / 10;
    }

    int tens = value / 10;
    int units = value % 10;

    struct display_params tens_params = { .value = tens, .start_idx = 0 };
    struct display_params units_params = { .value = units, .start_idx = 8 };

    // Create threads for tens and units digits
    k_thread_create(&tens_thread, tens_stack, STACK_SIZE,
                    display_digit_thread, &tens_params, NULL, NULL,
                    PRIORITY, 0, K_NO_WAIT);

    k_thread_create(&units_thread, units_stack, STACK_SIZE,
                    display_digit_thread, &units_params, NULL, NULL,
                    PRIORITY, 0, K_NO_WAIT);

    // Wait for threads to complete
    k_thread_join(&tens_thread, K_FOREVER);
    k_thread_join(&units_thread, K_FOREVER);

    return 0;
}

int display_number_matrix(int value) {
    int err = 0;

    led_matrix = allocate_led_matrix_arr(number_led_matrix_arr, MAX_LED_MATRIX_IDX);
    err = display_value_ht16k33(value);
    free_led_matrix_arr(led_matrix, MAX_LED_MATRIX_IDX);

    if (err != 0) {
        return -1;
    }
    return 0;
}

int display_face_state_matrix(int state) {
    int err = 0;

    switch(state) {
        case BAD:
            led_matrix = allocate_led_matrix_arr(bad_face_led_matrix_arr, MAX_FACE_MATRIX_IDX);
            break;
        case NORMAL:
            led_matrix = allocate_led_matrix_arr(normal_face_led_matrix_arr, MAX_FACE_MATRIX_IDX);
            break;
        case GOOD:
            led_matrix = allocate_led_matrix_arr(good_face_led_matrix_arr, MAX_FACE_MATRIX_IDX);
            break;
        default:
            led_matrix = allocate_led_matrix_arr(normal_face_led_matrix_arr, MAX_FACE_MATRIX_IDX);
    }

    err = display_value_ht16k33(1);
    free_led_matrix_arr(led_matrix, MAX_FACE_MATRIX_IDX);

    if (err != 0) {
        return -1;
    }
    return 0;
}

// Function to allocate and copy face_led_matrix_arr to int **ptr
int **allocate_led_matrix_arr(const int led_matrix_placeholder[][MAX_LED_MATRIX_NUM + 1], int max_idx) {
    int **ptr = (int **)malloc((max_idx + 1) * sizeof(int *));
    if (ptr == NULL) {
        printk("Error: Memory allocation failed\n");
        return NULL;
    }

    for (int i = 0; i <= max_idx; i++) {
        ptr[i] = (int *)malloc((MAX_LED_MATRIX_NUM + 1) * sizeof(int));
        if (ptr[i] == NULL) {
            printk("Error: Memory allocation failed\n");
            // Free previously allocated memory
            for (int j = 0; j < i; j++) {
                free(ptr[j]);
            }
            free(ptr);
            return NULL;
        }
        memcpy(ptr[i], led_matrix_placeholder[i], (MAX_LED_MATRIX_NUM + 1) * sizeof(int));
    }

    return ptr;
}

// Function to free the allocated memory for int **ptr
void free_led_matrix_arr(int **ptr, int max_idx) {
    if (ptr == NULL) {
        return;
    }

    for (int i = 0; i <= max_idx; i++) {
        free(ptr[i]);
    }
    free(ptr);
}
