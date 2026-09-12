#include "mqtt.h"
#include "config.h"
#include "mqtt_client.h"
#include "esp_log.h"
#include <stdio.h>
static esp_mqtt_client_handle_t s_mqtt_client;
static const char *TAG = "weather";

esp_err_t mqtt_publisher_data(float temp, float humidity){
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "{\"temperature\":%.1f,\"humidity\":%.1f}", temp, humidity );
    int result= esp_mqtt_client_publish(s_mqtt_client, MQTT_TOPIC, buffer, 0, 0, 1);
    if (result < 0){
        ESP_LOGE(TAG, "Failed to Publish to MQTT, publish_id: %d", result);
        return ESP_FAIL;
    }
    return ESP_OK;
}
esp_mqtt_client_config_t mqtt_config ={
    .broker = {
        .address = {
            .uri= MQTT_BROKER_URI
        }
    }
};

void mqtt_init_publisher(void)
{
    
    s_mqtt_client =esp_mqtt_client_init(&mqtt_config);
    ESP_LOGI(TAG, "Connecting to broker...");
    ESP_ERROR_CHECK(esp_mqtt_client_start(s_mqtt_client));
}

