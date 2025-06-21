#ifndef KORRA_TEST_CERTS_H
#define KORRA_TEST_CERTS_H

#ifdef CONFIG_WIFI_NM_WPA_SUPPLICANT_CRYPTO_ENTERPRISE
static const uint8_t wifi_client_cert_test[] = {
#include <wifi_certs/client.pem.inc>
    '\0'};

static const uint8_t wifi_client_key_test[] = {
#include <wifi_certs/client-key.pem.inc>
    '\0'};

static const uint8_t wifi_ca_cert2_test[] = {
#include <wifi_certs/ca2.pem.inc>
    '\0'};

static const uint8_t wifi_client_cert2_test[] = {
#include <wifi_certs/client2.pem.inc>
    '\0'};

static const uint8_t wifi_client_key2_test[] = {
#include <wifi_certs/client-key2.pem.inc>
    '\0'};
#endif // CONFIG_WIFI_NM_WPA_SUPPLICANT_CRYPTO_ENTERPRISE

#endif /* KORRA_TEST_CERTS_H */
