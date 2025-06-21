#ifndef KORRA_CREDENTIALS_H
#define KORRA_CREDENTIALS_H

#include <zephyr/kernel.h>
#include <zephyr/net/tls_credentials.h>

enum korra_credential_tag_type
{
// NOTE: some TLS credentials come in pairs:
// - TLS_CREDENTIAL_PUBLIC_CERTIFICATE with TLS_CREDENTIAL_PRIVATE_KEY,
// - TLS_CREDENTIAL_PSK with TLS_CREDENTIAL_PSK_ID.
// Such pairs of credentials must be assigned the same secure tag to be correctly handled in the system.

#ifdef CONFIG_WIFI_NM_WPA_SUPPLICANT_CRYPTO_ENTERPRISE
    /* WiFi enterprise */
    KORRA_CREDENTIAL_WIFI_CA_TAG = 0x10,
    KORRA_CREDENTIAL_WIFI_CLIENT_TAG,
    /* Phase 2 */
    KORRA_CREDENTIAL_WIFI_CA_P2_TAG,
    KORRA_CREDENTIAL_WIFI_CLIENT_P2_TAG,
#endif // CONFIG_WIFI_NM_WPA_SUPPLICANT_CRYPTO_ENTERPRISE

    /* Azure */
    KORRA_CREDENTIAL_AZURE_CA_TAG = 0x20,
};

/** Initialize the credentials */
extern int korra_credentials_init();

/** Get the text representation of the certificate type */
extern const char *korra_credential_tag_type_txt(enum korra_credential_tag_type type);

/** Get the text representation of the TLS credential type */
extern const char *tls_credential_type_txt(enum tls_credential_type type);

#endif /* KORRA_CREDENTIALS_H */
