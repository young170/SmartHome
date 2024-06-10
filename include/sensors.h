#ifndef SENSORS_H
#define SENSORS_H

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>

int init_dht(void);
// return fetched temperature data from DHT22 sensor
int get_temperature(void);
// return fetched humidity data from DHT22 sensor
int get_humidity(void);

#endif // SENSORS_H
