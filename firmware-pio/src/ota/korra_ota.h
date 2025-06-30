#ifndef KORRA_OTA_H
#define KORRA_OTA_H

#include <Arduino.h>

class KorraOta {
public:
  /**
   * Creates a new instance of the KorraOta class.
   * Please note that only one instance of the class can be initialized at the same time.
   *
   * @param client The secure TCP client to use for communication.
   */
  KorraOta(Client &client);

  /**
   * Cleanup resources created and managed by the KorraOta class.
   */
  ~KorraOta();

  /**
   * Setup the process (does not begin any update)
   *
   * @param ca_cert Root certificates
   */
  void begin(const char *ca_cert);

  /**
   * Start the firmware update process.
   * This should be called once we have ascertained that we have an update.
   *
   * @param url firmware binary URL
   * @param hash SHA-256 hash in hex
   * @param signature Signature to verify
   */
  void update(const char *url, const char *hash, const char *signature);

  /**
   * This method should be called periodically inside the main loop of the firmware.
   * It's safe to call this method in some interval (like 5ms).
   */
  void maintain();

private:
  const char *ca_cert;
};

#endif // KORRA_OTA_H
