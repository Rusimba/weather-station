#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "dht.h"
#include "esp_log.h"
#include "stdint.h"
#include "wifi.h"
#include "mqtt.h"
#include "driver/i2c_master.h"
#include "inttypes.h"
#include "bme280.h"

static const char *TAG = "weather";


typedef struct {
    float temperature;
    float humidity;
    float pressure; 
}measurement_t;


static QueueHandle_t s_queue_desc;
void sampler_task(void *pvParameters){
     while (1)
    {
        float humidity;
        float temperature;
        float pressure;
        esp_err_t err = bme280_read(&humidity, &temperature, &pressure);
        if (err == ESP_OK){
            measurement_t measurement = {
                .humidity = humidity,
                .temperature = temperature,
            };
            BaseType_t res =xQueueSend(s_queue_desc,&measurement,0);
            if (res == pdTRUE){
            ESP_LOGI(TAG, "Temp: %.1f C, Humidity: %.1f %%, Pressure: %.1f hPa", temperature, humidity, pressure);
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
    i2c_master_bus_config_t bus_config =   {
    .i2c_port = -1,
    .sda_io_num = GPIO_NUM_21,
    .scl_io_num = GPIO_NUM_22,
    .clk_source = I2C_CLK_SRC_DEFAULT,
    .glitch_ignore_cnt = 7,
    .flags.enable_internal_pullup = true,
    };

    i2c_master_bus_handle_t bus_handle;
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &bus_handle));
    ESP_ERROR_CHECK(bme280_init(bus_handle));
    
   
    wifi_init_sta();
    mqtt_init_publisher();
    s_queue_desc = xQueueCreate(30, sizeof(measurement_t));
    xTaskCreate(sampler_task,"sampler",2048,NULL,2,NULL);
    xTaskCreate(publisher_task,"publisher",4096,NULL,1,NULL);
}
