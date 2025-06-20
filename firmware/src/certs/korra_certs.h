#ifndef KORRA_CERTS_H
#define KORRA_CERTS_H

#include <zephyr/kernel.h>
#include <zephyr/net/tls_credentials.h>

enum korra_cert_sec_tags {
    KORRA_CERT_CA_SECTIGO_SEC_TAG = 1, // Should we set something like 0x1020001 ?
    KORRA_CERT_CA_AZURE_IOT_SEC_TAG,
};

extern const sec_tag_t registered_sec_tags[];

extern int korra_certs_init();

#endif /* KORRA_CERTS_H */
