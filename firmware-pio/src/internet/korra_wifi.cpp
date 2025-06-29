#include <sys/reboot.h>

#include "korra_wifi.h"

#ifdef CONFIG_BOARD_HAS_WIFI

#define PREFERENCES_KEY_WIFI_SSID "wifi-ssid"
#define PREFERENCES_KEY_WIFI_PASSPHRASE "wifi-passphrase"
#define PREFERENCES_KEY_WIFI_EAP_IDENTITY "wifi-eap-identity"
#define PREFERENCES_KEY_WIFI_EAP_USERNAME "wifi-eap-username"
#define PREFERENCES_KEY_WIFI_EAP_PASSWORD "wifi-eap-password"

KorraWiFi::KorraWiFi(Preferences &prefs) : prefs(prefs), _status(WL_IDLE_STATUS) {}

KorraWiFi::~KorraWiFi() {}

void KorraWiFi::begin()
{
  // Set WiFi to station mode and disconnect from an AP if it was previously connected.
  WiFi.mode(WIFI_STA);

  // Clear network stack https://forum.arduino.cc/t/mqtt-with-esp32-gives-timeout-exceeded-disconnecting/688723/5
  WiFi.disconnect();

#ifdef CONFIG_WIFI_SCAN_NETWORKS
  listNetworks();
#endif // CONFIG_WIFI_SCAN_NETWORKS

  // load the credentials
  credentials_load();

  // connect for the first time
  connect(true);
}

void KorraWiFi::maintain()
{
  connect(false);
}

bool KorraWiFi::credentials_save(const struct wifi_credentials *credentials, bool disconnect)
{
  prefs.putBytes(PREFERENCES_KEY_WIFI_SSID,
                 credentials->ssid, strlen(credentials->ssid));

  if (credentials->passphrase != NULL)
  {
    prefs.putBytes(PREFERENCES_KEY_WIFI_PASSPHRASE,
                   credentials->passphrase, strlen(credentials->passphrase));
  }

  if (credentials->eap_identity != NULL)
  {
    prefs.putBytes(PREFERENCES_KEY_WIFI_EAP_IDENTITY,
                   credentials->eap_identity, strlen(credentials->eap_identity));
  }

  if (credentials->eap_username != NULL)
  {
    prefs.putBytes(PREFERENCES_KEY_WIFI_EAP_USERNAME,
                   credentials->eap_username, strlen(credentials->eap_username));
  }

  if (credentials->eap_password != NULL)
  {
    prefs.putBytes(PREFERENCES_KEY_WIFI_EAP_PASSWORD,
                   credentials->eap_password, strlen(credentials->eap_password));
  }

  // copy the value into memory
  memcpy(&(this->credentials), credentials, sizeof(struct wifi_credentials));

  // disconnect if told to
  if (disconnect)
  {
    WiFi.disconnect();
  }

  return true;
}

bool KorraWiFi::credentials_clear()
{
  if (prefs.isKey(PREFERENCES_KEY_WIFI_SSID))
  {
    prefs.remove(PREFERENCES_KEY_WIFI_SSID);
  }
  if (prefs.isKey(PREFERENCES_KEY_WIFI_PASSPHRASE))
  {
    prefs.remove(PREFERENCES_KEY_WIFI_PASSPHRASE);
  }
  if (prefs.isKey(PREFERENCES_KEY_WIFI_EAP_IDENTITY))
  {
    prefs.remove(PREFERENCES_KEY_WIFI_EAP_IDENTITY);
  }
  if (prefs.isKey(PREFERENCES_KEY_WIFI_EAP_USERNAME))
  {
    prefs.remove(PREFERENCES_KEY_WIFI_EAP_USERNAME);
  }
  if (prefs.isKey(PREFERENCES_KEY_WIFI_EAP_PASSWORD))
  {
    prefs.remove(PREFERENCES_KEY_WIFI_EAP_PASSWORD);
  }

  return true;
}

void KorraWiFi::credentials_load()
{
  if (prefs.isKey(PREFERENCES_KEY_WIFI_SSID))
  {
    prefs.getBytes(PREFERENCES_KEY_WIFI_SSID,
                   (char *)credentials.ssid, sizeof(credentials.ssid));
  }

  if (prefs.isKey(PREFERENCES_KEY_WIFI_PASSPHRASE))
  {
    prefs.getBytes(PREFERENCES_KEY_WIFI_PASSPHRASE,
                   (char *)credentials.passphrase, sizeof(credentials.passphrase));
  }
  if (prefs.isKey(PREFERENCES_KEY_WIFI_EAP_IDENTITY))
  {
    prefs.getBytes(PREFERENCES_KEY_WIFI_EAP_IDENTITY,
                   (char *)credentials.eap_identity, sizeof(credentials.eap_identity));
  }
  if (prefs.isKey(PREFERENCES_KEY_WIFI_EAP_USERNAME))
  {
    prefs.getBytes(PREFERENCES_KEY_WIFI_EAP_USERNAME,
                   (char *)credentials.eap_username, sizeof(credentials.eap_username));
  }
  if (prefs.isKey(PREFERENCES_KEY_WIFI_EAP_PASSWORD))
  {
    prefs.getBytes(PREFERENCES_KEY_WIFI_EAP_PASSWORD,
                   (char *)credentials.eap_password, sizeof(credentials.eap_password));
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

#ifdef CONFIG_WIFI_SCAN_NETWORKS
void KorraWiFi::listNetworks()
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
  Serial.println(F("Id  | SSID                             | Ch | RSSI | Security   | BSSID             "));
  Serial.println(F("----+----------------------------------+----+------+------------+-------------------"));

  // 2) For each network, build a fixed-width line
  for (int8_t i = 0; i < count; i++)
  {
    // Build into a buffer with fixed column widths:
    //   • Index:      width  3, right-aligned
    //   • SSID:       width 32, left-aligned (crop or pad with spaces if shorter)
    //   • RSSI:       width  4, right-aligned
    //   • Channel:    width  2, right-aligned
    //   • Security:   width 10, left-aligned (crop or pad with spaces if shorter)
    //   • BSSID:      width 17, left-aligned

    Serial.printf(
        "%3d | %-32.32s | %2d | %4d | %-10.10s | %-17s \n",
        i,
        WiFi.SSID(i).c_str(), // Use c_str() to get a const char* from String
        WiFi.channel(i),
        WiFi.RSSI(i),
        // Use reinterpret_cast to convert from __FlashStringHelper* to const char*
        reinterpret_cast<const char *>(
            encryptionTypeToString(WiFi.encryptionType(i))),
        // Use c_str() to get a const char* from String
        WiFi.BSSIDstr(i).c_str());
  }
  Serial.println();

  // Delete the scan result to free memory for code below.
  WiFi.scanDelete();
}
#endif

void KorraWiFi::connect(bool initial)
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

  if (strlen(credentials.ssid) == 0)
  {
    if (!logged_missing_creds)
    {
      Serial.println("WiFi credentials are not set. Use the shell e.g. \"wifi credentials set ....\"");
      logged_missing_creds = true;
    }
    return;
  }

  Serial.printf("Attempting to connect to WiFi SSID: %s\n", credentials.ssid);

  if (credentials.eap_identity != NULL)
  {
    status = WiFi.begin(/* wpa2_ssid */ (const char *)credentials.ssid,
                        /* method */ wpa2_auth_method_t::WPA2_AUTH_PEAP,
                        /* wpa2_identity */ (const char *)credentials.eap_identity,
                        /* wpa2_username */ (const char *)credentials.eap_username,
                        /* wpa2_password */ (const char *)credentials.eap_password);
  }
  else
  {
    status = WiFi.begin(credentials.ssid, credentials.passphrase);
  }

  uint32_t started = millis();
  while (status != WL_CONNECTED)
  {
    // Timeout reached – perform a reset
    if ((millis() - started) > (CONFIG_WIFI_CONNECTION_REBOOT_TIMEOUT_SEC * 1000))
    {
      Serial.printf(" taken too long (%d sec). Rebooting in 5 sec ....\n", CONFIG_WIFI_CONNECTION_REBOOT_TIMEOUT_SEC);
      delay(5000);
      sys_reboot();
    }

    delay(500);
    Serial.print(F("."));
    status = WiFi.status();
  }
  _status = WL_CONNECTED;

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
  // The WiFi stack needs to have been activated by scanning or connecting hence why this is done last. Otherwise just zeros.
  if (initial)
  {
    snprintf(net_props.hostname, sizeof(net_props.hostname), "korra-" FMT_LL_ADDR_6_LOWER_NO_COLONS, PRINT_LL_ADDR_6(net_props.mac_addr));
    WiFi.setHostname(net_props.hostname);
    Serial.printf("Set hostname to %s\n", net_props.hostname);
  }
}

#endif // BOARD_HAS_WIFI
