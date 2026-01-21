# Info Orbs - ESP32 Multi-Display Widget System

**Info Orbs** is a premium, open-source multi-display desk accessory powered by an ESP32. It features five round GC9A01 displays to show real-time weather, stocks, clocks, and custom data with a sleek, interactive interface.

![Info Orbs Preview](references/weather.png) <!-- Replace with actual high-quality image if available -->

## 🚀 Features

- **Multi-Widget System**: Cycle through various widgets or stick to your favorite.
- **Dynamic Clock**: Support for Normal, Nixie, and Custom image-based clock faces.
- **Financial Tracker**: Real-time stock prices, crypto, and forex powered by Twelve Data.
- **Weather Forecast**: Comprehensive 3-day forecast with Light/Dark mode themes.
- **Global Time**: 5-Zone clock with customizable cities and flags.
- **Smart Connectivity**: Easy WiFi setup via an integrated web portal (WiFiManager).
- **Web Interface**: Manage configuration, upload files to LittleFS, and control the device remotely from your browser.
- **Animations**: Matrix rain effects and interactive "Eyes" animations.
- **Auto-Dimming**: Time-based brightness control for comfortable nighttime use.

## 🛠️ Hardware Specifications

The project is designed around the **ESP32 DevKit V1** and utilizes the **TFT_eSPI** library for high-speed display rendering.

### Components

- **Microcontroller**: ESP32 (DevKit V1)
- **Displays**: 5x GC9A01 1.28" Round LCDs (240x240 resolution)
- **Buttons**: 3x Push buttons for navigation and interaction
- **Storage**: LittleFS for configuration and asset storage

### Pin Mapping

| Component         | ESP32 Pin |
| :---------------- | :-------- |
| **TFT MOSI**      | GPIO 17   |
| **TFT SCLK**      | GPIO 23   |
| **TFT DC**        | GPIO 19   |
| **TFT RST**       | GPIO 18   |
| **Screen 1 CS**   | GPIO 13   |
| **Screen 2 CS**   | GPIO 33   |
| **Screen 3 CS**   | GPIO 32   |
| **Screen 4 CS**   | GPIO 25   |
| **Screen 5 CS**   | GPIO 21   |
| **Button Left**   | GPIO 26   |
| **Button Middle** | GPIO 27   |
| **Button Right**  | GPIO 14   |
| **Busy LED**      | GPIO 2    |

## 💻 Software & Development

Built with **PlatformIO**, the project uses a modular architecture for easy extension.

### Core Libraries

- `TFT_eSPI`: Optimized display driver.
- `ArduinoJson`: JSON parsing for APIs.
- `TJpg_Decoder`: Efficient JPEG rendering.
- `WiFiManager`: Captive portal for WiFi configuration.
- `PubSubClient`: MQTT support.

### Setup Instructions

1.  **Clone the Repository**:
    ```bash
    git clone https://github.com/jvalver1/info-orbs_montykona.git
    ```
2.  **Environment Setup**:
    - Open the project in **Visual Studio Code** with the **PlatformIO** extension.
3.  **Configuration**:
    - Navigate to `firmware/config/`.
    - Copy `config.h.template` to `config.h`.
    - Edit `config.h` to set your initial preferences (Timezone, Weather location, etc.).
4.  **Flash Firmware**:
    - Connect your ESP32.
    - Run the `Upload` task in PlatformIO.
    - _Note: Assets like fonts and images are automatically embedded or uploaded via scripts._

## 🎮 Usage

- **Short Press (Middle)**: Toggle widget-specific modes (e.g., Temperature High/Low, Clock Style).
- **Medium Press (Middle)**: Toggle settings (e.g., 12/24h format).
- **Navigation**: Use Left/Right buttons to switch between active widgets.
- **Web Portal**: Access `http://info-orbs.local` (or the IP shown on boot) to configure the device via your browser.

---

## 🤝 Contributing

Contributions are welcome! Whether it's adding a new widget, improving animations, or fixing bugs, feel free to open a Pull Request.

## 📜 License

This project is open-source. Please check the `LICENSE.txt` file for details.

---

_Created with ❤️ by the Info Orbs Community._
