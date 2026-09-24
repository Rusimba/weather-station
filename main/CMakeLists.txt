idf_component_register(SRCS "weather-station.c"
                    INCLUDE_DIRS "."
                    REQUIRES wifi dht weather_mqtt esp_driver_i2c)