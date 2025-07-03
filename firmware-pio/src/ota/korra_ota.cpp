#include <Arduino.h>
#include <sys/reboot.h>

#include "korra_ota.h"

/**
 * This file borrows heavily from
 * https://github.com/espressif/arduino-esp32/blob/ac961f671abd5ae1da0a15fd4bee71ed807c2cf3/libraries/Update/src/HttpsOTAUpdate.cpp
 * https://github.com/espressif/arduino-esp32/blob/ac961f671abd5ae1da0a15fd4bee71ed807c2cf3/libraries/Update/src/HttpsOTAUpdate.h
 *
 * However, we cannot use it directly because the HTTP headers are large when pulling from GitHub releases and we need
 * set bigger values for `buffer_size_tx` and `buffer_size` in `esp_http_client_config_t`.
 */

#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/task.h>

#include "esp32-hal-log.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"

#define OTA_TASK_STACK_SIZE 9216

static esp_http_client_config_t config;
static EventGroupHandle_t ota_status = NULL; // check for ota status
static EventBits_t set_bit;

static const int OTA_IDLE_BIT = BIT0;
static const int OTA_UPDATING_BIT = BIT1;
static const int OTA_SUCCESS_BIT = BIT2;
static const int OTA_FAIL_BIT = BIT3;

esp_err_t http_event_handler(esp_http_client_event_t *event) {
  // switch (event->event_id) {
  // case esp_http_client_event_id_t::HTTP_EVENT_ERROR:
  //   Serial.println("Http Event Error");
  //   break;
  // case esp_http_client_event_id_t::HTTP_EVENT_ON_CONNECTED:
  //   Serial.println("Http Event On Connected");
  //   break;
  // case esp_http_client_event_id_t::HTTP_EVENT_HEADER_SENT:
  //   Serial.println("Http Event Header Sent");
  //   break;
  // case esp_http_client_event_id_t::HTTP_EVENT_ON_HEADER:
  //   Serial.printf("Http Event On Header, key=%s, value=%s\n", event->header_key, event->header_value);
  //   break;
  // case esp_http_client_event_id_t::HTTP_EVENT_ON_DATA:
  //   break;
  // case esp_http_client_event_id_t::HTTP_EVENT_ON_FINISH:
  //   Serial.println("Http Event On Finish");
  //   break;
  // case esp_http_client_event_id_t::HTTP_EVENT_DISCONNECTED:
  //   Serial.println("Http Event Disconnected");
  //   break;
  // }
  return ESP_OK;
}

void https_ota_task(void *param) {
  if (ota_status) {
    xEventGroupSetBits(ota_status, OTA_UPDATING_BIT);
    xEventGroupClearBits(ota_status, OTA_IDLE_BIT);
  }
  esp_https_ota_config_t cfg;
  cfg.http_config = (const esp_http_client_config_t *)param;
  cfg.http_client_init_cb = NULL;
  cfg.bulk_flash_erase = false;      // Erase entire flash partition during initialization
  cfg.partial_http_download = false; // Enable Firmware image to be downloaded over multiple HTTP requests
  cfg.max_http_request_size = 0;     // Maximum request size for partial HTTP download

  esp_err_t ret = esp_https_ota((const esp_https_ota_config_t *)&cfg);
  if (ret == ESP_OK) {
    if (ota_status) {
      xEventGroupClearBits(ota_status, OTA_UPDATING_BIT);
      xEventGroupSetBits(ota_status, OTA_SUCCESS_BIT);
    }
  } else {
    if (ota_status) {
      xEventGroupClearBits(ota_status, OTA_UPDATING_BIT);
      xEventGroupSetBits(ota_status, OTA_FAIL_BIT);
    }
  }
  vTaskDelete(NULL);
}

KorraOta::KorraOta() {
}

KorraOta::~KorraOta() {
}

void KorraOta::begin(const char *ca_cert) {
  this->ca_cert = ca_cert;
}

void KorraOta::update(const struct korra_ota_info *value) {
  memcpy(&info, value, sizeof(struct korra_ota_info));
  Serial.println("Starting firmware update ...");
  Serial.printf("URL: %s\n", info.url);
  Serial.printf("Hash: %s\n", info.hash);
  Serial.printf("Signature: %s\n", info.signature);
  printed_fail = false;

  // TODO: figure out how to check hash and signature
  // https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/esp_https_ota.html#signature-verification

  config.url = info.url;
  config.cert_pem = ca_cert;
  config.skip_cert_common_name_check = true; // change to false if need be
  config.event_handler = http_event_handler;
  // config.buffer_size = 2048;
  config.buffer_size_tx = 2048;

  if (!ota_status) {
    ota_status = xEventGroupCreate();
    if (!ota_status) {
      log_e("OTA Event Group Create Failed");
    }
    xEventGroupSetBits(ota_status, OTA_IDLE_BIT);
  }

  if (xTaskCreate(&https_ota_task, "https_ota_task", OTA_TASK_STACK_SIZE, &config, 5, NULL) != pdPASS) {
    log_e("Couldn't create ota task\n");
  }
}

void KorraOta::maintain() {
  const enum https_ota_status status = current_status();
  if (status == https_ota_status::HTTPS_OTA_STATUS_SUCCESS) {
    Serial.println("Firmware written successfully. Rebooting in 5 sec ...");
    delay(5 * 1000);
    sys_reboot();
  } else if (status == https_ota_status::HTTPS_OTA_STATUS_FAIL) {
    if (!printed_fail) {
      Serial.println("Firmware upgrade failed");
      printed_fail = true;
    }
  }
}

void KorraOta::populate(const char *url, const char *hash, const char *signature, struct korra_ota_info *dest) {
  memcpy(dest->url, url, min((int)sizeof(dest->url), (int)strlen(url)));
  memcpy(dest->hash, hash, min((int)sizeof(dest->hash), (int)strlen(hash)));
  memcpy(dest->signature, signature, min((int)sizeof(dest->signature), (int)strlen(signature)));
}

const enum KorraOta::https_ota_status KorraOta::current_status() {
  if (ota_status) {
    set_bit = xEventGroupGetBits(ota_status);
    if (set_bit == OTA_IDLE_BIT) {
      return https_ota_status::HTTPS_OTA_STATUS_IDLE;
    }
    if (set_bit == OTA_UPDATING_BIT) {
      return https_ota_status::HTTPS_OTA_STATUS_UPDATING;
    }
    if (set_bit == OTA_SUCCESS_BIT) {
      return https_ota_status::HTTPS_OTA_STATUS_SUCCESS;
    }
    if (set_bit == OTA_FAIL_BIT) {
      return https_ota_status::HTTPS_OTA_STATUS_FAIL;
    }
  }
  return https_ota_status::HTTPS_OTA_STATUS_ERR;
}
