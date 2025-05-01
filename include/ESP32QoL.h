/**
 * @file ESP32QoL.h
 * @author yunarta
 * @date 3/17/25
 * @brief ESP32 Quality of Life library for Over-The-Air (OTA) update management
 *
 * This library provides functions to simplify OTA updates for ESP32 devices,
 * including downloading firmware, applying updates, and managing the update
 * validation process using ESP32's rollback features.
 */

#ifndef ESP32QOL_H
#define ESP32QOL_H

#include <Arduino.h>

/**
 * @brief Downloads and applies a firmware update, then restarts the device
 *
 * This function downloads a firmware binary from the specified URL, applies it,
 * and automatically restarts the device if the update was successful.
 *
 * @param url The URL (HTTP/HTTPS) of the firmware binary
 */
void performOTAUpdate(const String &url);

/**
 * @brief Downloads and applies a firmware update without restarting the device
 *
 * This function downloads a firmware binary from the specified URL and applies it
 * but does not restart the device automatically.
 *
 * @param url The URL (HTTP/HTTPS) of the firmware binary
 * @return true if the update was successfully downloaded and applied
 * @return false if the update failed
 */
bool performOTAUpdateOnly(const String &url);

/**
 * @brief Class for managing OTA update processes with version checking
 *
 * This class provides methods to initiate OTA updates with version checking,
 * and to interact with ESP32's rollback safety features.
 */
class OTAUpdateClass {
public:
    /**
     * @brief Initiates an OTA update if the provided version differs from the current one
     *
     * Checks if the current firmware version differs from the specified one and,
     * if so, downloads and applies a new firmware from the provided URL. After updating,
     * it marks the update as pending validation and restarts the device.
     *
     * @param appVersion The version string of the new firmware
     * @param url The URL of the firmware binary
     */
    void begin(const String &appVersion, const String &url) const;

    /**
     * @brief Marks the current firmware as valid to prevent rollback
     *
     * This should be called after a successful boot with the new firmware
     * to prevent automatic rollback to the previous version.
     */
    void markAsValid() const;

    /**
     * @brief Marks the current firmware as invalid and initiates rollback
     *
     * Forces the device to roll back to the previous firmware version
     * if rollback is available.
     */
    void markAsInvalid() const;
};

/**
 * @brief Global instance of OTAUpdateClass
 *
 * Use this instance to access OTA update functionality.
 */
extern const OTAUpdateClass OTAUpdate;

#endif //ESP32QOL_H
