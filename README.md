
# ESP32 Quality of Life (ESP32QoL) Library

## Overview

The **ESP32 Quality of Life (ESP32QoL)** library simplifies Over-The-Air (OTA) update management for ESP32 devices. It provides a robust framework for downloading, applying, and validating firmware updates, leveraging ESP32's rollback features for enhanced reliability.

## Features

- **OTA Update Management**: Download and apply firmware updates from HTTP/HTTPS URLs.
- **Version Checking**: Update only if the firmware version differs from the current version.
- **Rollback Support**: Automatically roll back to the previous firmware if the new one fails.
- **Validation Mechanism**: Mark firmware as valid or invalid after deployment.
- **HTTP Redirect Handling**: Follow HTTP redirects during firmware download.
- **Logging Support**: Optional logging for debugging and progress tracking.

## Installation

1. Clone the repository:
   ```bash
   git clone https://github.com/your-repo/ESP32-QualityOfLife.git
   ```
2. Copy the `src` and `include` folders into your ESP32 project.
3. Include the `ESP32QoL.h` header in your project.

## Usage

### Basic OTA Update

To perform an OTA update and restart the device upon success:
```cpp
#include "ESP32QoL.h"

void setup() {
    performOTAUpdate("https://example.com/firmware.bin");
}
```

### OTA Update with Version Checking

To update only if the firmware version differs:
```cpp
#include "ESP32QoL.h"

void setup() {
    OTAUpdate.begin("1.0.1", "https://example.com/firmware.bin");
}
```

### Mark Firmware as Valid

After verifying the new firmware works correctly:
```cpp
#include "ESP32QoL.h"

void loop() {
    OTAUpdate.markAsValid();
}
```

### Rollback Firmware

To force a rollback to the previous firmware:
```cpp
#include "ESP32QoL.h"

void loop() {
    OTAUpdate.markAsInvalid();
}
```

## Logging

Enable logging by defining `ESP32QOL_LOG` in your project:
```cpp
#define ESP32QOL_LOG
#include "ESP32QoL.h"
```

## Dependencies

- [Arduino Core for ESP32](https://github.com/espressif/arduino-esp32)
- WiFi library
- Preferences library
- HTTPClient library
- Update library

## Contributing

Contributions are welcome! Please fork the repository and submit a pull request with your changes.

## License

This project is licensed under the MIT License. See the `LICENSE` file for details.

## Author

Developed by **yunarta** on 3/17/25.
