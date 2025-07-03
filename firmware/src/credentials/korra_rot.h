// This file brings in root of trust certificates

#ifndef KORRA_ROT_H
#define KORRA_ROT_H

/**
 * user_trust_rsa.cer = USERTrust RSA Certification Authority
 * Discovered as the root on EAP for ASK4 Wireless (CN=wifi.ask4.com)
 * Works for eduroam at LJMU (CN=ljmurad2.ljmu.ac.uk)
 */

/**
 * user_trust_ecc.cer = USERTrust ECC Certification Authority
 * Discovered as the root for github.com (needed for OTA updates from *.github.com or github.com)
 */

static const unsigned char user_trust_cert[] = {
#include "user_trust_ecc.cer"
#include "user_trust_rsa.cer"
};

static const unsigned char azure_ca_cert[] = {
#include "ca_azure.cer"
};

#endif /* KORRA_ROT_H */
