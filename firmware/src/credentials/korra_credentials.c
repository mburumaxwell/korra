#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(korra_credentials, LOG_LEVEL_DBG);

#include "korra_credentials.h"
#include "korra_rot.h"
#include "korra_test_certs.h"

struct korra_credential
{
    enum tls_credential_type type;
    enum korra_credential_tag_type tag;
    const uint8_t *cred;
    const size_t credlen;
};

static struct korra_credential credentials[] = {
// NOTE: some TLS credentials come in pairs:
// - TLS_CREDENTIAL_PUBLIC_CERTIFICATE with TLS_CREDENTIAL_PRIVATE_KEY,
// - TLS_CREDENTIAL_PSK with TLS_CREDENTIAL_PSK_ID.
// Such pairs of credentials must be assigned the same secure tag to be correctly handled in the system.

#ifdef CONFIG_WIFI_NM_WPA_SUPPLICANT_CRYPTO_ENTERPRISE
    /* WiFi enterprise */
    {
        .type = TLS_CREDENTIAL_CA_CERTIFICATE,
        .tag = KORRA_CREDENTIAL_WIFI_CA_TAG,
        .cred = wifi_ca_cert,
        .credlen = ARRAY_SIZE(wifi_ca_cert),
    },
    {
        .type = TLS_CREDENTIAL_PUBLIC_CERTIFICATE,
        .tag = KORRA_CREDENTIAL_WIFI_CLIENT_TAG,
        .cred = wifi_client_cert_test,
        .credlen = ARRAY_SIZE(wifi_client_cert_test),
    },
    {
        .type = TLS_CREDENTIAL_PRIVATE_KEY,
        .tag = KORRA_CREDENTIAL_WIFI_CLIENT_TAG,
        .cred = wifi_client_key_test,
        .credlen = ARRAY_SIZE(wifi_client_key_test),
    },
    /* Phase 2 */
    {
        .type = TLS_CREDENTIAL_CA_CERTIFICATE,
        .tag = KORRA_CREDENTIAL_WIFI_CA_P2_TAG,
        .cred = wifi_ca_cert2_test,
        .credlen = ARRAY_SIZE(wifi_ca_cert2_test),
    },
    {
        .type = TLS_CREDENTIAL_PUBLIC_CERTIFICATE,
        .tag = KORRA_CREDENTIAL_WIFI_CLIENT_P2_TAG,
        .cred = wifi_client_cert2_test,
        .credlen = ARRAY_SIZE(wifi_client_cert2_test),
    },
    {
        .type = TLS_CREDENTIAL_PRIVATE_KEY,
        .tag = KORRA_CREDENTIAL_WIFI_CLIENT_P2_TAG,
        .cred = wifi_client_key2_test,
        .credlen = ARRAY_SIZE(wifi_client_key2_test),
    },
#endif // CONFIG_WIFI_NM_WPA_SUPPLICANT_CRYPTO_ENTERPRISE

    /* Azure */
    {
        .tag = KORRA_CREDENTIAL_AZURE_CA_TAG,
        .type = TLS_CREDENTIAL_CA_CERTIFICATE,
        .cred = azure_ca_cert,
        .credlen = ARRAY_SIZE(azure_ca_cert),
    },
};

int korra_credentials_init()
{
    int ret = 0;

    LOG_INF("Loading %d credentials", ARRAY_SIZE(credentials));
    for (size_t i = 0; i < ARRAY_SIZE(credentials); i++)
    {
        LOG_DBG("Adding type: %-18s -> tag: %-20s (len: %d)",
                tls_credential_type_txt(credentials[i].type),
                korra_credential_tag_type_txt(credentials[i].tag),
                credentials[i].credlen);

        ret = tls_credential_add(credentials[i].tag, credentials[i].type, credentials[i].cred, credentials[i].credlen);
        if (ret < 0)
        {
            LOG_ERR("Failed to register root cert for Azure: %d", ret);
            return ret;
        }
    }

    return ret;
}

const char *korra_credential_tag_type_txt(enum korra_credential_tag_type type)
{
    switch (type)
    {
#ifdef CONFIG_WIFI_NM_WPA_SUPPLICANT_CRYPTO_ENTERPRISE
    /* WiFi enterprise */
    case KORRA_CREDENTIAL_WIFI_CA_TAG:
        return "wifi-ca";
    case KORRA_CREDENTIAL_WIFI_CLIENT_TAG:
        return "wifi-client";
    case KORRA_CREDENTIAL_WIFI_CA_P2_TAG:
        return "wifi-ca-phase-2";
    case KORRA_CREDENTIAL_WIFI_CLIENT_P2_TAG:
        return "wifi-client-phase-2";
#endif // CONFIG_WIFI_NM_WPA_SUPPLICANT_CRYPTO_ENTERPRISE

    /* Azure */
    case KORRA_CREDENTIAL_AZURE_CA_TAG:
        return "azure-ca";

    default:
        return "unknown";
    }
}

const char *tls_credential_type_txt(enum tls_credential_type type)
{
    switch (type)
    {
    case TLS_CREDENTIAL_NONE:
        return "none";
    case TLS_CREDENTIAL_CA_CERTIFICATE:
        return "ca-certificate";
    case TLS_CREDENTIAL_PUBLIC_CERTIFICATE:
        return "public-certificate";
    case TLS_CREDENTIAL_PRIVATE_KEY:
        return "private-key";
    case TLS_CREDENTIAL_PSK:
        return "psk";
    case TLS_CREDENTIAL_PSK_ID:
        return "psk-id";
    default:
        return "unknown";
    }
}
