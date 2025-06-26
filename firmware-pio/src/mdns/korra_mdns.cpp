#include "korra_reboot.h"
#include "korra_mdns.h"

#if BOARD_HAS_NETWORK

KorraMdns::KorraMdns(UDP &udpClient) : mdns(udpClient)
{
}

KorraMdns::~KorraMdns()
{
}

void KorraMdns::begin(IPAddress ip, const char *hostname, uint8_t *mac)
{
  if (!mdns.begin(ip, hostname))
  {
    Serial.println("Error setting up MDNS!");
    reboot();
  }

  // We register a service to aid with checking if the device is on the network. This aids troubleshooting
  // service discovery where network has different conditions like mDNS/Bonjour disabled, different VLAN.
  // The name must start with the description as per examples.
  // Without the TXT record, sometimes execution crashes; see https://github.com/arduino-libraries/ArduinoMDNS/issues/36
  //                                                          https://github.com/arduino-libraries/ArduinoMDNS/pull/23
  // The TXT record "\x05id=01" implies TXT record with 5 bytes 'id=01' which is just a random key value pair
  // Post 7005 is random, not using 80 to avoid confusion with HTTP.
  // The service name includes the mac address to make it easier to identify multiple devices.
  // You can list this on Linux using "avahi-browse -at" and just for this one "avahi-browse -rt _korra._tcp"
  // On mac "dns-sd -B _services._dns-sd._tcp local" will list all services, "dns-sd -B _korra" will list just the ones for korra,
  // and "dns-sd -L "Korra <mac>" _korra" will print the TXT for the particular device.
  char serviceName[6 + 12 + 7 + 1]; // sizeof("Korra ") = 6; sizeof(mac*2) = 12; sizeof("._korra") = 7; + EOF (1) = 32
  snprintf(serviceName, sizeof(serviceName), "Korra %02X%02X%02X%02X%02X%02X._korra", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  mdns.addServiceRecord(serviceName, 7005, MDNSServiceProtocol_t::MDNSServiceTCP, "\x05id=01");
}

void KorraMdns::maintain()
{
  mdns.run();
}

#endif // BOARD_HAS_NETWORK
