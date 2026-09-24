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

#define BME280_OSRS_H 0b001
#define BME280_OSRS_P 0b001
#define BME280_MODE_FORCED 0b01
#define BME280_OSRS_T 0b001
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
    i2c_master_bus_config_t bus_config =   {
    .i2c_port = -1,
    .sda_io_num = GPIO_NUM_21,
    .scl_io_num = GPIO_NUM_22,
    .clk_source = I2C_CLK_SRC_DEFAULT,
    .glitch_ignore_cnt = 7,
    .flags.enable_internal_pullup = true,
    };
    i2c_device_config_t dev_config = {
    .dev_addr_length = I2C_ADDR_BIT_LEN_7,
    .device_address = 0x76,
    .scl_speed_hz = 100000,
    };
    i2c_master_bus_handle_t bus_handle;
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &bus_handle));
    uint8_t address = 0x08;
    while (address <= 0x77){
        esp_err_t  res = i2c_master_probe( bus_handle,  address,  50);
        if (res==ESP_OK){
            ESP_LOGI(TAG,"Found address: %02x ",address);
        }
        address++;
    }
    i2c_master_dev_handle_t dev_handle;
    uint8_t reg = 0xD0;
    uint8_t chip_id;
    ESP_ERROR_CHECK(i2c_master_bus_add_device( bus_handle,&dev_config, &dev_handle));
    esp_err_t  res = i2c_master_transmit_receive(dev_handle, &reg, 1, &chip_id, 1, 100);
    if (res==ESP_OK){
            ESP_LOGI(TAG,"REG =  %02x ",chip_id);
        }
    else{
        ESP_LOGI(TAG,"ERROR =  %02x ",res);
    }
    uint8_t ctrl_hum = BME280_OSRS_H;
    uint8_t ctrl_meas = (BME280_OSRS_T<<5)|(BME280_OSRS_P<<2) | BME280_MODE_FORCED;
    uint8_t buffer[4] = {0xF2,ctrl_hum,0xF4,ctrl_meas};
    ESP_ERROR_CHECK(i2c_master_transmit(dev_handle,buffer,4,500));
     vTaskDelay(pdMS_TO_TICKS(50)); 
    uint8_t start_reg = 0xF2;
    uint8_t read_arr[3];
    esp_err_t  read_res = i2c_master_transmit_receive(dev_handle, &start_reg, 1, read_arr, 3, 100);
    if (read_res==ESP_OK){
            ESP_LOG_BUFFER_HEX(TAG, read_arr, 3);
        }
    else{
        ESP_LOGI(TAG,"ERROR READ ADDRES registor ");
    }
    
    wifi_init_sta();
    mqtt_init_publisher();
    s_queue_desc = xQueueCreate(30, sizeof(measurement_t));
    xTaskCreate(sampler_task,"sampler",2048,NULL,2,NULL);
    xTaskCreate(publisher_task,"publisher",4096,NULL,1,NULL);
}
