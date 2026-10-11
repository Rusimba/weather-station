# Архитектура

Путь одного измерения от датчика до графика в браузере.

```mermaid
flowchart TB
  BME["BME280 sensor"] -- "I2C: register bytes" --> DRV

  subgraph ESP32["ESP32 (ESP-IDF + FreeRTOS)"]
    DRV["bme280 driver"] -- "3 × float" --> S["sampler_task"]
    S -- "measurement_t via queue" --> P["publisher_task"]
    P -- "JSON" --> M["esp-mqtt client"]
  end

  M -- "Wi-Fi → TCP :1883, MQTT QoS 0" --> B[("Mosquitto broker")]
  B -- "topic iot/weather/+/telemetry" --> N["NestJS backend"]
  N -- "1. SQL via Prisma" --> DB[("PostgreSQL")]
  N -- "2. WebSocket: new measurements" --> R["React + Recharts dashboard"]
  R -- "HTTP GET /measurements: history" --> N
```

### Зачем два канала в браузер

Браузер получает данные двумя каналами: HTTP и WebSocket
WebSocket для непрерывного обновления графиков
HTTP для истории замеров.
Без WebSоcket график обновляется только принудительно
БеЗ HTTP график пуст, истории замеров нет