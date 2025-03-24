//
// Created by yunarta on 3/17/25.
//

#include "ESP32QoL.h"

#include <Update.h>
#include <HTTPClient.h>

#include <WiFi.h>
#include <Preferences.h>
#include <esp_ota_ops.h>

OTAUpdateClass OTAUpdate;

void performOTAUpdate(String url) {
#ifdef LOG_INFO
    Serial.println(F("[INFO] Starting OTA Update..."));
#endif
    if (performOTAUpdateOnly(url) && Update.isFinished()) {
#ifdef LOG_INFO
        Serial.println(F("[INFO] OTA Update Successful! Restarting..."));
#endif
        ESP.restart();
    } else {
#ifdef LOG_INFO
        Serial.println(F("[INFO] OTA Update Failed!"));
#endif
    }
}

bool performOTAUpdateOnly(String url) {
    WiFiClientSecure httpClient;
    httpClient.setInsecure(); // Use HTTPS without SSL verification (for testing)
    HTTPClient http;

#ifdef LOG_INFO
    Serial.println(F("[INFO] Starting OTA Update process..."));
#endif
    http.begin(httpClient, url);
    int httpCode = http.GET();

    // Follow redirect logic
    while (httpCode == HTTP_CODE_MOVED_PERMANENTLY || httpCode == HTTP_CODE_FOUND ||
           httpCode == HTTP_CODE_SEE_OTHER || httpCode == HTTP_CODE_TEMPORARY_REDIRECT ||
           httpCode == HTTP_CODE_PERMANENT_REDIRECT) {
        String newUrl = http.header("Location");
        if (newUrl.isEmpty()) {
#ifdef LOG_INFO
            Serial.println(F("[INFO] Redirection failed: no Location header!"));
#endif
            return false;
        }

#ifdef LOG_DEBUG
        Serial.printf("[DEBUG] Redirecting to: %s\n", newUrl.c_str());
#endif
        http.end();
        http.begin(httpClient, newUrl);
        httpCode = http.GET();
    }

    if (httpCode == HTTP_CODE_OK) {
        int contentLength = http.getSize();
        if (contentLength <= 0) {
#ifdef LOG_INFO
            Serial.println(F("[INFO] No content received, aborting."));
#endif
            return false;
        }

        if (!Update.begin(contentLength)) {
#ifdef LOG_INFO
            Serial.println(F("[INFO] Not enough space for OTA update."));
#endif
            return false;
        }

        WiFiClient *stream = http.getStreamPtr();
        size_t written = 0;
        while (written < (size_t) contentLength) {
            size_t size = stream->available();
            if (size) {
                uint8_t buff[128];
                int read = stream->readBytes(buff, ((size > sizeof(buff)) ? sizeof(buff) : size));
                written += Update.write(buff, read);
            }
        }

        if (Update.end()) {
#ifdef LOG_INFO
            Serial.println(F("[INFO] OTA Update complete."));
#endif
        } else {
#ifdef LOG_INFO
            Serial.println(F("[INFO] OTA Update failed."));
#endif
            return false;
        }
    } else {
#ifdef LOG_INFO
        Serial.printf("[INFO] HTTP Request Failed, Error: %d\n", httpCode);
#endif
        return false;
    }

    http.end();
    return true;
}

void OTAUpdateClass::begin(const String &appVersion, const String &url) {
    Preferences preferences;
    preferences.begin("OTAUpdate", false);
    String currentVersion = preferences.getString("appVersion", "");
#ifdef LOG_DEBUG
    Serial.printf("[DEBUG] Current app version: %s\n", currentVersion.c_str());
    Serial.printf("[DEBUG] Target app version: %s\n", appVersion.c_str());
#endif
    if (!currentVersion.equalsIgnoreCase(appVersion)) {
#ifdef LOG_INFO
        Serial.println(F("[INFO] App version mismatch, starting OTA Update..."));
#endif
        if (performOTAUpdateOnly(url) && Update.isFinished()) {
#ifdef LOG_INFO
            Serial.println(F("[INFO] OTA Update Successful! Restarting..."));
#endif
            preferences.putString("appVersion", appVersion);
            preferences.putBool("pendingValidation", true);
            ESP.restart();
        } else {
#ifdef LOG_INFO
            Serial.println(F("[INFO] OTA Update Failed!"));
#endif
        }
    } else {
#ifdef LOG_INFO
        Serial.println(F("[INFO] App version is up-to-date, no update needed."));
#endif
    }
    preferences.end();
}

void OTAUpdateClass::markAsValid() {
#ifdef LOG_INFO
    Serial.println(F("[INFO] Marking OTA update as valid..."));
#endif
    Preferences preferences;
    preferences.begin("OTAUpdate", false);
    if (preferences.getBool("pendingValidation", false)) {
        esp_ota_mark_app_valid_cancel_rollback();
        preferences.putBool("pendingValidation", false);
    }
    preferences.end();
}

void OTAUpdateClass::markAsInvalid() {
#ifdef LOG_INFO
    Serial.println(F("[INFO] Checking if rollback is possible..."));
#endif
    if (esp_ota_check_rollback_is_possible()) {
#ifdef LOG_INFO
        Serial.println(F("[INFO] Marking OTA update as invalid and initiating rollback..."));
#endif
        esp_ota_mark_app_invalid_rollback_and_reboot();
    } else {
#ifdef LOG_INFO
        Serial.println(F("[INFO] Rollback is not possible."));
#endif
    }
}
