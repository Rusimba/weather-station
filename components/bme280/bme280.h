#ifndef __BME280_H__
#define __BME280_H__
#include <esp_err.h>
#include <driver/i2c_master.h>
esp_err_t bme280_init(i2c_master_bus_handle_t bus_handle);
esp_err_t bme280_read(float *humidity, float *temperature,float *pressure);
#endif //__BME280_H__