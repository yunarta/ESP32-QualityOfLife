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
    Serial.println(F("[INFO] Starting OTA Update..."));
    if (performOTAUpdateOnly(url) && Update.isFinished()) {
        Serial.println(F("[INFO] OTA Update Successful! Restarting..."));
        ESP.restart();
    } else {
        Serial.println(F("[INFO] OTA Update Failed!"));
    }
}

bool performOTAUpdateOnly(String url) {
    WiFiClientSecure httpClient;
    httpClient.setInsecure(); // Use HTTPS without SSL verification (for testing)
    HTTPClient http;

    Serial.println(F("[INFO] Starting OTA Update process..."));
    http.begin(httpClient, url);
    int httpCode = http.GET();

    // Follow redirect logic
    while (httpCode == HTTP_CODE_MOVED_PERMANENTLY || httpCode == HTTP_CODE_FOUND ||
           httpCode == HTTP_CODE_SEE_OTHER || httpCode == HTTP_CODE_TEMPORARY_REDIRECT ||
           httpCode == HTTP_CODE_PERMANENT_REDIRECT) {
        String newUrl = http.header("Location");
        if (newUrl.isEmpty()) {
            Serial.println(F("[INFO] Redirection failed: no Location header!"));
            return false;
        }

        Serial.printf_P(PSTR("[DEBUG] Redirecting to: %s\n"), newUrl.c_str());
        http.end();
        http.begin(httpClient, newUrl);
        httpCode = http.GET();
    }

    if (httpCode == HTTP_CODE_OK) {
        int contentLength = http.getSize();
        if (contentLength <= 0) {
            Serial.println(F("[INFO] No content received, aborting."));
            return false;
        }

        if (!Update.begin(contentLength)) {
            Serial.println(F("[INFO] Not enough space for OTA update."));
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
            Serial.println(F("[INFO] OTA Update complete."));
        } else {
            Serial.println(F("[INFO] OTA Update failed."));
            return false;
        }
    } else {
        Serial.printf_P(PSTR("[INFO] HTTP Request Failed, Error: %d\n"), httpCode);
        return false;
    }

    http.end();
    return true;
}

void OTAUpdateClass::begin(const String &appVersion, const String &url) {
    Preferences preferences;
    preferences.begin("OTAUpdate", false);
    String currentVersion = preferences.getString("appVersion", "");
    Serial.printf_P(PSTR("[DEBUG] Current app version: %s\n"), currentVersion.c_str());
    Serial.printf_P(PSTR("[DEBUG] Target app version: %s\n"), appVersion.c_str());
    if (!currentVersion.equalsIgnoreCase(appVersion)) {
        Serial.println(F("[INFO] App version mismatch, starting OTA Update..."));
        if (performOTAUpdateOnly(url) && Update.isFinished()) {
            Serial.println(F("[INFO] OTA Update Successful! Restarting..."));
            preferences.putString("appVersion", appVersion);
            preferences.putBool("pendingValidation", true);
            ESP.restart();
        } else {
            Serial.println(F("[INFO] OTA Update Failed!"));
        }
    } else {
        Serial.println(F("[INFO] App version is up-to-date, no update needed."));
    }
    preferences.end();
}

void OTAUpdateClass::markAsValid() {
    Serial.println(F("[INFO] Marking OTA update as valid..."));
    Preferences preferences;
    preferences.begin("OTAUpdate", false);
    if (preferences.getBool("pendingValidation", false)) {
        esp_ota_mark_app_valid_cancel_rollback();
        preferences.putBool("pendingValidation", false);
    }
    preferences.end();
}

void OTAUpdateClass::markAsInvalid() {
    Serial.println(F("[INFO] Checking if rollback is possible..."));
    if (esp_ota_check_rollback_is_possible()) {
        Serial.println(F("[INFO] Marking OTA update as invalid and initiating rollback..."));
        esp_ota_mark_app_invalid_rollback_and_reboot();
    } else {
        Serial.println(F("[INFO] Rollback is not possible."));
    }
}
