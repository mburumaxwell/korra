#ifndef KORRA_CLOUD_PROVISIONING_H_
#define KORRA_CLOUD_PROVISIONING_H_

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

extern int korra_cloud_provisioning_init(const char *regid, const size_t regid_len,
                                         struct korra_cloud_provisioning_info *result);
extern int korra_cloud_provisioning_run();
extern int korra_cloud_provisioning_clear();

#endif // KORRA_CLOUD_PROVISIONING_H_
