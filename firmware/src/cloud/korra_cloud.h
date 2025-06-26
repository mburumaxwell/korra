#ifndef KORRA_CLOUD_H_
#define KORRA_CLOUD_H_

#include <stddef.h>

/**
 * Initialize the cloud logic
 *
 * @param id The registration ID
 * @param id_len The length of the registration ID
 */
extern void korra_cloud_init(const char *regid, const size_t regid_len);

#endif // KORRA_CLOUD_H_
