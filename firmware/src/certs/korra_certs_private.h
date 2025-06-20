#ifndef KORRA_CERTS_PRIVATE_H
#define KORRA_CERTS_PRIVATE_H

static const unsigned char azure_iot_ca_cert[] = {
#include "azure_iot.cer"
};

static const unsigned char sectigo_ca_cert[] = {
#include "sectigo.cer"
};

#endif /* KORRA_CERTS_PRIVATE_H */
