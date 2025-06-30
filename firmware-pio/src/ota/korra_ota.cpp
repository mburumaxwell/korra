#include "korra_ota.h"
#include "HttpsOTAUpdate.h"
#include <sys/reboot.h>

void on_http_event_callback(HttpEvent_t *event) {
  switch (event->event_id) {
  case HTTP_EVENT_ERROR:
    Serial.println("Http Event Error");
    break;
  case HTTP_EVENT_ON_CONNECTED:
    Serial.println("Http Event On Connected");
    break;
  case HTTP_EVENT_HEADER_SENT:
    Serial.println("Http Event Header Sent");
    break;
  case HTTP_EVENT_ON_HEADER:
    Serial.printf("Http Event On Header, key=%s, value=%s\n", event->header_key, event->header_value);
    break;
  case HTTP_EVENT_ON_DATA:
    break;
  case HTTP_EVENT_ON_FINISH:
    Serial.println("Http Event On Finish");
    break;
  case HTTP_EVENT_DISCONNECTED:
    Serial.println("Http Event Disconnected");
    break;
  }
}

KorraOta::KorraOta(Client &client) {
}

KorraOta::~KorraOta() {
}

void KorraOta::begin(const char *ca_cert) {
  this->ca_cert - ca_cert;
}

void KorraOta::update(const char *url, const char *hash, const char *signature) {
  Serial.println("Starting firmware update ...");
  Serial.printf("URL: %s\n", url);
  Serial.printf("Hash: %s\n", hash);
  Serial.printf("Signature: %s\n", signature);

  // TODO: figure out how to check hash and signature
  // https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/esp_https_ota.html#signature-verification

  HttpsOTA.onHttpEvent(on_http_event_callback);
  HttpsOTA.begin(url, /* cert_pem */ ca_cert, /* skip_cert_common_name_check */ false);
}

void KorraOta::maintain() {
  HttpsOTAStatus_t status = HttpsOTA.status();
  if (status == HTTPS_OTA_SUCCESS) {
    Serial.println("Firmware written successfully. Rebooting in 5 sec ...");
    delay(5 * 1000);
    sys_reboot();
  } else if (status == HTTPS_OTA_FAIL) {
    Serial.println("Firmware upgrade failed");
  }
}
