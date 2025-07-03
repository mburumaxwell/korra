#ifndef KORRA_OTA_H
#define KORRA_OTA_H

struct korra_ota_info {
  char url[256 + 1];       // firmware binary URL
  char hash[64 + 1];       // SHA-256 hash in hex
  char signature[128 + 1]; // Signature (e.g., base64 or hex)
};

class KorraOta {
public:
  /**
   * Creates a new instance of the KorraOta class.
   * Please note that only one instance of the class can be initialized at the same time.
   */
  KorraOta();

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
   * @param value update info
   */
  void update(const struct korra_ota_info *value);

  /**
   * This method should be called periodically inside the main loop of the firmware.
   * It's safe to call this method in some interval (like 5ms).
   */
  void maintain();

  /**
   * Populate an instance of `struct korra_ota_info`.
   */
  void populate(const char *url, const char *hash, const char *signature, struct korra_ota_info *dest);

private:
  const char *ca_cert;
  struct korra_ota_info info;
  bool printed_fail;

private:
  enum https_ota_status {
    HTTPS_OTA_STATUS_IDLE,
    HTTPS_OTA_STATUS_UPDATING,
    HTTPS_OTA_STATUS_SUCCESS,
    HTTPS_OTA_STATUS_FAIL,
    HTTPS_OTA_STATUS_ERR
  };
  const enum https_ota_status current_status();
};

#endif // KORRA_OTA_H
