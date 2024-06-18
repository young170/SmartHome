/* main.c - Application main entry point */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <stddef.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>

#include <string.h>
#include <errno.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/drivers/gpio.h>
#include <soc.h>

#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>

#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>


#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>

#include "buttons.h"
#include "dht22_sensor.h"
#include "ht16k33_led.h"
#include "my_service.h"

#define LED0_NODE DT_ALIAS(led0)
#define LED1_NODE DT_ALIAS(led1)
#define LED2_NODE DT_ALIAS(led2)
#define LED3_NODE DT_ALIAS(led3)

static const struct gpio_dt_spec gpio_led0 = GPIO_DT_SPEC_GET(LED0_NODE, gpios);
static const struct gpio_dt_spec gpio_led1 = GPIO_DT_SPEC_GET(LED1_NODE, gpios);
static const struct gpio_dt_spec gpio_led2 = GPIO_DT_SPEC_GET(LED2_NODE, gpios);
static const struct gpio_dt_spec gpio_led3 = GPIO_DT_SPEC_GET(LED3_NODE, gpios);

#define RECEIVE_TIMEOUT 1000

#define MSG_SIZE 9
#define CO2_MULTIPLIER 256


#define DT_SPEC_AND_COMMA(node_id, prop, idx) \
	ADC_DT_SPEC_GET_BY_IDX(node_id, idx),

/* Data of ADC io-channels specified in devicetree. */
static const struct adc_dt_spec adc_channels[] = {
	DT_FOREACH_PROP_ELEM(DT_PATH(zephyr_user), io_channels,
			     DT_SPEC_AND_COMMA)
};

#define MAX_SENSORVALUE 1000
#define MIN_SENSORVALUE 8
#define SENSOR_INVALID_VALUE 65500

int sound_level = 0;
long map(long x, long in_min, long in_max, long out_min, long out_max)
{
        return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}
int get_sound_level() {
	return sound_level;
}

//K_MSGQ_DEFINE(uart_msgq, MSG_SIZE, 10, 4);

static const struct device *const uart_serial = DEVICE_DT_GET(DT_N_ALIAS_myserial);
int co2_ppm = 0; 

/* receive buffer used in UART ISR callback */
static char rx_buf[MSG_SIZE];
static int rx_buf_pos=0;

enum uart_fsm_state_code {
	UART_FSM_IDLE,
	UART_FSM_HEADER,
	UART_FSM_DATA,
	UART_FSM_CHECKSUM,
	UART_FSM_END,
};

static uint8_t uart_fsm_state = UART_FSM_IDLE; // reset

#define RUN_STATUS_LED          DK_LED1
#define RUN_LED_BLINK_INTERVAL  1000

extern struct k_timer sensor_update_timer;

static struct gpio_dt_spec gpio_sw_list[] = {
    GPIO_DT_SPEC_GET_OR(DT_ALIAS(sw0), gpios, {0}),
    GPIO_DT_SPEC_GET_OR(DT_ALIAS(sw1), gpios, {0}),
    GPIO_DT_SPEC_GET_OR(DT_ALIAS(sw2), gpios, {0}),
    GPIO_DT_SPEC_GET_OR(DT_ALIAS(sw3), gpios, {0}),
};

static uint8_t mfg_data[] = { 0xff, 0xff, 0x00 };

static const struct bt_data ad[] = {
	BT_DATA(BT_DATA_MANUFACTURER_DATA, mfg_data, 3),
};

static const struct bt_data sd[] =
{
	BT_DATA_BYTES(BT_DATA_UUID128_ALL, MY_SERVICE_UUID),
};

struct bt_conn *my_connection;

struct k_sem ble_init_ok;

static void connected(struct bt_conn *conn, uint8_t err)
{
	struct bt_conn_info info;
	char addr[BT_ADDR_LE_STR_LEN];

	my_connection = conn;

	if (err)
	{
		printk("Connection failed (err %u)\n", err);
		return;
	}
	else if(bt_conn_get_info(conn, &info))
	{
		printk("Could not parse connection info\n");
	}
	else
	{
		bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

		printk("Connection established!		\n\
		Connected to: %s					\n\
		Role: %u							\n\
		Connection interval: %u				\n\
		Slave latency: %u					\n\
		Connection supervisory timeout: %u	\n"
		, addr, info.role, info.le.interval, info.le.latency, info.le.timeout);
	}

	gpio_pin_set_dt(&gpio_led2, 1); // BLUETOOTH status LED, LED3
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	printk("Disconnected (reason %u)\n", reason);
	gpio_pin_set_dt(&gpio_led2, 0); // BLUETOOTH status LED, LED3
}

static bool le_param_req(struct bt_conn *conn, struct bt_le_conn_param *param)
{
	//If acceptable params, return true, otherwise return false.
	return true;
}

static void le_param_updated(struct bt_conn *conn, uint16_t interval, uint16_t latency, uint16_t timeout)
{
	struct bt_conn_info info;
	char addr[BT_ADDR_LE_STR_LEN];

	if(bt_conn_get_info(conn, &info))
	{
		printk("Could not parse connection info\n");
	}
	else
	{
		bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

		printk("Connection parameters updated!	\n\
		Connected to: %s						\n\
		New Connection Interval: %u				\n\
		New Slave Latency: %u					\n\
		New Connection Supervisory Timeout: %u	\n"
		, addr, info.le.interval, info.le.latency, info.le.timeout);
	}
}

static struct bt_conn_cb conn_callbacks =
{
	.connected			= connected,
	.disconnected   		= disconnected,
	.le_param_req			= le_param_req,
	.le_param_updated		= le_param_updated
};

static void bt_ready(int err)
{
	if (err)
	{
		printk("BLE init failed with error code %d\n", err);
		return;
	}

	//Configure connection callbacks
	bt_conn_cb_register(&conn_callbacks);

	//Initalize services
	my_service_init();

	//Start advertising
	err = bt_le_adv_start(BT_LE_ADV_CONN, ad, ARRAY_SIZE(ad),sd, ARRAY_SIZE(sd));
	if (err) {
		printk("Advertising failed to start (err %d)\n", err);
		return;
	}

	printk("Advertising successfully started\n");

	k_sem_give(&ble_init_ok);
}


static void error(void)
{
	while (true) {
		printk("Error!\n");
		/* Spin forever */
		k_sleep(K_MSEC(1000)); //1000ms
	}
}


uint8_t check_uart_fsm(uint8_t reset, uint8_t read_data) {
	if(reset)
	   uart_fsm_state = UART_FSM_IDLE;
	else 
	    switch (uart_fsm_state) {
		case UART_FSM_IDLE:
			if (read_data == 0xFF) {
				uart_fsm_state = UART_FSM_HEADER;
			}
			break;
		case UART_FSM_HEADER:
			if (read_data == 0x86) {
				uart_fsm_state = UART_FSM_DATA;
			} else {
				uart_fsm_state = UART_FSM_IDLE;
			}
			break;
		case UART_FSM_DATA:
			if (rx_buf_pos == MSG_SIZE - 2) {
				uart_fsm_state = UART_FSM_CHECKSUM;
			}
			break;
		case UART_FSM_CHECKSUM:
			if (rx_buf_pos == MSG_SIZE - 1) {
				uart_fsm_state = UART_FSM_END;
			}
			break;
		case UART_FSM_END:
			uart_fsm_state = UART_FSM_IDLE;
			break;
		default:
			uart_fsm_state = UART_FSM_IDLE;
			break;
	}
	return uart_fsm_state;
}

unsigned char getCheckSum(char *packet) {
	unsigned char i, checksum=0;
	for( i = 1; i < 8; i++) {
		checksum += packet[i];
	}
	checksum = 0xff - checksum;
	checksum += 1;
	return checksum;
}

/**
 * Read data via UART IRQ.
 *
 * @param dev UART device struct
 * @param user_data Pointer to user data (NULL in this practice)
 */
void serial_callback(const struct device *dev, void *user_data) {
	uint8_t c, high, low;
	char checksum_ok, value_calc_flag;
	int checksum;

	if (!uart_irq_update(uart_serial)) {
		printk("irq_update Error\n");
		return;
	}

	if (!uart_irq_rx_ready(uart_serial)) {
		printk("irq_ready: No data\n");
		return;
	}

	/* read until FIFO empty */
	while (uart_fifo_read(uart_serial, &c, 1) == 1) {
		// for recovery
		if (uart_fsm_state == UART_FSM_IDLE) {
			rx_buf_pos = 0;
		}
		check_uart_fsm(0,c);

		if (rx_buf_pos >= MSG_SIZE) {
			rx_buf_pos = 0;
		}
		rx_buf[rx_buf_pos++] = c;
	}

	// calculate checksum, and compare with received checksum
	if(uart_fsm_state == UART_FSM_END){
	  checksum = getCheckSum(rx_buf);
	  checksum_ok = (checksum == rx_buf[8]);
	  if (checksum_ok) {
		// printk("Checksum OK (%d == %d, index=%d)\n", checksum, rx_buf[8], rx_buf_pos);
	 
	     // check if we received all data and checksum is OK
	    value_calc_flag = (rx_buf_pos == MSG_SIZE);
	    if (value_calc_flag) {
		  high = rx_buf[2];
		  low = rx_buf[3];
		  co2_ppm = (high * CO2_MULTIPLIER) + low;
		//   printk("CO2: %d ppm (high = %d, low = %d)\n", co2_ppm , high, low);
		// print message buffer
		  for (int i = 0; i < MSG_SIZE; i+=1) {
			printk("%x ", rx_buf[i]);
		  }
		  printk("\n");
		}
	  } 
	  else {
		printk("Checksum failed (%d == %d, index=%d)\n", checksum, rx_buf[8], rx_buf_pos);
	  }
	  check_uart_fsm(1,0); // reset
	}
}

void serial_write() {
	uint8_t tx_buf[MSG_SIZE] = {0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79};
	for (int i = 0; i < MSG_SIZE; i+=1) {
		uart_poll_out(uart_serial, tx_buf[i]);
	}
}

int get_co2_ppm() {
	return co2_ppm;
}

int main(void)
{
	int err = 0;

	///////////////// LED Init /////////////////
	struct gpio_dt_spec gpio_led_list[4] = {gpio_led0, gpio_led1, gpio_led2, gpio_led3};
	int led_list_len = (sizeof(gpio_led_list)) / (sizeof(gpio_led_list[0]));

	// set BTNs as output
	err = configure_gpio_directions(gpio_sw_list, ARRAY_SIZE(gpio_sw_list), gpio_led_list, led_list_len);
    if (err) {
        printk("Error: config GPIO\n");
        return 0;
    }

	gpio_pin_set_dt(&gpio_led3, 1); // start, LED1

	// set BTN INT
    err = configure_gpio_interrupts(gpio_sw_list, ARRAY_SIZE(gpio_sw_list));
    if (err) {
        printk("Error: interrupt config GPIO\n");
        return 0;
    }

	err = init_led_ht16k33();
    if (err) {
        printk("HT16K33 LED init failed\n");
        return 0;
    }

	err = init_dht();
	if (err) {
		printk("DHT22 init failed\n");
		return 0;
	}

	///////////////// Bluetooth Initialization /////////////////
	k_sem_init(&ble_init_ok, 0, 1);
	err = bt_enable(bt_ready);

	if (err)
	{
		printk("BLE initialization failed\n");
		error(); //Catch error
	}

	err = k_sem_take(&ble_init_ok, K_MSEC(500));
	if (!err)
	{
		printk("Bluetooth initialized\n");
	} else
	{
		printk("BLE initialization did not complete in time\n");
		error(); //Catch error
	}

	///////////////// Start Bluetooth Connection /////////////////
	my_service_init();

	///////////////// Start Sensor Timer /////////////////
	k_timer_start(&sensor_update_timer, K_SECONDS(10), K_SECONDS(10));

	// co2
	if (!device_is_ready(uart_serial)) {
		printk("UART device not found!");
		return 0;
	}

	/* configure interrupt and callback to receive data */
	int ret = uart_irq_callback_user_data_set(uart_serial, serial_callback, NULL);

	if (ret < 0) {
		if (ret == -ENOTSUP) {
			printk("Interrupt-driven UART API support not enabled\n");
		} else if (ret == -ENOSYS) {
			printk("UART device does not support interrupt-driven API\n");
		} else {
			printk("Error setting UART callback: %d\n", ret);
		}
		return 0;
	}
	uart_irq_rx_enable(uart_serial);

	uint32_t count = 0;
	uint16_t buf;
	uint32_t sound_value = 0;
	struct adc_sequence sequence = {
	.buffer = &buf,
	/* buffer size in bytes, not number of samples */
	.buffer_size = sizeof(buf),
			// .oversampling = 0,
			// .calibrate = 0,
	};

		// printk("Sound Sensor : array_size [%d]\n", ARRAY_SIZE(adc_channels));
	/* Configure channels individually prior to sampling. */
	for (size_t i = 0U; i < ARRAY_SIZE(adc_channels); i++) {
		if (!adc_is_ready_dt(&adc_channels[i])) {
			printk("ADC controller device %s not ready\n", adc_channels[i].dev->name);
			return 0;
		}

		err = adc_channel_setup_dt(&adc_channels[i]);
		if (err < 0) {
			printk("Could not setup channel #%d (%d)\n", i, err);
			return 0;
		}
	}
	while(1) {
		gpio_pin_toggle_dt(&gpio_led1); // status blink

		printk("ADC reading[%u]:\n", count++);

		(void)adc_sequence_init_dt(&adc_channels[0], &sequence);
		err = adc_read(adc_channels[0].dev, &sequence);
		if (err < 0) {
			printk("Could not read (%d)\n", err);
			k_sleep(K_MSEC(100));
			continue;
		}

		sound_value = (int32_t)buf;
				if(sound_value >= SENSOR_INVALID_VALUE){
			printk("sound_value: invalid data %" PRIu32 "\n", sound_value);
			k_sleep(K_MSEC(100));
			continue;
		}
		sound_level = map(sound_value, 0, MAX_SENSORVALUE, 0, MIN_SENSORVALUE);
		printk("sound_value: %" PRIu32 " sound_level : %d\n", sound_value, sound_level);

		k_sleep(K_MSEC(5000));
		serial_write();
	}
}
