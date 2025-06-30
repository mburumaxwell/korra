#ifndef KORRA_CLOUD_SHARED_H
#define KORRA_CLOUD_SHARED_H

#include <stddef.h>

struct korra_cloud_provisioning_info {
  /** Whether the provisioning info is valid. */
  bool valid;

  /** The hostname of the device. */
  char hostname[64];

  /** The length of the hostname. */
  size_t hostname_len;

  /** The length of the hostname. */
  char id[128];

  /** The length of the ID. */
  size_t id_len;
};

#endif // KORRA_CLOUD_SHARED_H
