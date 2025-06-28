#include <sys/reboot.h>

#include "korra_mdns.h"

#ifdef CONFIG_BOARD_HAS_INTERNET

KorraMdns::KorraMdns(UDP &client) : mdns(client)
{
}

KorraMdns::~KorraMdns()
{
}

void KorraMdns::begin(const struct korra_network_props *props)
{
  if (!mdns.begin(props->local_ipaddr, props->hostname))
  {
    Serial.println("Error setting up mDNS! Rebooting in 5 sec ....");
    delay(5000);
    sys_reboot();
  }

  // We register a service to aid with checking if the device is on the network. This aids troubleshooting
  // service discovery where network has different conditions like mDNS/Bonjour disabled, different VLAN.
  // The name must start with the description as per examples.
  // Without the TXT record, sometimes execution crashes; see https://github.com/arduino-libraries/ArduinoMDNS/issues/36
  //                                                          https://github.com/arduino-libraries/ArduinoMDNS/pull/23
  // The TXT record "\x05id=01" implies TXT record with 5 bytes 'id=01' which is just a random key value pair
  // Post 7007 is random, not using 80 to avoid confusion with HTTP.
  // The service name includes the mac address to make it easier to identify multiple devices.
  // You can list this on Linux using "avahi-browse -at" and just for this one "avahi-browse -rt _korra._tcp"
  // On mac "dns-sd -B _services._dns-sd._tcp local" will list all services, "dns-sd -B _korra" will list just the ones for korra,
  // and "dns-sd -L "Korra <mac>" _korra" will print the TXT for the particular device.
#define SERVICE_NAME_FORMAT "Korra " FMT_LL_ADDR_6_LOWER_NO_COLONS "._korra"
  size_t service_name_len = snprintf(NULL, 0, SERVICE_NAME_FORMAT, PRINT_LL_ADDR_6(props->mac_addr));
  char service_name[service_name_len + 1] = {0};
  snprintf(service_name, sizeof(service_name), SERVICE_NAME_FORMAT, PRINT_LL_ADDR_6(props->mac_addr));
  mdns.addServiceRecord(service_name, 7007, MDNSServiceProtocol_t::MDNSServiceTCP, "\x05id=01");
}

void KorraMdns::maintain()
{
  mdns.run();
}

#endif // BOARD_HAS_INTERNET
