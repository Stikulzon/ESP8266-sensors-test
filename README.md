# ESP8266 PC Status & Environmental Monitor

A dual-purpose monitoring station powered by an ESP8266. It displays real-time PC statistics (CPU/GPU Temperature, Fan Speed) pushed from a Python client and local environmental data (Room Temp, Humidity, Pressure) measured by a BME280 sensor.

## 🌟 Features

*   **Real-time PC Monitoring:** Displays CPU Temp, GPU Temp, and Fan Speed (Linux only).
*   **Environmental Data:** Shows Room Temperature, Humidity, and Atmospheric Pressure via BME280.
*   **Connection Status:** Visual indicator on the LCD showing if the PC client is "Online" or "Offline".
*   **WiFi Enabled:** Hosted HTTP server accepts JSON payloads via REST API.
*   **Security:** Simple API Key authentication to prevent unauthorized data pushes.

## 🛠️ Hardware Required

1.  **ESP8266 Board** (NodeMCU, Wemos D1 Mini, etc.)
2.  **ST7735 TFT Display** (1.8" or similar SPI display)
3.  **BME280 Sensor** (I2C)
4.  **Connecting Wires**
5.  **Linux PC** (for the client script)

## 🔌 Wiring

### ST7735 TFT Display (SPI)
| TFT Pin     | ESP8266 Pin | Definition |
|:------------| :--- | :--- |
| **VCC**     | 3.3V | Power |
| **GND**     | GND | Ground |
| **CS**      | D8 (GPIO15) | Chip Select |
| **RESET**   | D4 (GPIO2) | Reset |
| **A0 / DC** | D3 (GPIO0) | Data/Command |
| **SDA**     | D7 (GPIO13) | MOSI (Hardware SPI) |
| **SCL**     | D5 (GPIO14) | SCLK (Hardware SPI) |
| **BLK**     | 3.3V | Backlight |

### BME280 Sensor (I2C)
| Sensor Pin | ESP8266 Pin |
| :--- | :--- |
| **VCC** | 3.3V |
| **GND** | GND |
| **SCL** | D1 (GPIO5) |
| **SDA** | D2 (GPIO4) |

---

## 💾 Installation

### 1. ESP8266 Firmware (`firmware.ino`)

1.  Open `firmware.ino` in the **Arduino IDE**.
2.  Install the following libraries via the Library Manager:
    *   `Adafruit GFX Library`
    *   `Adafruit ST7735 and ST7789 Library`
    *   `Adafruit BME280 Library`
    *   `Adafruit Unified Sensor`
    *   `ArduinoJson` (by Benoit Blanchon)
3.  **Configuration:** Edit the top of the file with your settings:
    ```cpp
    #define WIFI_SSID       "Your_WiFi_Name"
    #define WIFI_PASS       "Your_WiFi_Password"
    #define API_KEY         "Set_A_Secret_Key"
    ```
4.  Upload the code to your ESP8266.
5.  Open the Serial Monitor (115200 baud) to note the **IP Address** assigned to the ESP.

### 2. PC Client (`client.py`)

*Note: This script is designed for **Linux** systems using `lm-sensors`.*

1.  Install the required Python library:
    ```bash
    pip install requests
    ```
2.  Install system sensors:
    ```bash
    sudo apt-get install lm-sensors
    sudo sensors-detect  # Answer 'yes' to detect chips
    ```
3.  **Configuration:** Open `client.py` and edit the settings block:
    ```python
    # --- SETTINGS ---
    ESP_IP = "192.168.1.XXX"  # The IP address from the ESP Serial Monitor
    ESP_PORT = 8085
    API_KEY = "Set_A_Secret_Key" # Must match the key in firmware.ino
    ```

## 🚀 Usage

1.  Power on the ESP8266. It will display the local BME280 data immediately.
2.  Run the Python script on your PC:
    ```bash
    python3 client.py
    ```
3.  The LCD should update to show "Online" and display your PC temperatures.
4.  To run the script in the background, you can use `nohup` or create a systemd service.

## 🐛 Troubleshooting

*   **BME280 Error:** Ensure the I2C address in the code (`0x76`) matches your sensor. Some BME280 sensors use `0x77`.
*   **"Offline" Status:** Check if the PC and ESP are on the same network. Check if the `API_KEY` matches in both files.
*   **0°C Temperatures:** Run `sensors` in your terminal. If your hardware isn't detected, you may need to run `sudo sensors-detect` again or configure `lm-sensors` for your specific motherboard.

## 📄 License
Open Source. Feel free to modify and adapt.