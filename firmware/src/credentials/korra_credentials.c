#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(korra_credentials, LOG_LEVEL_INF);

#include <mbedtls/ctr_drbg.h>
#include <mbedtls/entropy.h>
#include <mbedtls/pk.h>
#include <mbedtls/x509_crt.h>
#include <mbedtls/x509_csr.h>
#include <mbedtls/x509.h>

#include <time.h>
#include <zephyr/settings/settings.h>

#include "korra_credentials.h"
#include "korra_rot.h"
#include "korra_test_certs.h"

struct korra_credential
{
    const enum korra_credential_tag_type tag;
    const enum tls_credential_type type;
    uint8_t *data;
    size_t len;
};

struct korra_credential_generateble
{
    const enum korra_credential_tag_type tag;
#ifdef CONFIG_TLS_CREDENTIALS_BACKEND_VOLATILE
    const char *settings_name_prefix;
#endif // CONFIG_TLS_CREDENTIALS_BACKEND_PROTECTED_STORAGE
};

static struct korra_credential credentials[] = {

#ifdef CONFIG_WIFI_NM_WPA_SUPPLICANT_CRYPTO_ENTERPRISE
    /* WiFi enterprise */
    {
        .tag = KORRA_CREDENTIAL_WIFI_CA_TAG,
        .type = TLS_CREDENTIAL_CA_CERTIFICATE,
        .data = (char *)wifi_ca_cert,
        .len = ARRAY_SIZE(wifi_ca_cert),
    },
    {
        .tag = KORRA_CREDENTIAL_WIFI_CLIENT_TAG,
        .type = TLS_CREDENTIAL_PUBLIC_CERTIFICATE,
        .data = (char *)wifi_client_cert,
        .len = ARRAY_SIZE(wifi_client_cert),
    },
    {
        .tag = KORRA_CREDENTIAL_WIFI_CLIENT_KEY_TAG,
        .type = TLS_CREDENTIAL_PRIVATE_KEY,
        .data = (char *)wifi_client_key,
        .len = ARRAY_SIZE(wifi_client_key),
    },
    /* Phase 2 */
    {
        .tag = KORRA_CREDENTIAL_WIFI_CA_P2_TAG,
        .type = TLS_CREDENTIAL_CA_CERTIFICATE,
        .data = (char *)wifi_ca_cert2,
        .len = ARRAY_SIZE(wifi_ca_cert2),
    },
    {
        .tag = KORRA_CREDENTIAL_WIFI_CLIENT_P2_TAG,
        .type = TLS_CREDENTIAL_PUBLIC_CERTIFICATE,
        .data = (char *)wifi_client_cert2,
        .len = ARRAY_SIZE(wifi_client_cert2),
    },
    {
        .tag = KORRA_CREDENTIAL_WIFI_CLIENT_KEY_P2_TAG,
        .type = TLS_CREDENTIAL_PRIVATE_KEY,
        .data = (char *)wifi_client_key2,
        .len = ARRAY_SIZE(wifi_client_key2),
    },
#endif // CONFIG_WIFI_NM_WPA_SUPPLICANT_CRYPTO_ENTERPRISE

    /* Azure */
    {
        .tag = KORRA_CREDENTIAL_AZURE_CA_TAG,
        .type = TLS_CREDENTIAL_CA_CERTIFICATE,
        .data = (char *)azure_ca_cert,
        .len = ARRAY_SIZE(azure_ca_cert),
    },

    /* Device */
    {
        .tag = KORRA_CREDENTIAL_DEVICE_TAG,
        .type = TLS_CREDENTIAL_PUBLIC_CERTIFICATE,
        .data = NULL,
        .len = 0,
    },
    {
        .tag = KORRA_CREDENTIAL_DEVICE_TAG,
        .type = TLS_CREDENTIAL_PRIVATE_KEY,
        .data = NULL,
        .len = 0,
    },
};

static struct korra_credential_generateble generateble_credentials[] = {
    {
        .tag = KORRA_CREDENTIAL_DEVICE_TAG,
        .settings_name_prefix = "device-" // will result in "device-{cert|key}"
    },
};

static int generate_credentials(const char *devid, const size_t devid_len);
static int load_credentials();

int korra_credentials_init(const char *devid, const size_t devid_len)
{
    int ret = 0;

    ret = generate_credentials(devid, devid_len);
    ret = load_credentials();

    return ret;
}

const char *korra_credential_tag_type_txt(const enum korra_credential_tag_type type)
{
    switch (type)
    {
#ifdef CONFIG_WIFI_NM_WPA_SUPPLICANT_CRYPTO_ENTERPRISE
    /* WiFi enterprise */
    case KORRA_CREDENTIAL_WIFI_CA_TAG:
        return "wifi-ca";
    case KORRA_CREDENTIAL_WIFI_CLIENT_TAG:
        return "wifi-client";
    case KORRA_CREDENTIAL_WIFI_CLIENT_KEY_TAG:
        return "wifi-client-key";
    case KORRA_CREDENTIAL_WIFI_CA_P2_TAG:
        return "wifi-ca-phase-2";
    case KORRA_CREDENTIAL_WIFI_CLIENT_P2_TAG:
        return "wifi-client-phase-2";
    case KORRA_CREDENTIAL_WIFI_CLIENT_KEY_P2_TAG:
        return "wifi-client-key-phase-2";
#endif // CONFIG_WIFI_NM_WPA_SUPPLICANT_CRYPTO_ENTERPRISE

    /* Azure */
    case KORRA_CREDENTIAL_AZURE_CA_TAG:
        return "azure-ca";

    /* Device */
    case KORRA_CREDENTIAL_DEVICE_TAG:
        return "device";

    default:
        return "unknown";
    }
}

const char *tls_credential_type_txt(const enum tls_credential_type type)
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

int korra_credentials_clear()
{
    int ret = 0;

    LOG_INF("Deleting %d known credentials", ARRAY_SIZE(credentials));
    for (size_t i = 0; i < ARRAY_SIZE(credentials); i++)
    {
        bool exists = true; // assume it exists unless proved otherwise
        ret = tls_credential_exists(credentials[i].tag, credentials[i].type, credentials[i].len, &exists);
        if (ret)
        {
            LOG_WRN("Unable to determine if type: %-18s -> tag: %-20s exists (ret: %d)",
                    tls_credential_type_txt(credentials[i].type),
                    korra_credential_tag_type_txt(credentials[i].tag),
                    ret);
            continue;
        }

        if (!exists)
        {
            // nothing more to do
            continue;
        }

        LOG_DBG("Deleting type: %-18s -> tag: %-20s",
                tls_credential_type_txt(credentials[i].type),
                korra_credential_tag_type_txt(credentials[i].tag));

        ret = tls_credential_delete(credentials[i].tag, credentials[i].type);
        if (ret)
        {
            LOG_WRN("Unable to delete type: %-18s -> tag: %-20s (ret: %d)",
                    tls_credential_type_txt(credentials[i].type),
                    korra_credential_tag_type_txt(credentials[i].tag),
                    ret);
            continue;
        }
    }

#ifdef CONFIG_TLS_CREDENTIALS_BACKEND_VOLATILE
    LOG_INF("Deleting %d known settings for credentials", ARRAY_SIZE(generateble_credentials) * 2);
    for (size_t i = 0; i < ARRAY_SIZE(generateble_credentials); i++)
    {
        bool exists = true; // assume it exists unless proved otherwise

        char settings_name_cert[strlen(generateble_credentials[i].settings_name_prefix) + sizeof("cert")];
        snprintf(settings_name_cert, sizeof(settings_name_cert), "%scert", generateble_credentials[i].settings_name_prefix);
        ret = settings_exists_one(settings_name_cert, &exists);
        if (ret == 0 && exists)
        {
            LOG_DBG("Deleting '%s' from settings", settings_name_cert);
            ret = settings_delete(settings_name_cert);
        }

        char settings_name_key[strlen(generateble_credentials[i].settings_name_prefix) + sizeof("key")];
        snprintf(settings_name_key, sizeof(settings_name_key), "%skey", generateble_credentials[i].settings_name_prefix);
        ret = settings_exists_one(settings_name_key, &exists);
        if (ret == 0 && exists)
        {
            LOG_DBG("Deleting '%s' from settings", settings_name_key);
            ret = settings_delete(settings_name_key);
        }
    }
#endif // CONFIG_TLS_CREDENTIALS_BACKEND_VOLATILE

    return ret;
}

size_t korra_credentials_get_device_cert(char *dest, size_t dest_len)
{
    size_t len;
    int ret = tls_credential_get(KORRA_CREDENTIAL_DEVICE_TAG, TLS_CREDENTIAL_PUBLIC_CERTIFICATE, dest, &len);
    if (ret == 0)
    {
        return len;
    }

    return -1;
}

int tls_credential_exists(enum korra_credential_tag_type tag,
                          enum tls_credential_type type,
                          size_t known_len,
                          bool *exists)
{
    size_t len;
    int ret = tls_credential_get(tag, type, NULL, &len);
    if (ret == -ENOENT)
    {
        *exists = len != 0 && known_len == len; // it exists if the length is not zero and matches
        return 0;
    }
    else if (ret == -EFBIG)
    {
        *exists = true;
        return 0;
    }
    else
    {
        return -EIO;
    }
}

int settings_exists_one(const char *name, bool *exists)
{
    ssize_t ret_or_len = settings_get_val_len(name);
    if (ret_or_len >= 0)
    {
        *exists = ret_or_len > 0;
        return 0;
    }
    return ret_or_len;
}

static int generate_cert(enum korra_credential_tag_type tag,
                         const char *devid, const size_t devid_len,
                         const char *not_before, const char *not_after,
                         uint8_t *cert_pem, size_t cert_pem_len,
                         uint8_t *key_pem, size_t key_pem_len)
{
    mbedtls_pk_context key;
    mbedtls_x509write_cert crt;
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    int ret;

    // make seed based on tag and device id to ensure more randomness
    char seed[sizeof("keygen-") + 20 + devid_len];
    snprintf(seed, sizeof(seed), "keygen-%-20s-%s", korra_credential_tag_type_txt(tag), devid);

    mbedtls_pk_init(&key);
    mbedtls_x509write_crt_init(&crt);
    mbedtls_ctr_drbg_init(&ctr_drbg);
    mbedtls_entropy_init(&entropy);

    mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy,
                          (const unsigned char *)seed, strlen(seed));

    // Generate EC key pair (secp256r1)
    mbedtls_pk_setup(&key, mbedtls_pk_info_from_type(MBEDTLS_PK_ECKEY));
    mbedtls_ecp_gen_key(MBEDTLS_ECP_DP_SECP256R1,
                        mbedtls_pk_ec(key),
                        mbedtls_ctr_drbg_random,
                        &ctr_drbg);
    mbedtls_pk_write_key_pem(&key, key_pem, key_pem_len);

    // Set certificate parameters
    unsigned char serial[MBEDTLS_X509_RFC5280_MAX_SERIAL_LEN] = {0};
    size_t serial_len = 1;
    serial[0] = 1; /* your previous serial “1” */
    mbedtls_x509write_crt_set_version(&crt, MBEDTLS_X509_CRT_VERSION_3);
    mbedtls_x509write_crt_set_md_alg(&crt, MBEDTLS_MD_SHA256);
    mbedtls_x509write_crt_set_serial_raw(&crt, serial, serial_len);
    mbedtls_x509write_crt_set_validity(&crt, not_before, not_after);
    mbedtls_x509write_crt_set_subject_key(&crt, &key);
    mbedtls_x509write_crt_set_issuer_key(&crt, &key); // Self-signed

    // Subject and issuer name (CN = device_id)
    char subject[sizeof("CN=") + devid_len + 1];
    snprintf(subject, sizeof(subject), "CN=%s", devid);
    mbedtls_x509write_crt_set_subject_name(&crt, subject);
    mbedtls_x509write_crt_set_issuer_name(&crt, subject);

    // Write certificate to PEM
    memset(cert_pem, 0, cert_pem_len);
    ret = mbedtls_x509write_crt_pem(&crt, cert_pem, cert_pem_len,
                                    mbedtls_ctr_drbg_random, &ctr_drbg);

    if (ret)
    {
        LOG_ERR("Unable to generate certificate for tag: %-20s: %d (mbedtls: -0x%04X)",
                korra_credential_tag_type_txt(tag), ret, -ret);
    }

    // Clean up
    mbedtls_pk_free(&key);
    mbedtls_x509write_crt_free(&crt);
    mbedtls_ctr_drbg_free(&ctr_drbg);
    mbedtls_entropy_free(&entropy);
    return ret;
}

static int generate_credentials(const char *devid, const size_t devid_len)
{
    // Check if time has been set (assuming anything before 2024-12-31 23:59:59 means time not set)
    time_t now = time(NULL);
    time_t min_valid_time = 1735689599; // 2024-12-31 23:59:59 UTC as Unix timestamp
    if (now < min_valid_time)
    {
        now = min_valid_time; // Default to 2024-12-31 23:59:59
        LOG_DBG("Time not set, using default time (2024-12-31 23:59:59)\n");
    }

    // Generate not_before (current time)
    struct tm tm;
    gmtime_r(&now, &tm);
    char not_before[sizeof("20241231235959")]; // format: YYYYMMDDhhmmss e.g. 20241231235959 for December 31st 2024 at 23:59:59
    strftime(not_before, sizeof(not_before), "%Y%m%d%H%M%S", &tm);

    // Generate not_after (couple of years ahead, approximate)
    time_t future = now + (CONFIG_DEVICE_CERTIFICATE_VALIDITY_YEARS * 365 * 24 * 3600);
    gmtime_r(&future, &tm);
    char not_after[sizeof("20241231235959")]; // format: YYYYMMDDhhmmss e.g. 20241231235959 for December 31st 2024 at 23:59:59
    strftime(not_after, sizeof(not_after), "%Y%m%d%H%M%S", &tm);

    LOG_INF("Checking %d credentials that need generation", ARRAY_SIZE(generateble_credentials));
    for (size_t i = 0; i < ARRAY_SIZE(generateble_credentials); i++)
    {
        int ret;
        bool exists, generate = false;
        ret = tls_credential_exists(generateble_credentials[i].tag, TLS_CREDENTIAL_PUBLIC_CERTIFICATE, 0, &exists);
        if (ret)
        {
            LOG_WRN("Unable to determine if tag: %-20s exists (ret: %d)",
                    korra_credential_tag_type_txt(generateble_credentials[i].tag),
                    ret);
            continue;
        }

        if (exists)
        {
            LOG_DBG("Credential for tag: %-20s already exists", korra_credential_tag_type_txt(generateble_credentials[i].tag));
            continue;
        }

        // generally elliptic curves occupy very little memory and are better than RSA
        size_t cert_pem_len = 1024, key_pem_len = 384;
        uint8_t *cert_pem = (uint8_t *)k_malloc(cert_pem_len);
        uint8_t *key_pem = (uint8_t *)k_malloc(key_pem_len);
        if (cert_pem == NULL || key_pem == NULL)
        {
            LOG_ERR("Unable to allocate memory to store generated cert and key for tag: %-20s",
                    korra_credential_tag_type_txt(generateble_credentials[i].tag));
            return -ENOMEM;
        }
        memset(cert_pem, 0, cert_pem_len);
        memset(key_pem, 0, key_pem_len);

#ifdef CONFIG_TLS_CREDENTIALS_BACKEND_VOLATILE
        // when using volatile backend, the cert and key are present in storage, we load them
        bool loaded_cert = false;
        char settings_name_cert[strlen(generateble_credentials[i].settings_name_prefix) + sizeof("cert")];
        snprintf(settings_name_cert, sizeof(settings_name_cert), "%scert", generateble_credentials[i].settings_name_prefix);
        ret = settings_exists_one(settings_name_cert, &exists);
        if (ret == 0 && exists)
        {
            LOG_DBG("Loading '%s' from settings", settings_name_cert);
            ret = settings_load_one(settings_name_cert, cert_pem, cert_pem_len);
            loaded_cert = ret > 0;
        }

        bool loaded_key = false;
        char settings_name_key[strlen(generateble_credentials[i].settings_name_prefix) + sizeof("key")];
        snprintf(settings_name_key, sizeof(settings_name_key), "%skey", generateble_credentials[i].settings_name_prefix);
        ret = settings_exists_one(settings_name_key, &exists);
        if (ret == 0 && exists)
        {
            LOG_DBG("Loading '%s' from settings", settings_name_key);
            ret = settings_load_one(settings_name_key, key_pem, key_pem_len);
            loaded_key = ret > 0;
        }

        generate = !(loaded_cert && loaded_key);
#endif // CONFIG_TLS_CREDENTIALS_BACKEND_VOLATILE

        if (generate)
        {
            ret = generate_cert(generateble_credentials[i].tag, devid, devid_len, not_before, not_after,
                                cert_pem, cert_pem_len, key_pem, key_pem_len);
            if (ret)
            {
                k_free(cert_pem);
                k_free(key_pem);
            }
            else
            {
#ifdef CONFIG_TLS_CREDENTIALS_BACKEND_VOLATILE
                // when using volatile backend, save the generated cert and key
                LOG_DBG("Saving '%s' to settings", settings_name_cert);
                settings_save_one(settings_name_cert, cert_pem, strlen(cert_pem));
                LOG_DBG("Saving '%s' to settings", settings_name_key);
                settings_save_one(settings_name_key, key_pem, strlen(key_pem));
#endif // CONFIG_TLS_CREDENTIALS_BACKEND_VOLATILE
            }
        }

        for (size_t j = 0; j < ARRAY_SIZE(credentials); j++)
        {
            if (credentials[j].tag == generateble_credentials[i].tag)
            {
                if (credentials[j].type == TLS_CREDENTIAL_PUBLIC_CERTIFICATE)
                {
                    credentials[j].data = cert_pem;
                    credentials[j].len = strlen(cert_pem);
                }
                else if (credentials[j].type == TLS_CREDENTIAL_PRIVATE_KEY)
                {
                    credentials[j].data = key_pem;
                    credentials[j].len = strlen(key_pem);
                }
            }
        }
    }

    return 0;
}

static int load_credentials()
{
    int ret;

    LOG_INF("Loading %d credentials", ARRAY_SIZE(credentials));
    for (size_t i = 0; i < ARRAY_SIZE(credentials); i++)
    {
        bool exists = true; // assume it exists unless proved otherwise
        ret = tls_credential_exists(credentials[i].tag, credentials[i].type, credentials[i].len, &exists);
        if (ret)
        {
            LOG_WRN("Unable to determine if type: %-18s -> tag: %-20s exists (ret: %d)",
                    tls_credential_type_txt(credentials[i].type),
                    korra_credential_tag_type_txt(credentials[i].tag),
                    ret);
            continue;
        }

        if (exists)
        {
            LOG_DBG("Credential for type: %-18s -> tag: %-20s already exists",
                    tls_credential_type_txt(credentials[i].type),
                    korra_credential_tag_type_txt(credentials[i].tag));
            continue;
        }

        LOG_DBG("Adding type: %-18s -> tag: %-20s (len: %d)",
                tls_credential_type_txt(credentials[i].type),
                korra_credential_tag_type_txt(credentials[i].tag),
                credentials[i].len);

        if (credentials[i].data == NULL || credentials[i].len <= 0)
        {
            LOG_ERR("No data/len for type: %-18s -> tag: %-20s (len: %d)",
                    tls_credential_type_txt(credentials[i].type),
                    korra_credential_tag_type_txt(credentials[i].tag),
                    credentials[i].len);
            return -EFAULT;
        }

        ret = tls_credential_add(credentials[i].tag, credentials[i].type, credentials[i].data, credentials[i].len);
        if (ret < 0)
        {
            LOG_ERR("Failed to add type: %-18s -> tag: %-20s (ret: %d)",
                    tls_credential_type_txt(credentials[i].type),
                    korra_credential_tag_type_txt(credentials[i].tag),
                    ret);
            return ret;
        }
    }

#ifndef CONFIG_TLS_CREDENTIALS_BACKEND_VOLATILE
    // For non-volatile storage, we no longer need the cert/keys in memory so we free them.
    // They are loaded every time they are needed.
    LOG_INF("Clearing %d credentials from memory", ARRAY_SIZE(generateble_credentials));
    for (size_t i = 0; i < ARRAY_SIZE(generateble_credentials); i++)
    {
        for (size_t i = 0; i < ARRAY_SIZE(credentials); i++)
        {
            if (credentials[i].tag == generateble_credentials[i].tag)
            {
                if (credentials[i].data)
                {
                    memset(credentials[i].data, 0, credentials[i].len); // clear to prevent
                    k_free(credentials[i].data);                        // release for use by others
                }
                credentials[i].data = NULL;
                credentials[i].len = 0;
            }
        }
    }
#endif // CONFIG_TLS_CREDENTIALS_BACKEND_PROTECTED_STORAGE

    return ret;
}
