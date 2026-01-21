# Info Orbs - ESP32 Multi-Display Widget System

**Info Orbs** is a premium, open-source multi-display desk accessory powered by an ESP32. It features five round GC9A01 displays to show real-time weather, stocks, clocks, and custom data with a sleek, interactive interface.

![Info Orbs Preview](references/weather.png)

## 🚀 Key Features

- **Multi-Widget Architecture**: Seamlessly switch between different specialized widgets.
- **Interactive Control**: Three physical buttons for navigation and mode switching.
- **Web-Based Management**: Full device control and configuration via a browser-based portal.
- **High Performance**: Optimized rendering using the `TFT_eSPI` library and LittleFS asset management.
- **Customization**: Support for custom fonts (TTF), icons, and image-based clock faces.

---

## 🧩 Widgets Overview

Each widget can be toggled and configured through the web interface or `config.h`.

### 🕰️ Clock Widget

The core of the system, offering multiple visual styles.

- **Modes**:
  - **Normal**: Classic digital clock using TTF fonts (DSEG7, Roboto, etc.).
  - **Nixie**: Specialized graphics mimicking vintage Nixie tubes.
  - **Custom (0-9)**: User-provided images (0.jpg to 11.jpg) stored in LittleFS.
- **Interaction**:
  - **Short Press (Middle)**: Cycle through valid clock types (Normal -> Nixie -> Custom).
  - **Medium Press (Middle)**: Toggle between 12-hour (AM/PM) and 24-hour formats.
- **Settings**: Customizable colors, shadows, personal Nixie colors, and second-tick indicators.

### 🌤️ Weather Widget

Provides current conditions and a 3-day forecast.

- **Themes**: Light and Dark modes.
- **Data Feeds**: Supports Visual Crossing (default), OpenWeatherMap, and Tempest.
- **Interaction**:
  - **Short Press (Middle)**: Toggle between High/Low temperature display on the forecast.
  - **Medium Press (Middle)**: Manually trigger a weather data refresh.
- **Settings**: Unit selection (Metric/Imperial), city name, and auto-cycling interval for Highs/Lows.

### 📈 Stock Widget

Track your favorite assets in real-time.

- **Market Support**: Stocks, Crypto (via `/USD`), and Forex (via `/EUR`).
- **Data Provider**: Powered by Twelve Data.
- **Interaction**:
  - **Short Press (Middle)**: Force update prices.
- **Settings**: Customizable list of up to 5 tickers and choice between price or percentage change display.

### 🌍 5-Zone Clock

A global perspective on time.

- **Display**: Shows 5 different timezones across the screens with city names and flags.
- **Settings**: Configure city name, timezone identifier (e.g., `Europe/London`), UTC offset, and country flag (emoji or code).

### 👁️ Eyes Widget

Add some personality to your desk with animated eyes that look around.

- **Toggle**: Can be enabled as an active widget in the rotation.

---

## 🌐 Web Server & Configuration

Info Orbs hosts a powerful web server accessible via your local network.

### Accessing the Portal

Connect your device to WiFi. Once connected, access the interface at:

- **mDNS**: `http://info-orbs.local`
- **IP Address**: Check the serial output or the welcome screen on boot.

### Features

1. **Configuration Portal (`/param`)**:
   - Change WiFi credentials.
   - Configure global settings (Timezone, Language, NTP server, Rotation).
   - Adjust TFT brightness and Night Mode dimming hours.
   - Enable/Disable specific widgets and customize their individual settings.
2. **File Manager (`/browse`)**:
   - **Upload/Download**: Manage assets like custom clock images directly from your browser.
   - **Custom Clock Setup**: Copy images to `/CustomClock0/`, `/CustomClock1/`, etc., to create your own clock faces.
   - **URL Fetcher**: Input a URL (e.g., a GitHub folder) to automatically download set of clock digits (0-11.jpg).
3. **Remote Control (`/buttons`)**:
   - A virtual UI to simulate physical button presses (`Short`, `Medium`, `Long`) from any device.

---

## 🛠️ Hardware Mapping

| Component         | ESP32 Pin | Component       | ESP32 Pin |
| :---------------- | :-------- | :-------------- | :-------- |
| **SDA (MOSI)**    | GPIO 17   | **Screen 1 CS** | GPIO 13   |
| **SCLK**          | GPIO 23   | **Screen 2 CS** | GPIO 33   |
| **DC**            | GPIO 19   | **Screen 3 CS** | GPIO 32   |
| **RST**           | GPIO 18   | **Screen 4 CS** | GPIO 25   |
| **Button Left**   | GPIO 26   | **Screen 5 CS** | GPIO 21   |
| **Button Middle** | GPIO 27   | **Busy LED**    | GPIO 2    |
| **Button Right**  | GPIO 14   |                 |           |

---

## 💻 Development & Flashing

Detailed instructions can be found in the [Firmware Install Guide](references/Firmware%20Install%20Guide.md).

1.  **Preparation**: Copy `firmware/config/config.h.template` to `config.h`.
2.  **Environment**: Use **PlatformIO** in VS Code.
3.  **Deploy**: Run the `Upload` task. Assets in the `data` directory are handled by automated scripts.

---

## 🤝 Community & Support

- **Discord**: Join us for setup help and contribution discussions [here](https://link.brett.tech/discord).
- **YouTube**: Watch the assembly and flashing guide [here](https://link.brett.tech/orbsYT).

_Created with ❤️ by the Info Orbs Community._
