# Info Orbs - ESP32 Multi-Display Widget System

**Info Orbs** is a premium, open-source multi-display desk accessory powered by an ESP32. It features five round GC9A01 displays to show real-time weather, stocks, clocks, and custom data with a sleek, interactive interface.

![Info Orbs Preview](references/weather.png)

## 🚀 Key Features

- **Multi-Widget Architecture**: Seamlessly switch between different specialized widgets.
- **Interactive Control**: Three physical buttons for navigation and mode switching.
- **Web-Based Management**: Full device control and configuration via a browser-based portal.
- **High Performance**: Optimized rendering using the `TFT_eSPI` library and LittleFS asset management.
- **Customization**: Support for custom fonts (TTF), icons, and image-based clock faces.
- **Dual-Core Stability**: Background networking pinned to Core 0 to keep the UI on Core 1 fully responsive and crash-free.

---

## 🧩 Widgets Overview

Each widget can be toggled and configured through the web interface or `config.h`.

### 🕰️ Clock Widget

The core of the system, offering multiple visual styles.

- **Modes**:
  - **Normal**: Classic digital clock using TrueType fonts (DSEG7, Roboto, etc.).
  - **Nixie**: Specialized graphics mimicking vintage Nixie tubes.
  - **Custom (0–9)**: User-provided images (`0.jpg` to `11.jpg`) stored in LittleFS.
- **Interaction**:
  - **Short Press (Middle)**: Cycle through valid clock types (Normal → Nixie → Custom).
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

### 📊 Parqet Portfolio Widget

Display your investment portfolio performance fetched from a self-hosted Parqet proxy.

- **Supports**: Multiple timeframes, performance measures, and chart types.
- **Settings**: Portfolio UID, proxy URL, and default view configuration.

### 🌍 5-Zone Clock

A global perspective on time across all five screens simultaneously.

- **Display**: Shows 5 different timezones simultaneously with city names, country flags, local time, and GMT offset relative to your local timezone.
- **Business Hours**: Optional colour-coded ring around each screen — green when the city is within working hours, red when outside, with weekend detection.
- **DST-Aware**: Uses POSIX timezone identifiers (e.g. `Europe/London`) and automatically tracks Daylight Saving Time transitions without any external API.
- **Interaction**:
  - **Medium Press (Middle)**: Toggle between 12-hour and 24-hour display.
- **Settings**: City name, IANA timezone string, UTC offset, country flag code (e.g. `GB`, `US`), and business-hours start/end.

### 💬 MQTT Widget

Display arbitrary data pushed from your home automation system.

- **Protocol**: Standard MQTT over TCP.
- **Configuration**: Set broker host, port, username, password, and the setup topic that delivers the initial JSON layout for all five screens.
- **Use Cases**: Home assistant dashboards, sensor readings, alert displays.

### 🌐 Web Data Widget

Pull any JSON endpoint and render its fields directly on the displays.

- **Format**: Expects a JSON response with a `displays` array (one entry per screen), each describing text, colour, and icon.
- **Polling**: Configurable interval; can also be set dynamically by the server via an `interval` field in the response.
- **Use Cases**: Custom metrics, home dashboards, cryptocurrency tickers from a self-hosted proxy.

### 🌱 Matrix Screensaver

A digital rain screensaver inspired by *The Matrix* that activates across all five round screens.

- **Effect**: Multiple independent columns of falling katakana/ASCII characters cascade downward across all five displays simultaneously. Each column has a brighter "head" character that leads the trail.
- **Display**: All five screens are used together as a wide canvas.
- **Settings** (all configurable via the web portal):
  - **Font Size**: Toggle between small and large character size.
  - **Text Color**: Base colour of the falling trail (RGB).
  - **Head Character Color**: Colour of the leading bright character at the top of each column.
  - **Min/Max Lines**: Control the density of active columns on screen.
  - **Min/Max Speed**: Control how fast the columns fall.
  - **Update Interval**: How frequently the animation advances (milliseconds).
- **No interaction**: The widget runs autonomously; use the navigation buttons to cycle away from it.

### 👁️ Eyes Widget

Add some personality to your desk with a pair of animated photorealistic eyes.

- **Animation**:
  - **Pupil Movement**: Eyes randomly glance left, centre, or right at configurable intervals.
  - **Blinking**: Smooth eyelid-slide animation; long blinks occur at random intervals with configurable duration.
- **Layout**: Left eye on Screen 1, right eye on Screen 3. An optional nose graphic is drawn on Screen 2.
- **Assets**: Rendered from embedded JPG images (eye white, iris, eyelid, nose) for a photorealistic look.
- **Settings**: Sclera color, iris color, pupil color, eyelid color; show/hide nose; blink interval range; eye movement interval; long-close duration.

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
- **[ADV] TFT Brightness**: Global brightness level (0–255).
- **[ADV] Night Mode Start/End**: Define the window for reduced brightness.
- **[ADV] Night Mode Brightness**: Lower brightness level for night hours (0–255).
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
- **Matrix**: Toggle font size, colors, and falling speed (min/max lines, min/max speed, update interval).
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
|:------------------|:----------|:----------------|:----------|
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

## ⚙️ Architecture & Stability Improvements

This section documents significant engineering changes made to the firmware to resolve system crashes and improve long-term stability. These changes are relevant for contributors and advanced users who want to understand the internals.

### Dual-Core Task Architecture

**Problem:** The original firmware ran all processing — WiFi management, HTTP requests, widget rendering, and button handling — on a single core in a monolithic loop. This caused the watchdog timer to trip during slow network operations and made the UI unresponsive during data fetches.

**Solution:** The workload is now split across both ESP32 cores:

| Core       | Responsibilities                                                                    |
|------------|-------------------------------------------------------------------------------------|
| **Core 0** | `NetworkTask` — WiFiManager processing, HTTP task execution, task response handling |
| **Core 1** | `loop()` — Button polling, widget update/draw, time sync                            |

The `NetworkTask` is created with `xTaskCreateStaticPinnedToCore()` using a **statically allocated** stack and task buffer. This avoids heap fragmentation during startup and guarantees the task can always be created regardless of runtime heap state.

```
setup() completes on Core 1
  └─ WiFi connects
  └─ Widgets initialized
  └─ NetworkTask spawned on Core 0
       └─ wifiManager->process()
       └─ TaskManager::processAwaitingTasks()
       └─ TaskManager::processTaskResponses()
```

### Button Interrupt Decoupling

**Problem:** Button debouncing was performed directly inside the GPIO interrupt service routine (ISR). Calling `millis()` and executing debounce logic inside an ISR is illegal in FreeRTOS — it can deadlock or produce corrupted timer state.

**Solution:** ISRs now do one thing only: call `vTaskNotifyGiveFromISR()` to wake a dedicated `buttonHandlerTask`. All debounce logic runs in that task's context on Core 1, safely outside the ISR.

```
GPIO IRQ fires (ISR)
  └─ vTaskNotifyGiveFromISR() → wakes buttonHandlerTask
       └─ button.updateState(millis())   ← debounce here, safely
       └─ widgetSet->buttonPressed()
```

### Task Manager Semaphore Fix

**Problem:** The global `taskSemaphore` was taken by one FreeRTOS task and released by a different task. FreeRTOS binary semaphores with ownership violations cause an assertion failure: `xQueueGenericSend queue.c:832`.

**Solution:** The shared semaphore was replaced with a local `taskLimitSemaphore` scoped entirely within `TaskManager`. The semaphore is always taken and given by the same task, eliminating the ownership violation.

### WiFi Logging ISR Crash

**Problem:** The ESP32 WiFi stack logs internal events via `wifi_log()`, which is routed through the `esp-insights` diagnostics wrapper (`__wrap_esp_log_write`). When the WiFi timer ISR fires, it calls `wifi_log()`, which tries to flush `stdio` and acquire the UART mutex. Because `lock_init_generic` cannot initialize a FreeRTOS mutex from within an interrupt context, it calls `abort()`.

The exact crash chain decoded from the backtrace:
```
timer_process_alarm → ieee80211_timer_process → wifi_log
  → __wrap_esp_log_write → vprintf → _fflush_r → uart_write
  → _lock_acquire_recursive → lock_init_generic → abort()
```

**Solution:** WiFi and related component logs are suppressed at the ESP-IDF level immediately after `Serial.begin()`, before WiFi is initialized:

```cpp
esp_log_level_set("wifi",         ESP_LOG_NONE);
esp_log_level_set("phy_init",     ESP_LOG_NONE);
esp_log_level_set("esp32-hal-adc", ESP_LOG_NONE);
```

This cuts the logging path entirely, preventing the ISR from ever reaching `lock_init_generic`.

### ADC2 / WiFi Hardware Conflict

**Problem:** The ESP32 hardware shares ADC2 with the WiFi radio. Any `analogRead()` call on an ADC2 pin (GPIO 0–9, 12–15) while WiFi is active fails with `ESP_ERR_TIMEOUT` and logs an error every call. Two locations in the codebase called `analogRead(0)` to seed the random number generator.

**Solution:** All `analogRead(0)` calls were replaced with `esp_random()`, which reads from the ESP32's hardware random number generator — a true entropy source that is available regardless of WiFi state:

| File             | Before                      | After                      |
|------------------|-----------------------------|----------------------------|
| `EyesWidget.cpp` | `randomSeed(analogRead(0))` | `randomSeed(esp_random())` |
| `NTPClient.cpp`  | `randomSeed(analogRead(0))` | `randomSeed(esp_random())` |

### TrueType Font Loading Resilience

**Problem:** During rapid widget switching, the FreeType font cache manager (`_ftc_manager`) is unloaded and reloaded on every font change. Under transient heap pressure (e.g., immediately after an HTTP response), `loadFont()` can fail. The previous code then called `drawString()` with a NULL `_ftc_manager`, causing FreeType to dereference a null function pointer and crash with:

```
Guru Meditation Error: Core 1 panic'ed (InstrFetchProhibited)
PC: 0x00000000
```

**Root cause detail:** `setFont()` called `loadFont()` multiple times in a retry loop *without* calling `unloadFont()` between attempts. Each failed attempt left `_ftc_manager` in a partially-initialized state. The next retry called `FTC_Manager_New()` on top of that corrupted state, leaking the previous manager and fragmenting the heap further — making subsequent attempts *less* likely to succeed.

**Solution:** A `tryLoadFont()` helper was introduced that **always calls `unloadFont()` first**, guaranteeing a clean FreeType state before every attempt. A `m_pendingFont` field tracks the *desired* font independently of whether it is currently loaded.

```
setFont(FONT_X)
  └─ m_pendingFont = FONT_X        ← intent recorded unconditionally
  └─ tryLoadFont(FONT_X)
       └─ m_render.unloadFont()    ← always clean slate
       └─ m_render.loadFont(...)   ← single attempt
  └─ if failed: m_curFont = NONE   ← safe, guarded

drawString(...)
  └─ if m_curFont == NONE:
       └─ tryLoadFont(m_pendingFont)   ← retry each frame
       └─ if still fails: skip frame   ← no crash, heals next draw
  └─ else: render normally
```

This means a transient font-load failure results in **at most one blank frame** — the widget self-heals on the very next draw cycle without any manual intervention, instead of showing permanently black screens or crashing.

### Font Rendering & FreeType Memory Optimizations

**Problem:** TrueType/OpenType font rendering using the FreeType library is highly resource-intensive on the ESP32. Several factors contributed to heap fragmentation and memory exhaustion during widget rendering:
1. **Multiple Font Renderers:** The system initialized a six-element array of `OpenFontRender` instances, each maintaining its own static lambda closures and caching system. This consumed excessive static memory and duplicated FreeType's shared glyph manager (`FTC_Manager`) cache.
2. **High-Frequency Allocation Churn:** During string drawing, `OpenFontRender` used `std::queue<FT_UInt32>` (backed by `std::deque`) to buffer Unicode codepoints. This allocated many small independent heap nodes per character on every frame, generating thousands of dynamic allocations per second.
3. **Redundant Bounding Box Queries:** Bounding box calculations called `drawHString` in "skip mode" to compute vertical alignment corrections (`yMin`), doubling font rendering calls and dynamic allocations.
4. **Frequent Font Cache Eviction:** The Clock Widget repeatedly switched fonts between digital display styles (DSEG7/DSEG14) and built-in fonts during a single draw loop, causing FreeType to continually evict and reload glyphs.

**Solution:** A comprehensive font-rendering optimization was implemented:
- **Single Renderer Consolidation:** Replaced the `OpenFontRender` array with a single consolidated `m_render` instance in `ScreenManager`. This eliminated duplicate glyph managers and saved ~2 KB of permanent heap.
- **Static Vectors in `drawHString`:** Converted local queues inside `OpenFontRender::drawHString()` to `static std::vector` variables that utilize `.clear()` and `.reserve()`. Since rendering is sequential on Core 1, this thread-safe change reduces heap allocations per character to zero.
- **FNV-1a Alignment Cache:** Implemented a 16-entry circular cache using FNV-1a hashing of the target string in `ScreenManager`. It caches vertical correction factors (`yMin`), bypassing redundant bounding-box calculations for identical strings.
- **Clock Font Stabilization:** Removed mid-draw font switching in the Clock Widget. It now draws AM/PM indicators using the built-in `TFT_eSPI` legacy fonts, preventing FreeType cache evictions.

### Static Heap & Task Management Optimizations

**Problem:** FreeRTOS tasks and standard library containers dynamically allocated on the heap led to instability. Standard heap allocations (`xTaskCreate` and `std::set`) can fail under memory pressure and contribute to long-term fragmentation.

**Solution:** Migrated dynamic allocations to compile-time static memory:
- **Static HTTP Task Stack:** Declared a global `StaticTask_t` buffer and a static `StackType_t` stack buffer (`12000` words/48 KB) in `TaskManager`. Switched `TaskManager::startWorkerTask` to use `xTaskCreateStaticPinnedToCore()`. This permanently reserves the task stack in `.bss` during compilation, recovering 12 KB (words) of dynamic heap and ensuring the networking task can always launch.
- **Fixed-Size Deduplication Array:** Replaced the dynamically allocated `std::set<String>` for tracking active request URLs with a static `String activeUrls[10]` array. Safe thread access is maintained using a simple linear scan, avoiding red-black tree node overhead and reclaiming ~3 KB of heap space per cycle.

### ESP_SSLClient Integration for Low-Heap Secure Networking

**Problem:** Standard ESP32 SSL clients (such as `WiFiClientSecure` using mbedTLS) require large buffers (often 16 KB RX / 16 KB TX) for TLS handshakes, which can consume over 30-40 KB of contiguous heap space. Under transient heap pressure, HTTPS requests to weather and stock APIs would fail (HTTP error `-1`) or cause system crashes.

**Solution:** Switched to a custom low-heap wrapper based on `ESP_SSLClient` (BearSSL):
- **ESP_SSLClientWrapper:** Implemented a custom wrapper class (`ESP_SSLClientWrapper`) inheriting from `WiFiClient` that embeds `ESP_SSLClient`. It configures tiny BearSSL internal buffer sizes (5120 bytes RX and 1024 bytes TX), reducing the total heap footprint for an active TLS socket to less than 15 KB.
- **Write Error Synchronization:** Added a bidirectional error-propagation helper (`syncWriteError()`) between the wrapper and `ESP_SSLClient`'s internal stream. This ensures write errors and socket close statuses are accurately propagated to `HTTPClient` instead of failing silently.
- **Timeout Forwarding:** Fixed connection timeouts by forwarding the timeout parameter to `ESP_SSLClient::connect()`.
- **Heap Safety Guard:** Embedded a check before allocating the SSL client. If the largest contiguous free memory block is below a threshold (`SSL_MIN_HEAP = 15000` bytes), the request is gracefully skipped and queued with a timeout error, preventing mbedTLS/BearSSL from crashing due to allocation failures.

### Environment-Switching Timezone Offset Memory Leak

**Problem:** Over a 15-minute runtime, the free heap and largest contiguous memory block steadily declined. This was caused by a memory leak of ~44 bytes per second.
- The root cause was `GlobalTime::calculateActiveTimezoneOffset()`, which was invoked once per second in the main loop.
- To compute the local time zone offset, it changed the global environment variable `TZ` by calling `setenv("TZ", "UTC0", 1)` and then restored it via `setenv("TZ", savedTz, 1)`.
- Under the `newlib` C library used by ESP-IDF, calling `setenv` with changing string lengths leaks memory because it dynamically reallocates new entries in the environment array but does not reclaim old ones.

**Solution:** Replaced environment-variable switching with pure calendar arithmetic:
- Implemented a static timezone-independent helper `tmToSeconds()` that converts a standard `struct tm` structure directly into raw epoch seconds using Gregorian calendar arithmetic:
  $$ \text{seconds} = f(\text{year}, \text{month}, \text{day}, \text{hour}, \text{minute}, \text{second}) $$
- The updated function retrieves local time using `localtime_r()` (which relies on the system's already-configured POSIX timezone) and subtracts the raw UTC epoch to compute the offset arithmetically:
  ```cpp
  struct tm tmLocal;
  localtime_r(&utcEpoch, &tmLocal);
  int offset = (int)(tmToSeconds(&tmLocal) - utcEpoch);
  ```
- **Impact:** The 44-byte-per-second memory leak is fully resolved. The ESP32's free heap and largest free block remain completely flat during infinite runtime cycles.

### Rationalized Log Levels & Compile-Time Diagnostics

**Problem:** The firmware previously emitted verbose debug messages from widgets directly to the serial port without any level-based control. This created excessive serial traffic and high overhead. Furthermore, monitoring heap usage and tracking memory leaks across different widgets during API calls and deserialization required ad-hoc print statements.

**Solution:** A unified widget logging and heap diagnostic framework was introduced:
- **Unified Log Macros:** Replaced all widget `Log.*` and `Serial.*` outputs with `WIDGET_LOG_ERROR`, `WIDGET_LOG_WARN`, `WIDGET_LOG_INFO`, `WIDGET_LOG_TRACE`, and `WIDGET_LOG_TRACE_RAW` macros. All macros prepend the respective widget tag (e.g., `[WiFi]`, `[Clock]`, `[Weather]`, `[Stock]`, `[Parqet]`, `[MQTT]`, `[5Zone]`) for consistent serial formatting.
- **Compile-Time Level Control:** Added the `WIDGET_DEBUG_LEVEL` directive in `config.h`. Standard levels include `WIDGET_LOG_LEVEL_SILENT` (default), `WIDGET_LOG_LEVEL_ERROR`, `WIDGET_LOG_LEVEL_WARN`, `WIDGET_LOG_LEVEL_INFO`, and `WIDGET_LOG_LEVEL_TRACE`. In silent mode, all widget logs and diagnostics compile down to no-ops, ensuring zero runtime or binary size overhead.
- **Heap Diagnostics:** Introduced the `WIDGET_HEAP_SNAP(prefix, label)` macro for widgets, and integrated the system-level `HEAP_SNAP(label)` macro. Both trace ESP32 heap metadata (total free heap, largest free block size, and fragmentation percentage) at key lifecycle points (initialization, network fetches, and JSON deserialization).
- **Runtime Web UI Toggle Integration:** All widget log macros and both widget and system heap diagnostics (`HEAP_SNAP`/`SHOW_MEMORY_USAGE`) check the global web portal toggle `isDebugEnabled_global()` before emitting outputs, ensuring all logs are strictly controlled by the runtime debug flag.

---

## 🤝 Community & Support

- **Discord**: Join us for setup help and contribution discussions [here](https://link.brett.tech/discord).
- **YouTube**: Watch the assembly and flashing guide [here](https://link.brett.tech/orbsYT).

*Created with ❤️ by the Info Orbs Community.*
