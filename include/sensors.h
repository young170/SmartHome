#ifndef SENSORS_H
#define SENSORS_H

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <stdio.h>

int init_dht(void);
int get_temperature(void);
int get_humidity(void);

#endif // SENSORS_H
