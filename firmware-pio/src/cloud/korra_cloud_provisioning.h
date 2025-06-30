#ifndef KORRA_CLOUD_PROVISIONING_H_
#define KORRA_CLOUD_PROVISIONING_H_

#include "korra_config.h"

#ifdef CONFIG_BOARD_HAS_INTERNET

#include <Preferences.h>
#include <ArduinoMqttClient.h>
#include <arduino-timer.h>

#include "korra_cloud_shared.h"

/**
 * This class is a wrapper for the cloud functionalities.
 */
class KorraCloudProvisioning
{
public:
  /**
   * Creates a new instance of the KorraCloudProvisioning class.
   * Please note that only one instance of the class can be initialized at the same time.
   *
   * @param client The secure TCP client to use for communication.
   * @param prefs The preferences instance to use for storing the provisioning info.
   * @param tier The timer instance to use for scheduling repetitive tasks.
   */
  KorraCloudProvisioning(Client &client, Preferences &prefs, Timer<> &timer);

  /**
   * Cleanup resources created and managed by the KorraCloudProvisioning class.
   */
  ~KorraCloudProvisioning();

  /**
   * Initializes the time logic.
   * This should be called once at the beginning of the program.
   *
   * @param regid The device ID.
   * @param regid_len The length of the device ID.
   */
  void begin(const char *regid, const size_t regid_len);

  /**
   * This method should be called periodically inside the main loop of the firmware.
   * It's safe to call this method in some interval (like 5ms).
   */
  void maintain();

  /**
   * Clears the provisioning info.
   */
  void clear();

  /**
   * Get the provisioning info.
   */
  inline korra_cloud_provisioning_info *info() { return &stored_info; }

  /**
   * Check if the connection to the cloud is established.
   *
   * @return `true` if the connection is established, `false` otherwise.
   */
  inline bool connected() { return mqtt.connected(); }

  /**
   * Returns existing instance (singleton) of the KorraCloudProvisioning class.
   * It may be a null pointer if the KorraCloudProvisioning object was never constructed or it was destroyed.
   */
  inline static KorraCloudProvisioning *instance() { return _instance; }

  /**
   * Please do not call this method from outside the `KorraCloudProvisioning` class
   */
  void query_registration_result();

  /**
   * Please do not call this method from outside the `KorraCloudProvisioning` class
   */
  void on_mqtt_message(int size);

protected:
  /** The provisioning status type. */
  enum provisioning_registration_status
  {
    PROVISIONING_REGISTRATION_STATUS_UNKNOWN = 0,

    /** Device has not yet come online. */
    PROVISIONING_REGISTRATION_STATUS_UNASSIGNED = 1,

    /** Device has connected to the DRS but IoT hub Id has not yet been returned to the device. */
    PROVISIONING_REGISTRATION_STATUS_ASSIGNING = 2,

    /** DRS successfully returned a device Id and connection string to the device. */
    PROVISIONING_REGISTRATION_STATUS_ASSIGNED = 3,

    /** Device enrollment failed. */
    PROVISIONING_REGISTRATION_STATUS_FAILED = 4,

    /** Device is disabled. */
    PROVISIONING_REGISTRATION_STATUS_DISABLED = 5,
  };

  /** The provisioning substatus type. */
  enum provisioning_registration_sub_status
  {
    PROVISIONING_REGISTRATION_SUB_STATUS_UNKNOWN = 1,

    /** Device has been assigned to an IoT hub for the first time. */
    PROVISIONING_REGISTRATION_SUB_STATUS_INITIAL_ASSIGNMENT = 1,

    /**
     * Device has been assigned to a different IoT hub and its device data was migrated from the previously assigned IoT hub.
     * Device data was removed from the previously assigned IoT hub.
     */
    PROVISIONING_REGISTRATION_SUB_STATUS_DEVICE_DATA_MIGRATED = 2,

    /**
     * Device has been assigned to a different IoT hub and its device data was populated from the initial state stored in the enrollment.
     * Device data was removed from the previously assigned IoT hub.
     */
    PROVISIONING_REGISTRATION_SUB_STATUS_DEVICE_DATA_RESET = 3,
  };

  /** The result of a registration operation. */
  struct device_provisioning_result
  {
    /** The registration id. */
    char *registration_id;

    /** The time when the device originally registered with the service. */
    time_t created_date_time_utc;

    /** The assigned Azure IoT hub. */
    char *assigned_hub;

    /** The Device Id. */
    char *device_id;

    /** The status of the operation. */
    provisioning_registration_status status;

    /** The substatus of the operation. */
    provisioning_registration_sub_status sub_status;

    /** The generation Id. */
    char *generation_id;

    /** The time when the device last refreshed the registration. */
    time_t last_updated_date_time_utc;

    /** Error code. */
    int error_code;

    /** Error message. */
    char *error_message;

    /** The Etag. */
    char *etag;

    /** The custom data returned from the webhook to the device. */
    char *json_payload;
  };

  /** Registration operation status. */
  struct registration_operation_status
  {
    /** The operation Id. */
    char *operation_id;

    /** The device enrollment status. */
    provisioning_registration_status status;

    /** The device registration status. */
    device_provisioning_result registration_state;

    /** the Retry-After header. */
    char *retry_after;
  };

private:
  Preferences &prefs;
  MqttClient mqtt;
  korra_cloud_provisioning_info stored_info = {0};
  registration_operation_status status = {0};
  char *username = NULL;
  size_t username_len = 0;
  uint16_t request_id = 1;
  bool registration_requested = false;
  Timer<> &timer;

  /// Living instance of the KorraCloudProvisioning class. It can be nullptr.
  static KorraCloudProvisioning *_instance;

private:
  void print();
  void load();
  void save();
  void connect(int retries = 3, int delay_ms = 5000);
  provisioning_registration_status parse_status(const char *value);
  provisioning_registration_sub_status parse_sub_status(const char *value);
  void free_status();
  void schedule_query_registration_result(int delay_sec);
};

#endif // BOARD_HAS_INTERNET

#endif // KORRA_CLOUD_PROVISIONING_H_
