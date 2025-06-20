#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(korra_certs, LOG_LEVEL_DBG);

#include "korra_certs.h"
#include "korra_certs_private.h"

const sec_tag_t registered_sec_tags[] = {
    KORRA_CERT_CA_SECTIGO_SEC_TAG,
    KORRA_CERT_CA_AZURE_IOT_SEC_TAG,
};

int korra_certs_init()
{
    int ret = 0;

    ret = tls_credential_add(KORRA_CERT_CA_AZURE_IOT_SEC_TAG, TLS_CREDENTIAL_CA_CERTIFICATE, azure_iot_ca_cert, sizeof(azure_iot_ca_cert));
    if (ret < 0)
    {
        LOG_ERR("Failed to register root cert for Azure IoT: %d", ret);
        return ret;
    }

    ret = tls_credential_add(KORRA_CERT_CA_SECTIGO_SEC_TAG, TLS_CREDENTIAL_CA_CERTIFICATE, sectigo_ca_cert, sizeof(sectigo_ca_cert));
    if (ret < 0)
    {
        LOG_ERR("Failed to register root cert for Sectigo: %d", ret);
        return ret;
    }

    return ret;
}
