#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "dht.h"
#include "esp_log.h"
#include "wifi.h"
#include "mqtt.h"
static const char *TAG = "weather";
void app_main(void)
{
    wifi_init_sta();
    mqtt_init_publisher();
    while (1)
    {
        float humidity;
        float temperature;
        esp_err_t err = dht_read_float_data(DHT_TYPE_DHT11, GPIO_NUM_4, &humidity, &temperature);
        if (err == ESP_OK){
            ESP_LOGI(TAG, "Temp: %.1f C, Humidity: %.1f %%", temperature, humidity);
            mqtt_publisher_data(temperature, humidity);
            ESP_LOGI(TAG, "Queued for publish");
        }else {
            ESP_LOGE(TAG, "Failed to read sensor");
        }
        vTaskDelay(pdMS_TO_TICKS(4000));
    }
    
}
