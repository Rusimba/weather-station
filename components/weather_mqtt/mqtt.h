
#ifndef __MQTT_H__
#define __MQTT_H__
#include <esp_err.h>
esp_err_t mqtt_publisher_data(float temp,float humidity);
void mqtt_init_publisher(void);
#endif // __MQTT_H__