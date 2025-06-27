#include <mbedtls/ctr_drbg.h>
#include <mbedtls/entropy.h>
#include <mbedtls/pk.h>
#include <mbedtls/x509_crt.h>
#include <mbedtls/x509_csr.h>
#include <mbedtls/x509.h>

#include "korra_credentials.h"

static const unsigned char ca_certs[] = {
#include "ca_azure.cer"
    // #include "ca_sectigo.cer"
};

#define PREFERENCES_KEY_DEVICE_CERT "device-cert"
#define PREFERENCES_KEY_DEVICE_KEY "device-key"

static int generate_cert(const char *devid, const size_t devid_len,
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
    snprintf(seed, sizeof(seed), "keygen-device-%s", devid);

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
        Serial.printf("Unable to generate certificate: %d (mbedtls: -0x%04X)\n", ret, -ret);
    }

    // Clean up
    mbedtls_pk_free(&key);
    mbedtls_x509write_crt_free(&crt);
    mbedtls_ctr_drbg_free(&ctr_drbg);
    mbedtls_entropy_free(&entropy);
    return ret;
}

KorraCredentials::KorraCredentials(Preferences &prefs) : prefs(prefs)
{
}

KorraCredentials::~KorraCredentials()
{
    if (devcert)
    {
        free(devcert);
        devcert = NULL;
    }
    devcert_len = 0;

    if (devkey)
    {
        free(devkey);
        devkey = NULL;
    }
    devkey_len = 0;
}

void KorraCredentials::begin(const char *devid, const size_t devid_len)
{
    if (prefs.isKey(PREFERENCES_KEY_DEVICE_CERT) && prefs.isKey(PREFERENCES_KEY_DEVICE_KEY))
    {
        Serial.println("Device certificate and key already exists, no need to generate a new one");
        return;
    }

    // Check if time has been set (assuming anything before 2024-12-31 23:59:59 means time not set)
    time_t now = time(NULL);
    time_t min_valid_time = 1735689599; // 2024-12-31 23:59:59 UTC as Unix timestamp
    if (now < min_valid_time)
    {
        now = min_valid_time; // Default to 2024-12-31 23:59:59
        Serial.printf("Time not set, using default time (2024-12-31 23:59:59)\n");
    }

    // Generate not_before (current time)
    struct tm tm;
    gmtime_r(&now, &tm);
    char not_before[sizeof("20241231235959")]; // format: YYYYMMDDhhmmss e.g. 20241231235959 for December 31st 2024 at 23:59:59
    strftime(not_before, sizeof(not_before), "%Y%m%d%H%M%S", &tm);

    // Generate not_after (number years ahead, approximate)
    time_t future = now + (CONFIG_DEVICE_CERTIFICATE_VALIDITY_YEARS * 365 * 24 * 3600);
    gmtime_r(&future, &tm);
    char not_after[sizeof("20241231235959")]; // format: YYYYMMDDhhmmss e.g. 20241231235959 for December 31st 2024 at 23:59:59
    strftime(not_after, sizeof(not_after), "%Y%m%d%H%M%S", &tm);

    Serial.printf("Generating device certificate and key. Not before: %s Not after:  %s\n", not_before, not_after);
    size_t cert_pem_len = 1024, key_pem_len = 384;
    uint8_t *cert_pem = (uint8_t *)malloc(cert_pem_len);
    uint8_t *key_pem = (uint8_t *)malloc(key_pem_len);
    int ret = generate_cert(devid, devid_len, not_before, not_after,
                            cert_pem, cert_pem_len, key_pem, key_pem_len);
    if (ret == 0)
    {
        cert_pem_len = strlen((char *)cert_pem);
        key_pem_len = strlen((char *)key_pem);
        prefs.putBytes(PREFERENCES_KEY_DEVICE_CERT, cert_pem, cert_pem_len);
        prefs.putBytes(PREFERENCES_KEY_DEVICE_KEY, key_pem, key_pem_len);
    }

    // free resources, these were just for creation
    free(cert_pem);
    free(key_pem);
}

void KorraCredentials::clear()
{
    prefs.remove(PREFERENCES_KEY_DEVICE_CERT);
    prefs.remove(PREFERENCES_KEY_DEVICE_KEY);
}

const char *KorraCredentials::root_ca_certs()
{
    return (const char *)ca_certs;
}

const char *KorraCredentials::device_cert()
{
    if (devcert == NULL)
    {
        devcert_len = prefs.getBytesLength(PREFERENCES_KEY_DEVICE_CERT);
        if (devcert_len == 0)
        {
            Serial.println("Device certificate is missing. Did you forget to call begin(...)?");
        }
        else
        {
            Serial.printf("Loading device certificate (%d bytes)\n", devcert_len);
            devcert_len++; // add byte for NULL
            devcert = (char *)malloc(devcert_len);
            memset(devcert, 0, devcert_len); // clear so that we only write what we need
            devcert_len = prefs.getBytes(PREFERENCES_KEY_DEVICE_CERT, devcert, devcert_len);
        }
    }

    return devcert;
}

const char *KorraCredentials::device_key()
{
    if (devkey == NULL)
    {
        devkey_len = prefs.getBytesLength(PREFERENCES_KEY_DEVICE_KEY);
        if (devkey_len == 0)
        {
            Serial.println("Device key is missing. Did you forget to call begin(...)?");
        }
        else
        {
            Serial.printf("Loading device key (%d bytes)\n", devkey_len);
            devkey_len++; // add byte for NULL
            devkey = (char *)malloc(devkey_len);
            memset(devkey, 0, devkey_len); // clear so that we only write what we need
            devkey_len = prefs.getBytes(PREFERENCES_KEY_DEVICE_KEY, devkey, devkey_len);
        }
    }

    return devkey;
}
