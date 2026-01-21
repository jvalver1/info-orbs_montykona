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

---

### ⚙️ Configuration Portal (`/param`)

The configuration portal allows you to fine-tune every aspect of your Info Orbs. Settings are categorized into sections. Some parameters are hidden by default and can be revealed by clicking **"Show Advanced Parameters"**.

#### 📋 General Settings

- **Timezone Location**: Set your IANA timezone string (e.g., `Europe/Berlin`, `America/New_York`).
- **Language**: Choose the display language (English, German, Spanish, French, etc.).
- **Widget Cycle Delay**: Time in seconds before automatically switching to the next active widget (set to `0` to disable auto-cycling).
- **[ADV] NTP Server**: The address of the time server (default: `pool.ntp.org`).

#### 📺 TFT Settings

- **Orb Rotation**: Rotate the screen orientation (0°, 90°, 180°, 270°).
- **Night Mode**: Enable automatic dimming during specific hours.
- **[ADV] TFT Brightness**: Global brightness level (0-255).
- **[ADV] Night Mode Start/End**: Define the window for reduced brightness.
- **[ADV] Night Mode Brightness**: Lower brightness level for night hours (0-255).
- **[ADV] Debug Output**: Enable detailed logging to the Serial monitor.

#### 🕰️ Clock Widget

- **Default Type**: Select the preferred clock style (Normal, Nixie, or Custom).
- **Clock Format**: Toggle between 24h and 12h (AM/PM).
- **[ADV] Colors**: Customize foreground and shadow colors.
- **[ADV] Shadowing**: Toggle segment shadowing for digital fonts.
- **[ADV] Custom Clocks**: Enable/Disable specific folders (`CustomClock0` to `CustomClock9`) and customize their individual color tints and tick colors.

#### 🌤️ Weather Widget

- **Weather Location**: Set your city or coordinates (Visual Crossing).
- **[ADV] Units**: Toggle between Metric and Imperial systems.
- **[ADV] Theme**: Choose between Light and Dark visual modes.
- **[ADV] High/Low Cycle**: Speed of the forecast temperature toggle.
- **[ADV] Feed-Specifics**:
  - **OpenWeatherMap**: Requires `Latitude`, `Longitude`, and a `Display Name`.
  - **Tempest**: Requires a `Station ID` and `Station Name`.

#### 📈 Stock & Portfolio Settings

- **Enabled**: Toggle the Stock and Parqet widgets.
- **Stock List**: Enter comma-separated tickers (e.g., `AAPL,TSLA,BTC/USD,EUR/GBP`).
- **[ADV] Change Format**: Toggle between Price Change (absolute) and Percentage Change.
- **[ADV] Portfolio ID**: Your Parqet portfolio UID.
- **[ADV] Portfolio Proxy**: URL of the Parqet data proxy.
- **[ADV] Portfolio Views**: Customize default timeframes, performance measures, and chart types.

#### 🧩 Widget Specifics

- **Eyes**: Customize Sclera, Iris, Pupil, and Eyelid colors; adjust blink and movement frequency.
- **Matrix**: Toggle font size, colors, and falling speed.
- **Global Time**: Configure up to 5 custom timezones with city names and country flags (referenced by 2-letter ISO code like `US`, `GB`, `ES`).
- **MQTT**:
  - **Connection**: Set your Broker Host, Port, User, and Password.
  - **Setup Topic**: The MQTT topic that provides the initial JSON configuration for the widget.
- **Web Data**: Point to any JSON endpoint to pull custom metrics onto the round screens.

> [!IMPORTANT]
> **Apply Changes**: After making any changes in the `/param` page, you must click **Save**. The device will automatically restart to apply the new configuration.

---

### 📂 File Manager (`/browse`)

The integrated file manager provides direct access to the device's LittleFS flash storage.

- **Navigation**: Click on folders to enter; use the "Back" button to return.
- **Asset Management**: Upload new images or delete existing files.
- **Image Previews**: Hover or look at thumbnails for JPG/JPEG files.
- **URL Fetcher**:
  - Automatically download a complete set of clock digits (`0.jpg` to `11.jpg`) into a folder.
  - Supports direct links and GitHub repository URLs (automatically converts to raw content links).
- **Custom Clock Setup**: To create a "Custom Clock", upload 12 images (`0.jpg` through `11.jpg`) into a folder named `/CustomClock0/`, `/CustomClock1/`, etc.

---

### 🎮 Remote Control (`/buttons`) & API

Control your Orbs from your phone or PC through a virtual interface or programmatically.

- **Simulation**: Trigger `Short`, `Medium`, and `Long` presses for the `Left`, `Middle`, and `Right` buttons.
- **API Access**: Developers can trigger button events or retrieve status via simple GET requests:
  - `GET /button?name=right&state=short`
  - `GET /browse?dir=/` (List files as HTML)
  - `POST /fetchFromUrl` (Trigger asset download)

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

1. **Preparation**: Copy `firmware/config/config.h.template` to `config.h`.
2. **Environment**: Use **PlatformIO** in VS Code.
3. **Deploy**: Run the `Upload` task. Assets in the `data` directory are handled by automated scripts.

---

## 🤝 Community & Support

- **Discord**: Join us for setup help and contribution discussions [here](https://link.brett.tech/discord).
- **YouTube**: Watch the assembly and flashing guide [here](https://link.brett.tech/orbsYT).

_Created with ❤️ by the Info Orbs Community._
