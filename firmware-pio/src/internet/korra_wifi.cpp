#include <ArduinoJson.h>

#include "korra_wifi.h"

#ifdef CONFIG_BOARD_HAS_WIFI

#define PREFERENCES_KEY_WIFI_CREDS "wifi-creds"

static void on_wifi_event_callback_korra(WiFiEvent_t event, WiFiEventInfo_t info) {
  KorraWiFi::instance()->on_wifi_event(event, info);
}

static int mask_to_buffer(const char *pw, size_t visible, char *dest, size_t destsize) {
  size_t len = strlen(pw);
  size_t show = visible < len ? visible : len;
  size_t total = len; // output length (no hidden length leak!)

  // ensure room for all chars + NULL
  if (destsize < total + 1) return -1;

  memcpy(dest, pw, show);               // copy visible prefix
  memset(dest + show, '*', len - show); // fill the rest with '*'

  dest[total] = '\0';
  return 0;
}

KorraWiFi *KorraWiFi::_instance = nullptr;

KorraWiFi::KorraWiFi(Preferences &prefs) : prefs(prefs) {
  _instance = this;
}

KorraWiFi::~KorraWiFi() {
  _instance = nullptr;
}

void KorraWiFi::begin() {
  // Set WiFi to station mode and disconnect from an AP if it was previously connected.
  WiFi.mode(WIFI_STA);

  // Clear network stack https://forum.arduino.cc/t/mqtt-with-esp32-gives-timeout-exceeded-disconnecting/688723/5
  WiFi.disconnect();

#ifdef CONFIG_WIFI_SCAN_NETWORKS
  listNetworks();
#endif // CONFIG_WIFI_SCAN_NETWORKS

  WiFi.setAutoReconnect(true);
  WiFi.onEvent(on_wifi_event_callback_korra);
  credentials_load(); // load the credentials
}

void KorraWiFi::maintain() {
  connect();
}

bool KorraWiFi::credentials_save(const struct wifi_credentials *credentials) {
  JsonDocument doc;

  doc["ssid"] = credentials->ssid;
  if (strlen(credentials->passphrase) > 0) doc["passphrase"] = credentials->passphrase;
  if (strlen(credentials->eap_identity) > 0) doc["eap_identity"] = credentials->eap_identity;
  if (strlen(credentials->eap_username) > 0) doc["eap_username"] = credentials->eap_username;
  if (strlen(credentials->eap_password) > 0) doc["eap_password"] = credentials->eap_password;

  // serialize json and save
  size_t payload_len = measureJson(doc);
  char payload[payload_len + 1] = {0};
  payload_len = serializeJson(doc, payload, payload_len);
  prefs.putBytes(PREFERENCES_KEY_WIFI_CREDS, payload, payload_len);

  // copy the values into memory
  memcpy(&(this->credentials), credentials, sizeof(struct wifi_credentials));

  // disconnect
  WiFi.disconnect();
  connection_requested = false;
  logged_missing_creds = false;

  return true;
}

bool KorraWiFi::credentials_clear() {
  if (prefs.isKey(PREFERENCES_KEY_WIFI_CREDS)) prefs.remove(PREFERENCES_KEY_WIFI_CREDS);

  // clear the values from memory
  this->credentials = {0};

  // disconnect
  WiFi.disconnect();
  connection_requested = false;

  return true;
}

void KorraWiFi::credentials_load() {
  if (prefs.isKey(PREFERENCES_KEY_WIFI_CREDS)) {
    // read from prefs
    const size_t payload_len = prefs.getBytesLength(PREFERENCES_KEY_WIFI_CREDS);
    char payload[payload_len + 1];
    prefs.getBytes(PREFERENCES_KEY_WIFI_CREDS, payload, payload_len);
    // Serial.printf("Read %d bytes from %s key:\n%s\n", payload_len, PREFERENCES_KEY_WIFI_CREDS, payload);

    // parse JSON
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload);
    if (error) {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
      return;
    }

    // ssid
    const char *ssid_raw = doc["ssid"];
    if (ssid_raw != NULL) {
      size_t ssid_raw_len = min((int)strlen(ssid_raw) + 1, (int)sizeof(credentials.ssid));
      memcpy(credentials.ssid, ssid_raw, ssid_raw_len - 1);
    }

    // passphrase
    const char *passphrase_raw = doc["passphrase"];
    if (passphrase_raw != NULL) {
      size_t passphrase_raw_len = min((int)strlen(passphrase_raw) + 1, (int)sizeof(credentials.passphrase));
      memcpy(credentials.passphrase, passphrase_raw, passphrase_raw_len - 1);
    }

    // eap_identity
    const char *eap_identity_raw = doc["eap_identity"];
    if (eap_identity_raw != NULL) {
      size_t eap_identity_raw_len = min((int)strlen(eap_identity_raw) + 1, (int)sizeof(credentials.eap_identity));
      memcpy(credentials.eap_identity, eap_identity_raw, eap_identity_raw_len - 1);
    }

    // eap_username
    const char *eap_username_raw = doc["eap_username"];
    if (eap_username_raw != NULL) {
      size_t eap_username_raw_len = min((int)strlen(eap_username_raw) + 1, (int)sizeof(credentials.eap_username));
      memcpy(credentials.eap_username, eap_username_raw, eap_username_raw_len - 1);
    }

    // eap_password
    const char *eap_password_raw = doc["eap_password"];
    if (eap_password_raw != NULL) {
      size_t eap_password_raw_len = min((int)strlen(eap_password_raw) + 1, (int)sizeof(credentials.eap_password));
      memcpy(credentials.eap_password, eap_password_raw, eap_password_raw_len - 1);
    }
  }

  credentials_print();
}

void KorraWiFi::credentials_print() {
  Serial.printf("WiFi Credentials SSID: '%s'\n", credentials.ssid);

  size_t passphrase_len = strlen(credentials.passphrase);
  char protected_passphrase[sizeof(credentials.passphrase)];
  mask_to_buffer(credentials.passphrase, 0.3 * passphrase_len, protected_passphrase, sizeof(protected_passphrase));
  Serial.printf("WiFi Credentials Passphrase: '%s'\n", passphrase_len > 0 ? protected_passphrase : "<unset>");

  size_t eap_identity_len = strlen(credentials.eap_identity);
  char protected_eap_identity[sizeof(credentials.eap_identity)];
  mask_to_buffer(credentials.eap_identity, 0.4 * eap_identity_len, protected_eap_identity,
                 sizeof(protected_eap_identity));
  Serial.printf("WiFi Credentials EAP Identity: '%s'\n", eap_identity_len > 0 ? protected_eap_identity : "<unset>");

  size_t eap_username_len = strlen(credentials.eap_username);
  char protected_eap_username[sizeof(credentials.eap_username)];
  mask_to_buffer(credentials.eap_username, 0.4 * eap_username_len, protected_eap_username,
                 sizeof(protected_eap_username));
  Serial.printf("WiFi Credentials EAP Username: '%s'\n", eap_username_len > 0 ? protected_eap_username : "<unset>");

  size_t eap_password_len = strlen(credentials.eap_password);
  char protected_eap_password[sizeof(credentials.eap_password)];
  mask_to_buffer(credentials.eap_password, 0.4 * eap_password_len, protected_eap_password,
                 sizeof(protected_eap_password));
  Serial.printf("WiFi Credentials EAP Password: '%s'\n", eap_password_len > 0 ? protected_eap_password : "<unset>");
}

const __FlashStringHelper *encryptionTypeToString(wifi_auth_mode_t mode) {
  switch (mode) {
  case WIFI_AUTH_OPEN:
    return F("open");
  case WIFI_AUTH_WEP:
    return F("WEP");
  case WIFI_AUTH_WPA_PSK:
    return F("WPA");
  case WIFI_AUTH_WPA2_PSK:
    return F("WPA2");
  case WIFI_AUTH_WPA_WPA2_PSK:
    return F("WPA+WPA2");
  case WIFI_AUTH_WPA2_ENTERPRISE:
    return F("WPA2-EAP");
  case WIFI_AUTH_WPA3_PSK:
    return F("WPA3");
  case WIFI_AUTH_WPA2_WPA3_PSK:
    return F("WPA2+WPA3");
  case WIFI_AUTH_WAPI_PSK:
    return F("WAPI");
  default:
    return F("unknown");
  }
}

#ifdef CONFIG_WIFI_SCAN_NETWORKS
void KorraWiFi::listNetworks() {
  Serial.println(F("** Scan Networks **"));
  int8_t count = WiFi.scanNetworks();
  if (count == -1) {
    Serial.println(F("Couldn't scan for WiFi networks"));
    return; // nothing more to do here
  }

  // 1) Print header
  Serial.println();
  Serial.println(F("Id  | SSID                             | Ch | RSSI | Security   | BSSID             "));
  Serial.println(F("----+----------------------------------+----+------+------------+-------------------"));

  // 2) For each network, build a fixed-width line
  for (int8_t i = 0; i < count; i++) {
    // Build into a buffer with fixed column widths:
    //   • Index:      width  3, right-aligned
    //   • SSID:       width 32, left-aligned (crop or pad with spaces if shorter)
    //   • RSSI:       width  4, right-aligned
    //   • Channel:    width  2, right-aligned
    //   • Security:   width 10, left-aligned (crop or pad with spaces if shorter)
    //   • BSSID:      width 17, left-aligned

    Serial.printf("%3d | %-32.32s | %2d | %4d | %-10.10s | %-17s \n", i,
                  WiFi.SSID(i).c_str(), // Use c_str() to get a const char* from String
                  WiFi.channel(i), WiFi.RSSI(i),
                  // Use reinterpret_cast to convert from __FlashStringHelper* to const char*
                  reinterpret_cast<const char *>(encryptionTypeToString(WiFi.encryptionType(i))),
                  // Use c_str() to get a const char* from String
                  WiFi.BSSIDstr(i).c_str());
  }
  Serial.println();

  // Delete the scan result to free memory for code below.
  WiFi.scanDelete();
}
#endif

void KorraWiFi::connect() {
  // if already connected, or we requested it, there is nothing more to do
  if (connected() || connection_requested) return;

  // ensure we have credentials
  if (strlen(credentials.ssid) == 0) {
    if (!logged_missing_creds) {
      Serial.println("WiFi credentials are not set. Use the shell e.g. \"wifi credentials set ....\"");
      logged_missing_creds = true;
    }
    return;
  }

  if (strlen(credentials.eap_identity) > 0 || strlen(credentials.eap_username) > 0 ||
      strlen(credentials.eap_password) > 0) {
    Serial.printf("Attempting to connect to WiFi (EAP) SSID: '%s'\n", credentials.ssid);
    WiFi.begin(/* wpa2_ssid */ (const char *)credentials.ssid,
               /* method */ wpa2_auth_method_t::WPA2_AUTH_PEAP,
               /* wpa2_identity */ (const char *)credentials.eap_identity,
               /* wpa2_username */ (const char *)credentials.eap_username,
               /* wpa2_password */ (const char *)credentials.eap_password);
  } else {
    if (strlen(credentials.passphrase) > 0) {
      Serial.printf("Attempting to connect to WiFi (Personal) SSID: '%s'\n", credentials.ssid);
      WiFi.begin(credentials.ssid, credentials.passphrase);
    } else {
      Serial.printf("Attempting to connect to WiFi (Open) SSID: '%s'\n", credentials.ssid);
      WiFi.begin(credentials.ssid, NULL);
    }
  }

  connection_requested = true;
}

void KorraWiFi::on_wifi_event(WiFiEvent_t event, WiFiEventInfo_t info) {
  if (event == WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_GOT_IP) {
    Serial.println();
    Serial.println(F("WiFi connected successfully!"));
    Serial.printf("SSID: %s\n", WiFi.SSID().c_str());
    Serial.printf("BSSID: %s\n", WiFi.BSSIDstr().c_str());
    Serial.printf("Channel: %d\n", WiFi.channel());
    // Serial.printf("Security: %s\n", encryptionTypeToString(WiFi.encryptionType()));
    Serial.printf("RSSI: %d\n", WiFi.RSSI());
    Serial.printf("IP Address: %s\n", WiFi.localIP().toString());

    // populate net_props
    snprintf(net_props.kind, sizeof(net_props.kind), KORRA_NETWORK_KIND_WIFI);
    WiFi.macAddress(net_props.mac_addr);
    snprintf(net_props.mac, sizeof(net_props.mac), WiFi.macAddress().c_str());
    snprintf(net_props.network, sizeof(net_props.network), WiFi.SSID().c_str());
    net_props.local_ipaddr = WiFi.localIP();
    snprintf(net_props.local_ip, sizeof(net_props.local_ip), WiFi.localIP().toString().c_str());

    // For the first time, set the hostname using the mac address.
    // Change the hostname to a more useful name. E.g. a default value like "esp32s3-594E40" changes to "korra-594E40"
    // The WiFi stack needs to have been activated by scanning or connecting hence why this is done last.
    // Otherwise just zeros.
    if (strlen(net_props.hostname) == 0) {
      snprintf(net_props.hostname, sizeof(net_props.hostname), "korra-" FMT_LL_ADDR_6_LOWER_NO_COLONS,
               PRINT_LL_ADDR_6(net_props.mac_addr));
      WiFi.setHostname(net_props.hostname);
      Serial.printf("Set hostname to %s\n", net_props.hostname);
    }
  } else if (event == WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_LOST_IP) {
    Serial.println(F("WiFi disconnected"));
  } else {
    // Serial.printf("[WiFi-event] event: %d\n", event);
  }
}

#endif // BOARD_HAS_WIFI
