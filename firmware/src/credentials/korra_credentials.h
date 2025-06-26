#ifndef KORRA_CREDENTIALS_H
#define KORRA_CREDENTIALS_H

#include <zephyr/kernel.h>
#include <zephyr/net/tls_credentials.h>

enum korra_credential_tag_type
{
#ifdef CONFIG_WIFI_NM_WPA_SUPPLICANT_CRYPTO_ENTERPRISE
    /* WiFi enterprise */
    KORRA_CREDENTIAL_WIFI_CA_TAG = 0x10,
    KORRA_CREDENTIAL_WIFI_CLIENT_TAG,
    KORRA_CREDENTIAL_WIFI_CLIENT_KEY_TAG,
    /* Phase 2 */
    KORRA_CREDENTIAL_WIFI_CA_P2_TAG,
    KORRA_CREDENTIAL_WIFI_CLIENT_P2_TAG,
    KORRA_CREDENTIAL_WIFI_CLIENT_KEY_P2_TAG,
#endif // CONFIG_WIFI_NM_WPA_SUPPLICANT_CRYPTO_ENTERPRISE

    /* Azure */
    KORRA_CREDENTIAL_AZURE_CA_TAG = 0x20,

    /* Device */
    KORRA_CREDENTIAL_DEVICE_TAG,
};

/**
 * Initialize the credentials
 * @param devid - the device ID
 * @param devid_len - the length of the device ID
 * @return 0 on success, negative error code on failure
 */
extern int korra_credentials_init(const char *devid, const size_t devid_len);

/**
 * Get the text representation of the certificate type
 * @param type - the type of the credential
 * @return the text representation of the credential type
 */
extern const char *korra_credential_tag_type_txt(const enum korra_credential_tag_type type);

/**
 * Get the text representation of the TLS credential type
 * @param type - the type of the credential
 * @return the text representation of the credential type
 */
extern const char *tls_credential_type_txt(const enum tls_credential_type type);

/**
 * Check if a setting exists
 * @param name - the name of the setting
 * @param exists - pointer to the boolean to set
 * @return 0 on success, negative error code on failure
 */
extern int settings_exists_one(const char *name, bool *exists);

/**
 * Check if a TLS credential exists (pair)
 * @param tag - the tag of the credential
 * @param type - the type of the credential
 * @param known_len - the known length of the credential
 * @param exists - pointer to the boolean to set
 * @return 0 on success, negative error code on failure
 */
extern int tls_credential_exists(enum korra_credential_tag_type tag,
                                 enum tls_credential_type type,
                                 size_t known_len,
                                 bool *exists);

/**
 * Clear the credentials
 * @return 0 on success, negative error code on failure
 */
extern int korra_credentials_clear();

/**
 * Get the device certificate without the private key
 * @param dest - destination buffer
 * @param dest_len - destination buffer length
 * @return the length of the certificate
 * @note This should only be used to print to allow copying for provisioning setup.
 */
extern size_t korra_credentials_get_device_cert(char *dest, size_t dest_len);

#endif /* KORRA_CREDENTIALS_H */
