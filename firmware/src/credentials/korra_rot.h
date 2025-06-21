// This file brings in root of trust certificates

#ifndef KORRA_ROT_H
#define KORRA_ROT_H

#ifdef CONFIG_WIFI_ENTERPRISE
static const unsigned char wifi_ca_cert[] = {
#include "ca_wifi.cer"
};
#endif // CONFIG_WIFI_ENTERPRISE

static const unsigned char azure_ca_cert[] = {
#include "ca_azure.cer"
};

#endif /* KORRA_ROT_H */
