/**
 * @file ESP32QoL.cpp
 * @author yunarta
 * @date 3/17/25
 * @brief Implementation of ESP32 Quality of Life library for OTA updates
 * 
 * This file implements the functions declared in ESP32QoL.h for downloading,
 * applying, and validating OTA updates on ESP32 devices.
 */

#include "ESP32QoL.h"

#include <Update.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <Preferences.h>
#include <esp_ota_ops.h>

/**
 * Global instance of the OTAUpdateClass
 */
const OTAUpdateClass OTAUpdate;

/**
 * Downloads and applies an OTA update, then restarts the device if successful
 *
 * @param url URL of the firmware binary
 */
void performOTAUpdate(const String &url) {
    if (performOTAUpdateOnly(url) && Update.isFinished()) {
        ESP.restart();
    }
}

/**
 * Handles HTTP redirects and returns the final HTTP response code
 *
 * @param http Reference to the HTTPClient object
 * @param httpClient Reference to the WiFiClientSecure
 * @return Final HTTP response code after following all redirects
 */
int handleHttpRedirects(HTTPClient &http, WiFiClientSecure &httpClient) {
    int httpCode = http.GET();

    // HTTP redirect status codes
    constexpr int redirectCodes[] = {
        HTTP_CODE_MOVED_PERMANENTLY,
        HTTP_CODE_FOUND,
        HTTP_CODE_SEE_OTHER,
        HTTP_CODE_TEMPORARY_REDIRECT,
        HTTP_CODE_PERMANENT_REDIRECT
    };

    // Follow redirects
    for (int i = 0; i < 10; i++) {
        // Limit to 10 redirects to avoid infinite loops
        bool isRedirect = false;

        // Check if the current code is a redirect code
        for (const int code: redirectCodes) {
            if (httpCode == code) {
                isRedirect = true;
                break;
            }
        }

        if (!isRedirect) {
            break;
        }

        String redirectUrl = http.header("Location");
        if (redirectUrl.isEmpty()) {
            return HTTP_CODE_BAD_REQUEST;
        }

        http.end();
        http.begin(httpClient, redirectUrl);
        httpCode = http.GET();
    }

    return httpCode;
}

/**
 * Downloads and applies the firmware update
 *
 * @param http Reference to the HTTPClient with active connection
 * @return true if firmware was successfully downloaded and applied
 */
bool downloadAndApplyUpdate(HTTPClient &http) {
    int contentLength = http.getSize();
    if (contentLength <= 0) {
#ifdef ESP32QOL_LOG
        Serial.println("Invalid firmware size");
#endif
        return false;
    }

    if (!Update.begin(contentLength)) {
#ifdef ESP32QOL_LOG
        Serial.println("Failed to begin update");
#endif
        return false;
    }

    WiFiClient *stream = http.getStreamPtr();
    size_t written = 0;
    uint8_t buff[128];

#ifdef ESP32QOL_LOG
    int lastProgress = -10;
#endif
    // Download and write firmware
    while (written < (size_t) contentLength) {
        size_t size = stream->available();
        if (size) {
            size_t read = stream->readBytes(buff, min(sizeof(buff), size));
            written += Update.write(buff, read);
#ifdef ESP32QOL_LOG
            int progress = (written * 100) / contentLength;
            if (progress >= lastProgress + 5) {
                Serial.printf("Update progress: %d%%\n", progress);
                lastProgress = progress;
            }
#endif
        }
    }

    bool success = Update.end();
#ifdef ESP32QOL_LOG
    Serial.println(success ? "Update completed" : "Update failed");
#endif
    return success;
}

/**
 * Downloads and applies an OTA update without restarting
 *
 * This function handles:
 * - HTTPS connections (without certificate verification)
 * - HTTP redirects (301, 302, 303, 307, 308)
 * - Streaming the firmware to the update partition
 *
 * @param url URL of the firmware binary
 * @return true if update was successfully downloaded and applied
 * @return false if the update failed at any stage
 */
bool performOTAUpdateOnly(const String &url) {
    WiFiClientSecure httpClient;
    HTTPClient http;

#ifdef ESP32QOL_LOG
    Serial.println("Starting update from: " + url);
#endif

    httpClient.setInsecure(); // Use HTTPS without SSL verification (for testing)
    http.begin(httpClient, url);

    // Handle redirects and get final HTTP code
    int httpCode = handleHttpRedirects(http, httpClient);

#ifdef ESP32QOL_LOG
    if (httpCode != HTTP_CODE_OK) {
        Serial.printf("HTTP error code: %d\n", httpCode);
    }
#endif

    bool success = false;
    if (httpCode == HTTP_CODE_OK) {
        success = downloadAndApplyUpdate(http);
    }

    http.end();
    return success;
}

/**
 * Initiates an OTA update with version checking
 *
 * Only downloads and applies the update if the current version
 * differs from the target version. Uses Preferences storage
 * to track the current version and validation state.
 *
 * @param appVersion Target version string
 * @param url URL of the firmware binary
 */
void OTAUpdateClass::begin(const String &appVersion, const String &url) const {
    Preferences preferences;
    preferences.begin("OTAUpdate", false);

    String currentVersion = preferences.getString("appVersion", "");
    if (!currentVersion.equalsIgnoreCase(appVersion)) {
#ifdef ESP32QOL_LOG
        Serial.printf("Version mismatch - current: %s, target: %s\n", 
                     currentVersion.c_str(), appVersion.c_str());
#endif
        if (performOTAUpdateOnly(url) && Update.isFinished()) {
#ifdef ESP32QOL_LOG
            Serial.println("Update successful, restarting");
#endif
            preferences.putString("appVersion", appVersion);
            preferences.putBool("pendingValidation", true);
            ESP.restart();
        }
    }
    preferences.end();
}

/**
 * Marks the current firmware as valid
 *
 * This cancels any potential rollback and should be called
 * after verifying that the new firmware is functioning correctly.
 * Only performs the action if pendingValidation is true.
 */
void OTAUpdateClass::markAsValid() const {
    Preferences preferences;
    preferences.begin("OTAUpdate", false);
    if (preferences.getBool("pendingValidation", false)) {
        esp_ota_mark_app_valid_cancel_rollback();
        preferences.putBool("pendingValidation", false);
    }
    preferences.end();
}

/**
 * Marks the current firmware as invalid and triggers rollback
 *
 * This forces the device to roll back to the previous firmware
 * version if rollback is available. Typically used when the
 * new firmware is determined to be malfunctioning.
 */
void OTAUpdateClass::markAsInvalid() const {
    if (esp_ota_check_rollback_is_possible()) {
        esp_ota_mark_app_invalid_rollback_and_reboot();
    }
}
