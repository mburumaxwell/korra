#ifndef KORRA_CREDENTIALS_H
#define KORRA_CREDENTIALS_H

#include <Preferences.h>

class KorraCredentials {
public:
  /**
   * Creates a new instance of the KorraCredentials class.
   * Please note that only one instance of the class can be initialized at the same time.
   *
   * @param prefs The preferences instance to use for storing the credentials.
   */
  KorraCredentials(Preferences &prefs);

  /**
   * Cleanup resources created and managed by the KorraCredentials class.
   */
  ~KorraCredentials();

  /**
   * Initializes the credentials logic.
   * This should be called once at the begining of the program.
   *
   * @param devid The device ID.
   * @param devid_len The length of the device ID.
   */
  void begin(const char *devid, const size_t devid_len);

  /**
   * Clear the credentials.
   */
  void clear();

  /**
   * Get the root CA certificates
   * @return the root CA certificates
   */
  const char *root_ca_certs();

  /**
   * Get the device certificate without the private key
   * @return the device certificate
   * @note This should only be used to print to allow copying for provisioning setup.
   */
  const char *device_cert();

  /**
   * Get the device certificate without the private key
   * @return the device key
   * @note This should only be used in the transport client.
   */
  const char *device_key();

private:
  Preferences &prefs;
  char *devcert, *devkey;
  size_t devcert_len, devkey_len;
};

#endif /* KORRA_CREDENTIALS_H */
