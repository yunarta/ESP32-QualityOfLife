//
// Created by yunarta on 3/17/25.
//

#include "ESP32QoL.h"

#include <Update.h>
#include <HTTPClient.h>

void performOTAUpdate(String url) {
    WiFiClientSecure httpClient;
    httpClient.setInsecure();  // Use HTTPS without SSL verification (for testing)
    HTTPClient http;

    Serial.println("Starting OTA Update...");
    http.begin(httpClient, url);
    int httpCode = http.GET();

    if (httpCode == HTTP_CODE_OK) {
        int contentLength = http.getSize();
        if (contentLength <= 0) {
            Serial.println("No content received, aborting.");
            return;
        }

        if (!Update.begin(contentLength)) {
            Serial.println("Not enough space for OTA update.");
            return;
        }

        WiFiClient *stream = http.getStreamPtr();
        size_t written = 0;
        while (written < contentLength) {
            size_t size = stream->available();
            if (size) {
                uint8_t buff[128];
                int read = stream->readBytes(buff, ((size > sizeof(buff)) ? sizeof(buff) : size));
                written += Update.write(buff, read);
            }
        }

        if (Update.end()) {
            Serial.println("OTA Update Successful! Restarting...");
            ESP.restart();
        } else {
            Serial.println("OTA Update Failed!");
        }
    } else {
        Serial.printf("HTTP Request Failed, Error: %d\n", httpCode);
    }
    http.end();
}