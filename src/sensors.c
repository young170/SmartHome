#include "sensors.h"

const struct device *const dht22 = DEVICE_DT_GET_ONE(aosong_dht);

int init_dht(void) {
    if (!device_is_ready(dht22)) {
		printk("Device %s is not ready\n", dht22->name);
		return 1;
	}

    return 0;
}

int get_temperature(void) {
    int rc = sensor_sample_fetch(dht22);

    if (rc != 0) {
        // printk("Sensor fetch failed: %d\n", rc);
        return -1;
    }

    struct sensor_value temperature;

    rc = sensor_channel_get(dht22, SENSOR_CHAN_AMBIENT_TEMP, &temperature);
    if (rc != 0) {
        printk("Sensor value get failed: %d\n", rc);
        return -1;
    } else {
        printk("Temperature: %d\n", temperature.val1);
    }

    return temperature.val1;
}

int get_humidity(void) {
    int rc = sensor_sample_fetch(dht22);

    if (rc != 0) {
        // printk("Sensor fetch failed: %d\n", rc);
        return -1;
    }

    struct sensor_value humidity;

    rc = sensor_channel_get(dht22, SENSOR_CHAN_HUMIDITY, &humidity);
    if (rc != 0) {
        printk("Sensor value get failed: %d\n", rc);
        return -1;
    } else {
        printk("Humidity: %d\n", humidity.val1);
    }

    return humidity.val1;
}
