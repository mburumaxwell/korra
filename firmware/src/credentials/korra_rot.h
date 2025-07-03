// This file brings in root of trust certificates

#ifndef KORRA_ROT_H
#define KORRA_ROT_H

#ifdef CONFIG_WIFI_NM_WPA_SUPPLICANT_CRYPTO_ENTERPRISE
static const unsigned char wifi_ca_cert[] = {
#include "ca_sectigo.cer"
};
#endif // CONFIG_WIFI_NM_WPA_SUPPLICANT_CRYPTO_ENTERPRISE

static const unsigned char azure_ca_cert[] = {
#include "ca_azure.cer"
};

#endif /* KORRA_ROT_H */
