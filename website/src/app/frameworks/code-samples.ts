const arduino = `#include <WiFi.h>
#include <ArduinoJson.h>
#include "DHT.h"

#define DHT_PIN 4

DHT dht(DHT_PIN, DHT11);

void setup() {
  Serial.begin(9600);
  dht.begin();

  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
}

void loop() {
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();

  // Send to cloud
  sendTelemetry(temperature, humidity);
  delay(30000);
}`;

const zephyr = `#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>

#define STACK_SIZE 1024
#define PRIORITY 7

K_THREAD_STACK_DEFINE(sensor_stack, STACK_SIZE);
struct k_thread sensor_thread;

void sensor_task(void *p1, void *p2, void *p3) {
    const struct device *sensor = DEVICE_DT_GET_ONE(bosch_bme280);

    while (1) {
        struct sensor_value temp, humidity;

        sensor_sample_fetch(sensor);
        sensor_channel_get(sensor, SENSOR_CHAN_AMBIENT_TEMP, &temp);
        sensor_channel_get(sensor, SENSOR_CHAN_HUMIDITY, &humidity);

        // Process sensor data
        process_telemetry(temp, humidity);

        k_sleep(K_SECONDS(30));
    }
}

int main(void) {
    k_thread_create(&sensor_thread, sensor_stack,
                    K_THREAD_STACK_SIZEOF(sensor_stack),
                    sensor_task, NULL, NULL, NULL,
                    PRIORITY, 0, K_NO_WAIT);
    return 0;
}`;

const espidf = `#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/i2c.h"

static const char *TAG = "GREENHOUSE";

void sensor_task(void *pvParameters) {
    while (1) {
        // Read I2C sensor
        uint8_t data[4];
        i2c_master_read_from_device(I2C_NUM_0, SENSOR_ADDR,
                                    data, sizeof(data),
                                    1000 / portTICK_PERIOD_MS);

        float temperature = parse_temperature(data);
        float humidity = parse_humidity(data);
        ESP_LOGI(TAG, "Temp: %.2f°C, Humidity: %.2f%%",
                 temperature, humidity);

        // Send to Azure IoT Hub
        send_telemetry_to_cloud(temperature, humidity);

        vTaskDelay(30000 / portTICK_PERIOD_MS);
    }
}

void app_main(void) {
    ESP_ERROR_CHECK(nvs_flash_init());

    // Create sensor task
    xTaskCreate(sensor_task, "sensor_task", 4096, NULL, 5, NULL);
}`;

export const samples = {
  arduino,
  zephyr,
  espidf,
};
