#include "korra_reboot.h"
#include "korra_wifi.h"

#if BOARD_HAS_WIFI

#include "esp_eap_client.h"

KorraWifi::KorraWifi() : _status(WL_IDLE_STATUS)
{
}

KorraWifi::~KorraWifi()
{
}

void KorraWifi::begin()
{
  // Set WiFi to station mode and disconnect from an AP if it was previously connected.
  WiFi.mode(WIFI_STA);

  // Clear network stack https://forum.arduino.cc/t/mqtt-with-esp32-gives-timeout-exceeded-disconnecting/688723/5
  WiFi.disconnect();

#if !WIFI_SKIP_LIST_NETWORKS
  listNetworks();
#endif
  connect(true);
}

void KorraWifi::maintain()
{
  connect(false);
}

void KorraWifi::printMacAddress(uint8_t mac[])
{
  for (int i = 5; i >= 0; i--)
  {
    if (mac[i] < 16)
    {
      Serial.print(F("0"));
    }
    Serial.print(mac[i], HEX);
    if (i > 0)
    {
      Serial.print(F(":"));
    }
  }
}

const __FlashStringHelper *encryptionTypeToString(wifi_auth_mode_t mode)
{
  switch (mode)
  {
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

#if CONFIG_WIFI_SCAN_NETWORKS
void KorraWifi::listNetworks()
{
  Serial.println(F("** Scan Networks **"));
  int8_t count = WiFi.scanNetworks();
  if (count == -1)
  {
    Serial.println(F("Couldn't scan for WiFi networks"));
    return; // nothing more to do here
  }

  // 1) Print header
  Serial.println();
  Serial.println(F("Id  | SSID                             | Signal (dBm) | Ch | Encryption"));
  Serial.println(F("----+----------------------------------+--------------+----+------------"));

  // 2) For each network, build a fixed-width line
  for (int8_t i = 0; i < count; i++)
  {
    // Build into a buffer with fixed column widths:
    //   • Index:      width  3, right-aligned
    //   • SSID:       width 32, left-aligned (crop or pad with spaces if shorter)
    //   • Signal:     width  5, right-aligned (the "(dBm)" is in header)
    //   • Channel:    width  2, right-aligned
    //   • Encryption: width 10, left-aligned
    // 3 + 32 + 5 + spaces + 2 + 10 + (4 * 3 separators) + 1 EOF byte ≈ 64 – 72 bytes total
    char lineBuf[80];
    snprintf(
        lineBuf, sizeof(lineBuf),
        "%3d | %-32.32s | %5d        | %2d | %-10.10s",
        i,
        WiFi.SSID(i).c_str(), // Use c_str() to get a const char* from String
        WiFi.RSSI(i),
        WiFi.channel(i),
        // Use reinterpret_cast to convert from __FlashStringHelper* to const char*
        reinterpret_cast<const char *>(
            encryptionTypeToString(WiFi.encryptionType(i))));

    Serial.println(lineBuf);
  }
  Serial.println();

  // Delete the scan result to free memory for code below.
  WiFi.scanDelete();
}
#endif

void KorraWifi::connect(bool initial)
{
  // if already connected, there is nothing more to do
  uint8_t status = WiFi.status();
  if (status == WL_CONNECTED)
  {
    return;
  }

  // detect disconnection
  if (_status == WL_CONNECTED && _status != status)
  {
    Serial.println(F("WiFi disconnected"));
  }

  Serial.print(F("Attempting to connect to WiFi SSID: "));
  Serial.println(CONFIG_WIFI_SSID);

#ifdef CONFIG_WIFI_PASSPHRASE
  status = WiFi.begin(CONFIG_WIFI_SSID, CONFIG_WIFI_PASSPHRASE);
#else

#ifdef CONFIG_WIFI_ENTERPRISE_USERNAME
  esp_eap_client_set_identity((uint8_t *)"", 0);
  esp_eap_client_set_username((uint8_t *)CONFIG_WIFI_ENTERPRISE_USERNAME, strlen(CONFIG_WIFI_ENTERPRISE_USERNAME));
  esp_eap_client_set_password((uint8_t *)CONFIG_WIFI_ENTERPRISE_PASSWORD, strlen(CONFIG_WIFI_ENTERPRISE_PASSWORD));
  esp_wifi_sta_enterprise_enable();
#endif

  status = WiFi.begin(CONFIG_WIFI_SSID);
#endif

  uint32_t started = millis();
  while (status != WL_CONNECTED)
  {
    // Timeout reached – perform a reset
    if ((millis() - started) > CONFIG_WIFI_CONNECTION_REBOOT_TIMEOUT_MILLIS)
    {
      Serial.println(F(" taken too long. Rebooting ...."));
      reboot();
    }

    delay(500);
    Serial.print(F("."));
    status = WiFi.status();
  }
  _status = WL_CONNECTED;

  Serial.println();
  Serial.println(F("WiFi connected successfully!"));
  Serial.print(F("SSID: "));
  Serial.println(WiFi.SSID());

  uint8_t mac[MAC_ADDRESS_LENGTH];
  WiFi.BSSID(mac);
  Serial.print(F("BSSID: "));
  printMacAddress(mac);
  Serial.println();

  Serial.print(F("Signal strength (RSSI): "));
  Serial.print(WiFi.RSSI());
  Serial.println(F(" dBm"));

  Serial.print(F("IP Address: "));
  Serial.println(WiFi.localIP());

  WiFi.macAddress(_macAddress);
  Serial.print(F("MAC address: "));
  printMacAddress(_macAddress);
  Serial.println();

  // For the first time, set the hostname using the mac address.
  // Change the hostname to a more useful name. E.g. a default value like "esp32s3-594E40" changes to "korra-594E40"
  // The WiFi stack needs to have been activated by scanning or connecting hence why this is done last. Otherwise just zeros.
  if (initial)
  {
    snprintf(_hostname, sizeof(_hostname), "korra-%02X%02X%02X", _macAddress[3], _macAddress[4], _macAddress[5]);
    WiFi.setHostname(_hostname);
    Serial.print("Set hostname to ");
    Serial.println(_hostname);
  }
}

#endif // BOARD_HAS_WIFI
