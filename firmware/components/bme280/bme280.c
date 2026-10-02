#include <stdint.h>
#include "esp_log.h"
#include "bme280.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <inttypes.h>

#define BME280_OSRS_P_X1 0b001
#define BME280_OSRS_T_X1 0b001
#define BME280_OSRS_H_X1 0b001
#define BME280_MODE_FORCED 0b01
#define BME280_REG_CHIP_ID 0xD0
#define BME280_REG_CALIB_1 0x88
#define BME280_REG_CALIB_2 0xE1
#define BME280_REG_CTRL_HUM 0xF2
#define BME280_REG_STATUS 0xF3
#define BME280_REG_CTRL_MEAS 0xF4
#define BME280_REG_DATA 0xF7
#define BME280_REG_RESET 0xE0
#define BME280_CHIP_ID 0x60
#define BME280_I2C_ADDR 0x76
#define BME280_STATUS_RETRIES 10
static const char *TAG = "bme280";
static uint16_t dig_T1, dig_P1;
static int16_t dig_T2, dig_T3, dig_P2,dig_P3,dig_P4,dig_P5,dig_P6,dig_P7,dig_P8,dig_P9, dig_H2, dig_H4, dig_H5;
static uint8_t dig_H1, dig_H3;
static int8_t dig_H6;
static  int32_t t_fine;
static  i2c_master_dev_handle_t s_dev_handle;

static int32_t BME280_compensate_T_int32(int32_t adc_T) {
        int32_t var1, var2, T;
        var1 = ((((adc_T>>3) - ((int32_t)dig_T1<<1))) * ((int32_t)dig_T2)) >> 11;
        var2 = (((((adc_T>>4) - ((int32_t)dig_T1)) * ((adc_T>>4) - ((int32_t)dig_T1))) >> 12) * ((int32_t)dig_T3)) >> 14;
        t_fine = var1 + var2;
        T = (t_fine * 5 + 128) >> 8; 
        return T; 
}
static uint32_t  BME280_compensate_P_int64(int32_t adc_P) { 
    int64_t var1, var2, p; var1 = ((int64_t)t_fine) - 128000; var2 = var1 * var1 * (int64_t)dig_P6;
    var2 = var2 + ((var1*(int64_t)dig_P5)<<17);
     var2 = var2 + (((int64_t)dig_P4)<<35);
      var1 = ((var1 * var1 * (int64_t)dig_P3)>>8) + ((var1 * (int64_t)dig_P2)<<12);
       var1 = (((((int64_t)1)<<47)+var1))*((int64_t)dig_P1)>>33;
        if (var1 == 0) { 
            return 0; // avoid exception caused by division by zero 
        } 
        p = 1048576-adc_P; 
        p = (((p<<31)-var2)*3125)/var1; 
        var1 = (((int64_t)dig_P9) * (p>>13) * (p>>13)) >> 25; var2 = (((int64_t)dig_P8) * p) >> 19;
        p = ((p + var1 + var2) >> 8) + (((int64_t)dig_P7)<<4); return (uint32_t)p; 
}
static int32_t  bme280_compensate_H_int32(int32_t adc_H) {
    int32_t v_x1_u32r;
    v_x1_u32r = (t_fine - ((int32_t)76800));
    v_x1_u32r = (((((adc_H << 14) - (((int32_t)dig_H4) << 20) - (((int32_t)dig_H5) * v_x1_u32r)) + 
                ((int32_t)16384)) >> 15) * (((((((v_x1_u32r * ((int32_t)dig_H6)) >> 10) * 
                (((v_x1_u32r * ((int32_t)dig_H3)) >> 11) + ((int32_t)32768))) >> 10) + 
                ((int32_t)2097152)) * ((int32_t)dig_H2) + 8192) >> 14));
    v_x1_u32r = (v_x1_u32r - (((((v_x1_u32r >> 15) * (v_x1_u32r >> 15)) >> 7) * ((int32_t)dig_H1)) >> 4));
    v_x1_u32r = (v_x1_u32r < 0 ? 0 : v_x1_u32r);
    v_x1_u32r = (v_x1_u32r > 419430400 ? 419430400 : v_x1_u32r);
    return (int32_t)(v_x1_u32r >> 12);
}

static uint16_t u16_le(const uint8_t *p){
   uint16_t dig =  ((uint16_t)p[1] << 8) | p[0];
    return dig;
}
static int16_t  s16_le(const uint8_t *p){
    return (int16_t)u16_le(p);
}

esp_err_t bme280_init(i2c_master_bus_handle_t bus_handle){
    i2c_device_config_t dev_config = {
    .dev_addr_length = I2C_ADDR_BIT_LEN_7,
    .device_address = BME280_I2C_ADDR,
    .scl_speed_hz = 100000,
    };
    esp_err_t bus_adder = i2c_master_bus_add_device( bus_handle,&dev_config, &s_dev_handle);
    if (bus_adder != ESP_OK)
    {
        ESP_LOGE(TAG, "Bus Adder  failed: %s (0x%X)", esp_err_to_name(bus_adder), bus_adder);
        return bus_adder;
    }

    uint8_t reg_chip = BME280_REG_CHIP_ID;
    uint8_t chip_id;
    esp_err_t  res = i2c_master_transmit_receive(s_dev_handle, &reg_chip, 1, &chip_id, 1, 100);
    if (res != ESP_OK){
        ESP_LOGE(TAG,"Chip ID read failed = %s ",esp_err_to_name(res));
        return  res;
        }
    if (chip_id != BME280_CHIP_ID){
        ESP_LOGE(TAG,"ESP_ERR_INVALID_RESPONSE =  %02x ",chip_id);
        return ESP_ERR_INVALID_RESPONSE;
    }
    ESP_LOGI(TAG," CHIP ID =  %02x ",chip_id);
    uint8_t buffer[2] = {BME280_REG_CTRL_HUM, BME280_OSRS_H_X1};
    esp_err_t hum_res = i2c_master_transmit(s_dev_handle,buffer,2,500);
     if (hum_res ==ESP_OK){
            ESP_LOGI(TAG,"hum_res =  %02x ", hum_res);
        }
    else{
        ESP_LOGE(TAG,"ERROR Write hum on  0xF2");
        return hum_res;
    } 
    uint8_t calib[26];
    uint8_t calib2[7];
    uint8_t reg = BME280_REG_CALIB_1;
    uint8_t reg2 = BME280_REG_CALIB_2;
    esp_err_t  calib_res = i2c_master_transmit_receive(s_dev_handle, &reg, 1, calib, 26, 100);
    if (calib_res != ESP_OK)
    {
        ESP_LOGE(TAG, "Calibration block1 read failed: %s (0x%X)", esp_err_to_name(calib_res), calib_res);
        return calib_res;
    }
    esp_err_t  calib_res2 = i2c_master_transmit_receive(s_dev_handle, &reg2, 1, calib2, 7, 100);
    if (calib_res2 != ESP_OK)
    {
        ESP_LOGE(TAG, "Calibration block2 read failed: %s (0x%X)", esp_err_to_name(calib_res2), calib_res2);
        return calib_res2;
    }
    dig_T1 = u16_le(&calib[0]);
    dig_T2 = s16_le(&calib[2]);
    dig_T3 = s16_le(&calib[4]);
    dig_P1 = u16_le(&calib[6]);
    dig_P2 =  s16_le(&calib[8]);
    dig_P3 =  s16_le(&calib[10]);
    dig_P4 =  s16_le(&calib[12]);
    dig_P5 =  s16_le(&calib[14]);
    dig_P6 = s16_le(&calib[16]);
    dig_P7 =  s16_le(&calib[18]);
    dig_P8 =  s16_le(&calib[20]);
    dig_P9 =  s16_le(&calib[22]);
    dig_H1 = (uint8_t)(calib[25]);
    dig_H2 = (int16_t)(((uint16_t)calib2[1] << 8) | calib2[0]);
    dig_H3 = (uint8_t)(calib2[2]);
    dig_H4 =(int16_t)( ((int8_t)calib2[3] *16 ) | (calib2[4] & 0b00001111));
    dig_H5= (int16_t)(((int8_t)calib2[5]*16)|(calib2[4]>>4));
    dig_H6 = (int8_t)(calib2[6]);
    return ESP_OK;
}

esp_err_t bme280_read(float *humidity, float *temperature,float *pressure){
    uint8_t ctrl_meas = (BME280_OSRS_T_X1<<5)|(BME280_OSRS_P_X1<<2) | BME280_MODE_FORCED;
    uint8_t buffer[2] = {BME280_REG_CTRL_MEAS,ctrl_meas };
    esp_err_t meas_res = i2c_master_transmit(s_dev_handle,buffer,2,500);
    if (meas_res != ESP_OK)
    {
        ESP_LOGE(TAG, "Writing  Meas_Res failed: %s (0x%X)", esp_err_to_name(meas_res), meas_res);
        return meas_res;
    }
    uint8_t reg = BME280_REG_STATUS;
    uint8_t status;
    uint8_t i = BME280_STATUS_RETRIES;
    while( i > 0){
        vTaskDelay(pdMS_TO_TICKS(10));
         esp_err_t  res = i2c_master_transmit_receive(s_dev_handle, &reg, 1, &status, 1, 100);
            if (res != ESP_OK) {
            ESP_LOGE(TAG, "I2C read status register failed: %s", esp_err_to_name(res));
            return res; 
        }
            if ((status & (1 << 3))!= 0){
                ESP_LOGI(TAG, "BME280 is measuring...");
            }
            if ((status & (1 << 3)) == 0){
                break;
            }
            if (i == 1){
                ESP_LOGE(TAG, "Attempts to read the status have ended.");
                return ESP_ERR_TIMEOUT;
            }
            i--;
    }
    uint8_t data_reg = BME280_REG_DATA;
    uint8_t data[8];
    esp_err_t  data_res = i2c_master_transmit_receive(s_dev_handle, &data_reg, 1, data, 8, 100);
    if (data_res==ESP_OK){
            ESP_LOG_BUFFER_HEX(TAG, data, 8);
            int32_t raw_t= ((int32_t)data[3]<<12) |((int32_t)data[4]<<4 )|(data[5]>>4);
            int32_t raw_p= ((int32_t)data[0]<<12) |((int32_t)data[1]<<4 )|(data[2]>>4);
            uint16_t raw_h = ((uint16_t)data[6]<<8) |((uint16_t)data[7] );
            ESP_LOGI(TAG,"RAW_T = %" PRId32,raw_t);
            ESP_LOGI(TAG,"RAW_P = %" PRId32,raw_p);
            ESP_LOGI(TAG,"RAW_H = %" PRIu16,raw_h);
            int32_t temperature_compensate = BME280_compensate_T_int32(raw_t);
            uint32_t humidity_compensate = bme280_compensate_H_int32(raw_h);
            uint32_t pressure_compensate = BME280_compensate_P_int64(raw_p);
            *temperature = temperature_compensate/ 100.0f; // Сelius 12.45
            *humidity = humidity_compensate/ 1024.0f; // %
            *pressure = pressure_compensate/ 25600.0f; // hPa GECTOPASCAL
            ESP_LOGI(TAG, "t_fine = %" PRId32, t_fine);
            ESP_LOGI(TAG, "Temperature: %.2f C", (float)*temperature);
            ESP_LOGI(TAG, "Humidity: %.2f Rh", (float)*humidity);
            ESP_LOGI(TAG, "Pressure: %.2f  hPa", (float)*pressure);
        }
    else{
        ESP_LOGE(TAG,"ERROR READ MEANSURE REGISTOR");
        return data_res;
    }
   return ESP_OK;
}