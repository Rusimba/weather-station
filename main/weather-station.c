#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "dht.h"
#include "esp_log.h"
#include "wifi.h"
#include "mqtt.h"
static const char *TAG = "weather";
typedef struct {
    float temperature;
    float humidity;
}measurement_t;
static QueueHandle_t s_queue_desc;
void sampler_task(void *pvParameters){
     while (1)
    {
        float humidity;
        float temperature;
        esp_err_t err = dht_read_float_data(DHT_TYPE_DHT11, GPIO_NUM_4, &humidity, &temperature);
        if (err == ESP_OK){
            measurement_t measurement = {
                .humidity = humidity,
                .temperature = temperature,
            };
            BaseType_t res =xQueueSend(s_queue_desc,&measurement,0);
            if (res == pdTRUE){
            ESP_LOGI(TAG, "Temp: %.1f C, Humidity: %.1f %%", temperature, humidity);
            }else{
                ESP_LOGE(TAG, "Failed to add queue");
            }
        }else {
            ESP_LOGE(TAG, "Failed to read sensor");
        }
        vTaskDelay(pdMS_TO_TICKS(4000));
    }
}
void publisher_task(void *pvParameters){
   measurement_t measurement;
    while (1)
    {
        BaseType_t res = xQueueReceive(s_queue_desc,&measurement,portMAX_DELAY);
        if (res == pdTRUE){
        mqtt_publisher_data(measurement.temperature, measurement.humidity);
        ESP_LOGI(TAG, "Queued for publish");
        }else {
            ESP_LOGE(TAG, "NO QUEUE");
        }
    }
}
void app_main(void)
{
    wifi_init_sta();
    mqtt_init_publisher();
    s_queue_desc = xQueueCreate(30, sizeof(measurement_t));
    xTaskCreate(sampler_task,"sampler",2048,NULL,2,NULL);
    xTaskCreate(publisher_task,"publisher",4096,NULL,1,NULL);
}
