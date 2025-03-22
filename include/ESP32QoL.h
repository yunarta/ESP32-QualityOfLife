//
// Created by yunarta on 3/17/25.
//

#ifndef ESP32QOL_H
#define ESP32QOL_H

#include <Arduino.h>

void performOTAUpdate(String url);

bool performOTAUpdateOnly(String url);

class OTAUpdateClass {
public:
    void begin(const String &appVersion, const String &url);

    void markAsValid();

    void markAsInvalid();
} extern OTAUpdate;

#endif //ESP32QOL_H
