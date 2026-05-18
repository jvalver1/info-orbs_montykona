# **Software Architecture and Technical Specification for the Info-Orbs ESP32-S3 Display Ecosystem**

## **Executive Architectural Summary**

The Info-Orbs platform represents an advanced, open-source embedded display widget system natively designed for the Espressif ESP32 microcontroller architecture.1 Functioning as a high-fidelity networked desktop visualization tool, the system is engineered to provide real-time informational widgets, including synchronized weather conditions, stock market tickers, and intricate Nixie tube-style clock animations.2 The core hardware topology relies on driving multiple round Thin-Film-Transistor (TFT) liquid-crystal displays over a single shared Serial Peripheral Interface (SPI) bus, utilizing discrete Chip Select (CS) lines for physical multiplexing.4 The system is physically housed in custom enclosures, with prominent forks and 3D-printable iterations developed by the community, such as those by the developer Montykona (associated with the GitHub user jvalver1).5

To achieve a continuously responsive User Experience (UX) while simultaneously executing high-latency, blocking network operations, the software architecture must exploit the asymmetric multiprocessor capabilities of the ESP32-S3 SoC. The deployment of a Real-Time Operating System (FreeRTOS) is strictly mandated to orchestrate this concurrency.1 The architectural paradigm dictates that network communication, data parsing, and user interface rendering must be highly isolated across the dual-core boundary to prevent CPU starvation and thread blocking.

This document serves as an exhaustive technical specification to guide the re-engineering and optimization of the firmware. It addresses the conservation of the legacy hardware pinout, the mitigation of critical race conditions inherent in implicit parallelism, the integration of an Access Point (AP) captive portal for initial provisioning, and the replacement of legacy graphics libraries with highly optimized Direct Memory Access (DMA) driven alternatives. The overarching objective is to produce a deeply efficient, mathematically stable, and highly responsive codebase.

## **Hardware Topology and Electrical Constraints**

The foundational hardware layer dictates the immovable boundaries within which the firmware must operate. The Info-Orbs developer kit and the associated Montykona printed circuit board (PCB) iterations incorporate an ESP32 38-pin module, up to six active round TFT displays (with five actively driven and one spare), and three tactile pushbuttons for user navigation.1 Power is delivered via a 5V USB-C breakout board, regulating down to the 3.3V logic level required by the ESP32 and the display modules.3

### **Microcontroller Target: ESP32-S3 Architecture**

The ESP32-S3 features a dual-core Xtensa 32-bit LX7 microprocessor capable of running at frequencies up to 240 MHz. Crucially for graphics-heavy embedded applications, the LX7 core includes vector instructions that drastically accelerate floating-point mathematical operations and matrix transformations. The architecture provides a Protocol CPU (PRO\_CPU, natively mapped to Core 0\) typically reserved for the Wi-Fi baseband and network operations, and an Application CPU (APP\_CPU, natively mapped to Core 1\) for user application logic and display rendering. Furthermore, the memory map consists of internal SRAM and optional external Pseudo-Static RAM (PSRAM) accessed via the Octal SPI (OPI) bus. The firmware must intelligently route memory allocations based on the latency requirements of the payload, keeping high-speed display buffers in internal SRAM while offloading large JSON network payloads to PSRAM.

### **Mandatory Pinout Configuration and Signal Integrity**

For any subsequent firmware iterations to maintain strict backward compatibility with the existing hardware ecosystem and Montykona PCB routing, the physical General-Purpose Input/Output (GPIO) pin assignment must remain rigidly conserved.1

| Subsystem | Function Designation | ESP32 GPIO | Electrical Characteristics and Architectural Notes |
| :---- | :---- | :---- | :---- |
| **Power Input** | VCC | 5V / VCC | Driven by USB-C breakout. Requires decoupling capacitors on the PCB.1 |
| **Power Ground** | GND | GND | Common ground reference plane.1 |
| **SPI Master Bus** | SDA (MOSI) | G17 | Master Out Slave In. Pushes high-speed pixel data to all displays.1 |
| **SPI Master Bus** | SCLK | G23 | SPI Clock signal. Must support high-frequency transmission (target: 40 MHz).1 |
| **Display Control** | DC (Data/Command) | G19 | Selector line determining if the SPI payload is an instruction or pixel data.1 |
| **Display Control** | RST (Hardware Reset) | G18 | Global hardware reset line tied to all TFT modules simultaneously.1 |
| **Multiplex Line 1** | CS1 (Screen 1\) | G13 | Chip Select for Display 1\. Active Low. Requires strict pull-up behavior.1 |
| **Multiplex Line 2** | CS2 (Screen 2\) | G33 | Chip Select for Display 2\. Active Low. Requires strict pull-up behavior.1 |
| **Multiplex Line 3** | CS3 (Screen 3\) | G32 | Chip Select for Display 3\. Active Low. Requires strict pull-up behavior.1 |
| **Multiplex Line 4** | CS4 (Screen 4\) | G25 | Chip Select for Display 4\. Active Low. Requires strict pull-up behavior.1 |
| **Multiplex Line 5** | CS5 (Screen 5\) | G21 | Chip Select for Display 5\. Active Low. Requires strict pull-up behavior.1 |
| **User Input 1** | Button 1 (Left/Back) | G14 | Pulled to VCC/5V when pressed. Requires aggressive debouncing.1 |
| **User Input 2** | Button 2 (Select/Menu) | G26 | Pulled to VCC/5V when pressed. Requires aggressive debouncing.1 |
| **User Input 3** | Button 3 (Right/Fwd) | G27 | Pulled to VCC/5V when pressed. Requires aggressive debouncing.1 |

### **Serial Peripheral Interface (SPI) Contention Analysis**

The hardware architecture places all five active display modules on the exact same MOSI (SDA) and SCLK pins.1 Standard TFT displays (such as the ubiquitous GC9A01 ICs used for round screens) do not natively support addressing in the manner of the I2C protocol. Instead, multiplexing is physically achieved via the Chip Select (CS) pins.4

When the microcontroller needs to write a localized frame to Screen 1, the firmware must pull GPIO 13 (CS1) LOW, transmit the frame data via SPI, and subsequently pull GPIO 13 HIGH to terminate the transaction. All other CS lines must be held strictly HIGH during this operation to prevent data corruption and parasitic rendering. Due to the combined capacitive load of five separate display controllers attached to a single clock and data line, signal integrity is a primary architectural concern. The firmware must configure the ESP32 SPI host driver with appropriate drive strength and limit the SPI clock frequency to a stable threshold. Pushing the clock beyond 40 MHz on a heavily loaded, unbuffered bus typically results in bit-flipping at the physical layer, manifesting as visual static or complete display desynchronization.

Furthermore, the simultaneous initialization of five TFT displays can cause sudden current inrush spikes that may trigger the ESP32's internal Brownout Detector (BOD). The initialization routine must strictly stagger the startup sequence, introducing a calculated delay of 50 to 100 milliseconds between pulling each respective CS pin low and transmitting the initial wake-up commands.

## **Dual-Core Optimization Strategy and Task Scheduling**

To guarantee a highly responsive user interface while executing high-latency internet transactions, the firmware must enforce strict execution boundaries utilizing the dual-core architecture. FreeRTOS provides the foundational pre-emptive scheduler required to pin specific tasks to specific CPU cores, optimizing the Asymmetric Multiprocessing (AMP) paradigm.

### **Core 0 (PRO\_CPU): Network and Communications Domain**

Core 0 is dedicated exclusively to handling the inherent unpredictability of the network layer. The Wi-Fi subsystem, the Lightweight IP (LwIP) stack, and the mbedTLS cryptographic libraries execute natively on this core. To prevent network latency, DNS resolution timeouts, or API rate limits from stuttering the graphical interface, all external polling and connection management must be isolated here.

Tasks specifically pinned to Core 0 include the Wi-Fi Connection Manager, which governs the transition between Station (STA) mode and Access Point (AP) mode. Alongside it resides the Asynchronous Web Server, functioning as a captive portal for initial device configuration. The heaviest computational burden on this core is the Data Fetcher Task, which executes HTTPS GET requests to remote REST endpoints (e.g., fetching weather JSON payloads or stock market ticker data). Because establishing a TLS 1.2/1.3 connection requires significant contiguous heap memory and heavy CPU cycles for key exchange, isolating this on Core 0 ensures that Core 1 remains completely undisturbed during the handshake. Finally, a dedicated JSON Parsing Task deserializes incoming network payloads into tightly packed, statically allocated C-structures, immediately freeing the volatile heap space.

### **Core 1 (APP\_CPU): User Experience and Graphics Domain**

Core 1 operates in a completely isolated domain for the User Interface. Its primary directive is maintaining high frame rates for animations (such as the Nixie clock) and ensuring zero-latency responses to tactile button presses.

Tasks pinned to Core 1 include the Display Director Task, which acts as the ultimate orchestrator for visual output. It calculates pixel animations, manages the lifecycle of individual widgets, and triggers the hardware Direct Memory Access (DMA) transfers to the SPI peripheral. An Input Interrupt Service Routine (ISR) Handler captures button state changes via hardware interrupts, immediately offloading the time-consuming debounce logic to a deferred FreeRTOS software timer. Additionally, a UX State Machine task evaluates user inputs to manage menu navigation, global screen dimming, and contextual transitions between the available data widgets.

| Task Designation | Execution Core | FreeRTOS Priority | Estimated Stack Size (Bytes) | Operational Description |
| :---- | :---- | :---- | :---- | :---- |
| **System Idle** | Core 0 & 1 | 0 (Lowest) | Configurable | Feeds the hardware Watchdog Timers (WDT) and allows lower-power states. |
| **Wi-Fi Event Loop** | Core 0 | 18 (High) | 4096 | Native ESP-IDF task managing PHY layer events, DHCP, and link states. |
| **Async Web Server** | Core 0 | 5 (Medium) | 8192 | Serves HTML/CSS to clients during the AP configuration phase. |
| **API Data Fetcher** | Core 0 | 3 (Low) | 8192 | Executes blocking HTTPS requests to REST APIs (Weather, Stocks). |
| **Display Director** | Core 1 | 10 (High) | 8192 | Calculates framebuffers and orchestrates DMA push sequences to displays. |
| **UX State Machine** | Core 1 | 5 (Medium) | 4096 | Processes user inputs and modifies global visualization states. |
| **Input Handler (Deferred)** | Core 1 | 15 (Very High) | 2048 | Unblocked by ISR. Applies software debounce and queues UI events. |

## **Concurrency Management and Race Condition Mitigation**

The user query specifically highlighted a requirement to explore mechanisms that increase efficiency while lowering the chance of errors, particularly race conditions due to implicit parallelism. In a dual-core environment, implicit parallelism introduces severe vulnerabilities in the form of data tearing and memory access violations.

A race condition occurs fundamentally when two independent execution threads attempt to access a shared resource concurrently, and at least one of those accesses is a write operation. For instance, if the API Data Fetcher on Core 0 attempts to update a multidimensional array representing stock prices at the exact millisecond that the Display Director on Core 1 is iterating through that same array to draw text on the screen, the resulting uncoordinated access will result in corrupted display output, pointer exception faults, or a total system crash commonly triggering a LoadProhibited Exception in the Xtensa core.

### **Architectural Directives for Absolute Thread Safety**

To mathematically eliminate concurrency errors, the firmware architecture must strictly decouple data acquisition from data presentation using specific FreeRTOS Inter-Process Communication (IPC) primitives. Direct access to global variables across core boundaries is strictly prohibited.

#### **Mutex-Protected Shared State Synchronization**

All global variables representing widget data must be encapsulated within distinct, typed struct definitions. Access to these structures must be heavily guarded by FreeRTOS Priority-Inheritance Mutexes (xSemaphoreCreateMutex). A standard binary semaphore is insufficient due to the risk of priority inversion, where a low-priority task holds the lock while a high-priority task is blocked, causing the system to stall.

The write sequence executing on Core 0 strictly dictates that when the Data Fetcher task receives a new parsed JSON payload, it must first attempt to acquire the designated Mutex using xSemaphoreTake(DataMutex, portMAX\_DELAY). Once the lock is acquired, it safely overwrites the structured data. Immediately after the assignment, it releases the Mutex using xSemaphoreGive(DataMutex).

Conversely, the read sequence executing on Core 1 dictates that before the Display Director begins calculating the visual layout of a widget, it must acquire the exact same Mutex. It then executes a highly efficient memory copy (memcpy) to duplicate the relevant data into a localized, private buffer allocated on Core 1's stack. The Mutex is then instantly released. The subsequent display drawing routine—which is computationally expensive and slow—operates strictly on the localized copy. This architectural pattern ensures that the lock is held for mere microseconds, allowing Core 0 to continuously update the master structure in the background without waiting for Core 1 to finish rendering.

#### **Event Groups for Granular State Signaling**

FreeRTOS Event Groups (xEventGroupCreate) are the recommended primitive for system-wide state synchronization. While queues are excellent for passing data, event groups are ideal for broadcasting boolean states to multiple tasks simultaneously. A global Event Group should be instantiated to track the macro-state of the system. Specific bits within the group map to specific conditions: BIT\_0 represents Wi-Fi Connected, BIT\_1 represents AP Mode Active, BIT\_2 signifies New Weather Data Available, and BIT\_3 indicates New Stock Data Available.

The Display Director on Core 1 can utilize the xEventGroupWaitBits API to enter a highly efficient blocked state. It wakes up to redraw the weather widget only when Core 0 explicitly sets BIT\_2, thereby conserving immense CPU cycles and drastically reducing thermal output compared to a continuous polling loop.

#### **Instruction Queuing and Deferred Interrupt Handling**

The tactile pushbuttons connected to G14, G26, and G27 1 must never be read using simple polling loops (e.g., continuously calling digitalRead() within the main task loop), as this consumes unnecessary CPU cycles on the Application Core. Instead, hardware interrupts must be attached to these GPIO pins.

Upon a physical button press, the Interrupt Service Routine (ISR) executes. Because ISRs preempt all other operations and run in a specialized context, no blocking code or FreeRTOS API calls (other than those ending in FromISR) are permitted. The ISR must execute a minimal directive: vTaskNotifyGiveFromISR(). This single instruction unblocks a high-priority, deferred Button Handler task. This dedicated task applies a software debounce algorithm mathematically evaluating the time delta between interrupts to ignore mechanical switch bounce. Once validated, the task generates a typed command payload and pushes it into a FreeRTOS Command Queue using xQueueSend. The UX State Machine reads from this queue, ensuring inputs are processed sequentially and asynchronously.

## **Graphics Subsystem and Deep Library Optimization**

The visual fidelity of the Info-Orbs platform—particularly the requirement to render intricate Nixie tube visualizations and fluidly scrolling data 2—demands highly optimized graphics handling. The legacy implementation detailed in the original repository relies primarily on the TFT\_eSPI library.4 While TFT\_eSPI is highly regarded within the broader hobbyist ecosystem, its internal architecture presents severe bottlenecks when scaled to complex, multi-display topologies.

### **Limitations of TFT\_eSPI in Multi-Display Systems**

The TFT\_eSPI library is fundamentally constructed with the assumption of a single primary display attached to the SPI bus. To drive multiple displays 3, developers are typically forced into inefficient paradigms. The most common workaround involves manipulating the CS pins manually using standard GPIO writes before invoking the library's drawing functions.4

This manual manipulation critically fractures the library's ability to utilize asynchronous DMA transfers. Direct Memory Access is a hardware feature of the ESP32 that allows the SPI peripheral to read data directly from RAM and push it to the display without any CPU intervention. However, if a firmware developer must manually hold a CS pin low using digitalWrite(CS\_PIN, LOW), they are forced to wait until the SPI transfer is entirely complete before pulling the pin high again. If a full-screen frame takes 30 milliseconds to transmit, the CPU is completely blocked in a busy-wait loop for that entire duration. Across five screens, this equates to 150 milliseconds of dead compute time per frame, limiting the maximum system framerate to a sluggish 6 FPS and defeating the purpose of utilizing a 240 MHz dual-core processor.

### **Strategic Migration Directive: Integration of LovyanGFX or esp\_lcd**

To satisfy the requirement to increase efficiency in size and compute resources while utilizing more efficient libraries, it is strictly mandated to migrate the graphical abstraction layer to LovyanGFX (or the native ESP-IDF esp\_lcd component if migrating entirely away from the Arduino framework). LovyanGFX is structurally optimized for the ESP32 architecture and natively supports multiple concurrent display panel objects.

The architectural advantages of LovyanGFX are profound. It allows the instantiation of multiple independent display objects within the firmware. Each object is configured with its specific corresponding CS pin (G13, G33, G32, G25, G21) 1 while sharing the exact same SPI bus configuration (G17, G23). The library driver natively manages the bus locking mechanism and automatic CS toggling at the lowest silicon level.

When a visual widget is ready to be drawn, LovyanGFX constructs a linked list of DMA descriptors. It then instructs the DMA engine to push the framebuffer to the SPI peripheral. The critical distinction is that this process is non-blocking. The library automatically handles pulling the CS pin high via a hardware interrupt only when the DMA transfer completes. The CPU is instantly freed to begin calculating the visual layout for the next screen or processing a user input command, increasing apparent responsiveness exponentially.

| Graphics Feature | Legacy TFT\_eSPI Implementation | Proposed LovyanGFX Architecture | Performance Impact |
| :---- | :---- | :---- | :---- |
| **Multi-Display CS Handling** | Manual GPIO toggling at application level.4 | Native hardware-level management per panel object. | Eliminates CPU busy-waiting; allows asynchronous bus arbitration. |
| **DMA Utilization** | Frequently broken by manual CS management. | Deeply integrated via DMA linked lists and automated callbacks. | Frees Core 1 to calculate upcoming frames concurrently with transmission. |
| **Sprite Memory Allocation** | Basic PSRAM support, prone to fragmentation. | Advanced packed memory mapping, native RGB565 handling. | Reduces heap fragmentation; prevents LoadProhibited hard faults. |
| **Alpha Blending** | Computationally expensive loop-based pixel math. | Optimized vector instructions utilizing the Xtensa LX7 core. | Enables highly fluid crossfade animations for Nixie clock widgets.2 |

### **Advanced Framebuffer Strategy and Sprite Drawing**

To eliminate visual tearing—a phenomenon where the user sees a frame partially drawn—rendering must never occur directly to the physical display panel. An off-screen memory buffer, commonly referred to as a "Sprite," must be utilized.

The firmware creates a Sprite buffer in the ESP32's internal SRAM matching the exact dimensions of the target widget. All text rendering, geometric shape drawing, and image decoding (such as the high-resolution Nixie digits) are executed entirely within this invisible memory space. Once the composition of the entire widget is complete, the entire contiguous block of memory is pushed to the target TFT display via a single DMA SPI transaction. By allocating these sprites in 16-bit RGB565 color format, the data aligns perfectly with the native expectation of the ILI9341/GC9A01 display controllers, bypassing any need for runtime color space conversion and drastically reducing transmission bandwidth.

## **First-Time Provisioning and the Network Configuration Workflow**

The utility of any headless, connected device relies heavily on a seamless and foolproof initial setup experience. The firmware must include a robust, state-driven initialization sequence that safely defaults to a local configuration mode if no previously known Wi-Fi networks are available in the environment.

### **Access Point (AP) Fallback and Captive Portal Initialization**

Upon power-up, the ESP32-S3 must initialize its Non-Volatile Storage (NVS) partition to query for saved Wi-Fi credentials and API keys. The startup sequence follows a strict state machine:

First, the system reads the NVS. If valid credentials exist, the device attempts to connect to the router in Station (STA) mode. If the connection fails after a defined timeout period (e.g., the router is offline, or the user has changed their network password), the system aborts the STA attempt to prevent an infinite loop.

Upon failure, or if no credentials exist (the factory default state), the firmware transitions the Wi-Fi radio into Access Point (AP) mode. It broadcasts a localized SSID, typically designated as "Info-Orbs-Setup". Simultaneously, a local DNS server instance is instantiated on Core 0\. This DNS server is configured to execute a captive portal hijack. Any Domain Name System request made by a connected smartphone or computer (e.g., navigating to google.com) is intercepted by the ESP32 and resolved directly to the ESP32's localized AP IP address (conventionally 192.168.4.1).

### **Asynchronous Web Server and Configuration Payload**

An HTTP Web Server must execute asynchronously on Core 0\. When the user's browser is forcibly redirected to the localized IP address, the server responds with a lightweight, responsive HTML/CSS configuration page. To conserve flash storage space, this web interface should be compressed using gzip and stored within the ESP32's SPIFFS or LittleFS filesystem partition.

The interface must expose input fields for the target Wi-Fi SSID, the associated password, and required external API keys (such as OpenWeatherMap tokens or AlphaVantage keys for stock data). Furthermore, localization preferences including Timezone offsets and Metric/Imperial unit selections must be configurable here.

Upon form submission, the web server processes the incoming HTTP POST request. The JSON payload is extracted, sanitized to prevent buffer overflows, and written strictly to the NVS partition. Following a successful atomic write operation, the server returns an HTTP 200 OK response to the browser and initiates a software reboot sequence via the esp\_restart() API. Upon the subsequent boot, the device will seamlessly enter STA mode utilizing the newly provisioned credentials.

### **Network Polling Efficiency and HTTPS Overhead**

In standard operational mode, Core 0 manages all outbound data requests to external APIs. To fulfill the strict requirement for efficiency in compute resources, the networking architecture must be heavily optimized.

External APIs require HTTPS, which relies on Transport Layer Security (TLS). The ESP32 utilizes the mbedTLS library for this encryption. Establishing a TLS 1.2 or 1.3 connection is incredibly memory-intensive, often requiring upwards of 35-40 Kilobytes of contiguous heap space for the certificate chains and cryptographic buffers. If the firmware attempts to open simultaneous connections for Weather and Stocks, it will likely exhaust the available heap memory, resulting in an immediate system crash.

Therefore, the Data Fetcher task must serialize its network requests. It must never open two HTTPS sockets concurrently. Furthermore, where supported by the remote API endpoints, the HTTP client should utilize Keep-Alive headers. This instructs the remote server to hold the TCP socket open after the response is sent, completely eliminating the massive cryptographic overhead of the TLS handshake on subsequent polling intervals.

If the Wi-Fi connection drops, or if an API returns an HTTP 429 Too Many Requests status code, the firmware must implement a mathematical exponential backoff algorithm. Instead of aggressively polling the endpoint every few seconds and consuming valuable power, the delay between retries should double sequentially (e.g., 5 seconds, 10 seconds, 20 seconds, up to a maximum cap).

### **Memory-Safe JSON Parsing Architecture**

Network responses for weather conditions and financial data are universally formatted as JSON payloads. The deserialization of JSON in embedded C++ is historically a primary vector for memory fragmentation. The firmware architecture must strictly utilize the ArduinoJson library, but it must forgo dynamic allocation wherever possible.

Instead of parsing an unknown payload dynamically, the firmware must pre-calculate the exact required size of the JSON document using the StaticJsonDocument class or a tightly scoped DynamicJsonDocument allocated globally in a pre-reserved block of memory. Parsing must occur immediately after the payload is received by Core 0\. The raw, incoming string payload must be analyzed, the required specific variables (current temperature, specific stock price, time epoch) extracted and pushed into the Mutex-protected C-structs, and the massive raw string buffer instantly discarded. This highly disciplined approach to memory lifecycle management severely curtails heap fragmentation, ensuring that the device can run continuously for months without requiring a reboot.

## **Data Structure and Inter-Task Communication Models**

The architectural robustness and longevity of the Info-Orbs platform depend fundamentally on how the parsed data is modeled in memory and transmitted between the execution cores.

### **Defining Strict Data Transfer Objects (DTOs)**

Each visualization widget must be backed by a strictly typed Data Transfer Object (DTO). Utilizing plain C-structures provides the lowest possible memory overhead, avoiding the virtual table bloat associated with complex C++ objects.

C

typedef struct {  
    float current\_temperature;  
    float apparent\_temperature;  
    uint8\_t relative\_humidity\_percentage;  
    uint16\_t weather\_condition\_code;  
    char location\_name;  
    bool is\_data\_valid;  
    uint32\_t last\_update\_timestamp;  
} WeatherDTO\_t;

typedef struct {  
    char ticker\_symbol;  
    float current\_market\_price;  
    float absolute\_daily\_change;  
    float percentage\_daily\_change;  
    bool market\_is\_open;  
    bool is\_data\_valid;  
} StockDTO\_t;

These structures are instantiated globally. Core 0 maintains exclusive write access via Mutexes, ensuring that half-written data (such as updating a price but not the associated percentage change) is never exposed to the UI rendering engine on Core 1\.

### **Instruction Passing via FreeRTOS Queues**

While Mutexes are excellent for protecting passive state, FreeRTOS Queues are the optimal structural choice for actively passing command instructions between the execution contexts.

Consider the workflow when a user interacts with the device. If the user presses Button 2 (G26) to change the displayed widget from the Weather screen to the Stocks screen on Display 3:

1. The hardware interrupt on Core 1 fires, unblocking the deferred Button Handler Task.  
2. The Button Handler Task generates a structured command message representing CMD\_CHANGE\_WIDGET\_TARGET\_SCREEN\_3\_STOCKS.  
3. This message is placed into an instruction Queue using the xQueueSend API.  
4. The Display Director Task, operating within its main loop on Core 1, reads this Queue via xQueueReceive. Upon processing the new command, it updates its internal state machine array, noting that Screen 3 must now render financial data.  
5. Simultaneously, if the UX State Machine determines that the Stock data residing in the StockDTO\_t is stale (based on the last\_update\_timestamp), it generates a secondary request command.  
6. This request command (CMD\_FORCE\_FETCH\_STOCK\_DATA) is placed into a separate Queue directed at Core 0\.  
7. The Data Fetcher Task on Core 0 receives this request, preempts its standard polling timer, executes the HTTPS call to the financial API, updates the Mutex-protected struct, and finally flags the global Event Group indicating that fresh data is ready for rendering.

This queue-driven, message-passing architecture ensures that neither execution core is ever blocked waiting for the other. It perfectly fulfills the requirement for implicit parallelism while erecting an impenetrable barrier against data races.

## **Widget Specific Rendering Workflows**

To construct a functional new iteration of the codebase, the logical flow and mathematical requirements of specific widgets must be rigorously defined. The following workflows detail the lifecycle of the system's primary visualization features.

### **The Nixie Clock Animation Engine**

The Nixie tube clock 2 requires continuous animation and ultra-low latency to successfully maintain the aesthetic illusion of glowing neon gas inside glass tubes.

The workflow begins with Time Synchronization on Core 0\. A Network Time Protocol (NTP) task initializes immediately upon establishing a Wi-Fi connection. It executes a lightweight UDP request to synchronize the internal ESP32 Real-Time Clock (RTC). To conserve bandwidth and prevent IP banning, the task then enters a deep sleep state, waking only once every 12 to 24 hours to correct any subtle quartz oscillator drift.

The Animation Rendering occurs exclusively on Core 1\. The Display Director Task maintains a small internal array representing the digits currently displayed across the physical screens. At a highly tuned refresh rate (e.g., 30 Frames Per Second), the task compares the current RTC time against the displayed time array.

When a digit change is required (for example, the minute rolls over from 09 to 10), the firmware must not simply erase the screen and draw the new number, as this destroys the Nixie illusion. Instead, it must execute a mathematical alpha-blending routine within the Sprite buffer. The RGB565 bitmap of the outgoing '9' digit is mathematically faded out by scaling down its pixel brightness values iteratively, while the bitmap of the incoming '0' digit is simultaneously scaled up and overlaid.

The blending math applied at the pixel level is defined as:

![][image1]

As ![][image2] increments from 0 to 255 over the course of the transition, the image smoothly crossfades. Once the blended frame is calculated in SRAM, it is pushed via SPI DMA. Because the DMA engine handles the physical transmission autonomously, Core 1 can immediately begin calculating the crossfade mathematics for the next screen in the array, ensuring fluid animation across all five displays concurrently.

### **The Environmental Weather Display**

Weather data is inherently static over short time intervals, requiring a vastly different resource allocation strategy compared to the high-framerate Nixie clock.

The Polling Strategy on Core 0 dictates that the Weather task executes a REST API call strictly once every 15 to 30 minutes, governed tightly by a FreeRTOS software timer. The JSON payload is parsed, and specific meteorological condition codes (e.g., 800 representing Clear Sky, 500 representing Light Rain) are evaluated. These numeric codes are mapped via a lookup table to specific high-resolution bitmap icons stored persistently in the firmware's flash memory.

Unlike the clock, the weather widget rendering is entirely static. The Display Director Task draws the weather interface into the Sprite buffer exactly once when the BIT\_2: New Weather Data Available event is flagged. Once the DMA push to the respective screen is complete, the rendering loop for that specific display enters a blocked state. This critical optimization ensures Core 1 expends absolutely zero CPU cycles redrawing the weather screen until the next 15-minute update occurs. This significantly lowers overall power consumption and prevents thermal buildup inside the physical chassis.

### **The Financial Ticker and Scrolling Assets**

The stock market widget requires slightly higher temporal resolution than weather data but faces identical network constraints regarding TLS overhead and API rate limits.

The Data Fetcher task polls the financial endpoint at user-configurable intervals (e.g., every 5 minutes during open market hours). Upon parsing the payload, the firmware calculates the daily delta to dynamically determine the color scheme of the text elements (rendering green for positive growth and red for negative decline).

If the layout requires scrolling text to accommodate long corporate names or multiple ticker symbols on a single small circular display, a localized hardware timer on Core 1 is utilized. This timer gently increments the X-axis offset of the text string within the Sprite buffer. To maintain efficiency, only the bounding box containing the moving text is updated and pushed via DMA at a steady 20 FPS, while the static elements (like the background and the static price) remain untouched in memory.

## **Physical Integration, Thermal Dissipation, and Case Mechanics**

The hardware schematic is complemented by custom 3D-printable chassis components, prominently featuring cases designed by Montykona.7 Firmware developers must acutely consider the physical and mechanical constraints imposed by these enclosures.

The ESP32-S3 microcontroller, the voltage regulation circuitry, and five active TFT displays generate measurable thermal output. The custom PCB is designed to mount securely to the rear of the printed case.7 If the firmware continuously drives the CPU at 240 MHz and pushes all five screens to maximum backlight brightness without utilizing sleep states or blocking delays, the confined ambient temperature within the PLA or PETG case will rise, potentially leading to thermal throttling of the silicon or deformation of the 3D printed plastics.

The software architecture must include a holistic thermal management strategy. This is achieved by implementing an idle state timer within the UX State Machine. After a defined period of zero user interaction (no button presses), the firmware should utilize the LEDC (LED Control) PWM peripheral to smoothly dim the backlights of the TFT displays to a fraction of their maximum current. Furthermore, during night hours (calculated via the synced RTC), the displays should be powered down entirely, and the ESP32 should enter a FreeRTOS Light Sleep state, halting the CPU clocks while maintaining RAM retention and Wi-Fi association.

Additionally, the physical assembly utilizes a TPU (Thermoplastic Polyurethane) plug for a USB-C power cord, intending for the cable to remain securely plugged into the internal ESP32 breakout.7 The firmware must account for potential voltage drop across lower-quality consumer USB cables by strictly avoiding instantaneous current spikes. The staggered initialization sequence mentioned in the hardware section is paramount here; pulling all screens out of reset simultaneously demands a surge of current that can easily exceed the transient limits of a basic USB power supply, causing an immediate brownout and reboot loop.

## **Code Efficiency, Asset Packing, and Compilation Strategy**

To guarantee extreme long-term stability, the overall firmware size and runtime compute resource allocation must be tightly controlled. While the ESP32-S3 possesses ample internal SRAM, careless allocation will rapidly degrade performance.

### **Static Allocation Imperative**

Dynamic memory allocation (using malloc or the new keyword in C++) should be strictly prohibited within the main operational loops. Over hours of continuous operation, dynamic allocation of varying string lengths (such as parsing different JSON sizes) leads to severe heap fragmentation. Eventually, the memory space resembles a checkerboard, and an attempt to allocate a large contiguous block (like a new TLS connection buffer) will fail, crashing the device.

All FreeRTOS task stacks, command queues, and DTO structures must be statically allocated in the global scope during the initial boot sequence.

C

// Architectural example of static allocation for FreeRTOS tasks to eliminate heap fragmentation  
StackType\_t xDisplayTaskStack;  
StaticTask\_t xDisplayTaskBuffer;

xTaskCreateStaticPinnedToCore(  
    DisplayDirectorTask,          // Function pointer  
    "DisplayDirector",            // Task name for debugging  
    8192,                         // Stack depth  
    NULL,                         // Task parameters  
    10,                           // Priority  
    xDisplayTaskStack,            // Pre-allocated stack memory array  
    \&xDisplayTaskBuffer,          // Pre-allocated task control block  
    1                             // Pinned Core (APP\_CPU)  
);

### **Flash Storage and PROGMEM Asset Packing**

The graphical assets required for the system—specifically the high-resolution Nixie tube bitmaps, the complex weather icons, and custom anti-aliased typography fonts—consume massive amounts of digital space. Storing these as raw bitmap files or decoding JPEGs at runtime is computationally prohibitive.

All assets must be pre-processed on a host machine during compilation and stored in the ESP32's flash memory (via the PROGMEM directive) as highly compressed byte arrays. Utilizing tooling provided by graphics libraries like LovyanGFX or LVGL allows developers to generate C-arrays directly formatted in the RGB565 color space. Because this 16-bit format natively matches the expectations of the SPI TFT displays, the firmware can transfer the image arrays from flash memory directly to the display buffer without requiring any CPU cycles for color space conversion or decompression math at runtime.

### **Configuration Template Decoupling**

As detailed in the legacy repository architecture 1, the system requires the duplication of a config.h.template file to define operational parameters prior to compilation. To enhance this paradigm for modern iterations, the firmware architecture must strictly decouple volatile user settings from immutable compile-time hardware settings.

The config.h file must be strictly reserved for defining hardware permutations—such as declaring GPIO pin definitions, selecting the specific TFT controller IC type, or tuning the SPI bus frequency. These parameters define the physical compilation target and will not change unless the PCB is redesigned. Conversely, volatile user preferences—such as Wi-Fi passwords, API keys, and timezone strings—must be handled exclusively at runtime via the Web Access Point and stored in the NVS partition. This architectural separation prevents end-users from being forced to install a complex toolchain like PlatformIO 1 and recompile the entire firmware simply to update their local Wi-Fi password if their router is replaced.

## **System Resilience and Automated Error Recovery**

A well-architected embedded system, particularly one interacting continuously with unpredictable external network environments, must anticipate failure states at all layers. The software specification must define behaviors for when data is unavailable, when network requests hang, and when physical hardware acts erratically.

### **Hardware Watchdog Timers (WDT) Integration**

Both Core 0 and Core 1 must be strictly monitored by the ESP32's internal Task Watchdog Timers (TWDT). In a complex dual-core system, if the SPI bus locks up due to electrical interference, or if a network socket fails to close and hangs the PRO\_CPU indefinitely without triggering a timeout, the system becomes unresponsive.

The Watchdog Timer acts as an ultimate fail-safe. It is essentially a hardware countdown clock. During normal operation, the FreeRTOS idle tasks (or explicit application tasks) periodically "feed" the watchdog, resetting the countdown. If a task becomes stuck in an infinite while loop or deadlocks while waiting for a Mutex that will never be released, the watchdog will fail to be fed. Upon the timer expiring (typically configured for 3 to 5 seconds), the internal silicon forces an unconditional hard reset of the entire microcontroller. This ensures the Info-Orbs platform remains highly autonomous, self-healing without requiring the user to physically disconnect and reconnect the USB power cable.

### **Graceful Degradation of the User Interface**

When an external failure occurs—for example, if the OpenWeatherMap API server goes offline, or an API key expires and returns an HTTP 401 Unauthorized error—the display interface must not freeze, crash, or present corrupted memory data.

The JSON parsing task on Core 0 must aggressively validate all expected fields. If a required field is missing or an error code is detected, the Data Fetcher must flag a standardized error state within the respective DTO struct (e.g., setting is\_data\_valid \= false). Upon receiving the Event Group signal, the Display Director on Core 1 evaluates this flag. Instead of attempting to draw missing data, it initiates a graceful degradation routine. It renders a subtle, aesthetically cohesive error icon (such as a small broken cloud or an exclamation mark) on that specific screen, while allowing all other functional widgets—such as the RTC-backed Nixie clock—to continue animating flawlessly. This isolated failure mode presents a highly polished, professional user experience that clearly communicates network issues without breaking the fundamental utility of the device.

## **Final Architectural Directives**

The Info-Orbs platform leverages a highly capable dual-core microcontroller, but extracting maximum graphical performance from the ESP32-S3 while maintaining absolute system stability requires a mathematically rigorous, defensively programmed software architecture.

Based on the exhaustive analysis of the system parameters, the physical constraints of the Montykona PCB and enclosure iterations 1, and the necessity to wholly prevent implicit parallelism race conditions, the following core architectural directives form the unalterable foundation of the new codebase specification:

First, Absolute Core Isolation must be maintained. Core 0 must be exclusively dedicated to the Wi-Fi baseband, LwIP stack, the Web Server configuration portal, and serialized TLS network polling. Core 1 must be computationally isolated for high-speed display execution and the asynchronous handling of tactile button interrupts.

Second, the system must enforce Decoupling via FreeRTOS IPC Primitives. Volatile variables and data structs must never be accessed directly across core boundaries. Priority-Inheritance Mutexes must protect all shared memory objects, and Event Groups alongside Command Queues must be the singular mechanisms for inter-core state signaling and instruction passing. This topology guarantees the mathematical elimination of data tearing.

Third, the graphics subsystem must undergo a Migration to Native Multi-Display Drivers. The legacy reliance on single-display abstraction layers (e.g., TFT\_eSPI) forces blocking CPU operations during CS pin toggling.4 The architecture must transition to LovyanGFX to facilitate autonomous, asynchronous DMA transfers across all five multiplexed CS pins, unblocking the Application Core and enabling fluid, high-framerate blending animations required for the Nixie clock.2

Finally, the firmware must implement Static Memory Pre-Allocation and robust thermal mitigation. To satisfy the mandate for extreme compute efficiency, all primary memory payloads—particularly Off-screen Sprite buffers and JSON document allocators—must be statically defined during system boot. Combined with staggered hardware initialization routines and FreeRTOS sleep state utilization, the firmware will operate securely within the thermal and electrical constraints of the physical chassis, resulting in an exceptionally stable, deeply efficient, and highly responsive embedded product.

#### **Obras citadas**

1. brettdottech/info-orbs: An open source desk widget using affordable TFT displays and an ESP32 \- GitHub, fecha de acceso: mayo 18, 2026, [https://github.com/brettdottech/info-orbs](https://github.com/brettdottech/info-orbs)  
2. a fully open source esp32 based,desk display widget kit I developed (: There's a great community around it and lots of widgets being developed. You can find the github by searching "info orbs Github". I'm very proud of what this has turned into\! \- Reddit, fecha de acceso: mayo 18, 2026, [https://www.reddit.com/r/esp32/comments/1hqpl0a/this\_is\_info\_orbs\_a\_fully\_open\_source\_esp32/](https://www.reddit.com/r/esp32/comments/1hqpl0a/this_is_info_orbs_a_fully_open_source_esp32/)  
3. Info Orbs Dev Kit \- brett.tech, fecha de acceso: mayo 18, 2026, [https://brett.tech/products/info-orbs-full-dev-kit](https://brett.tech/products/info-orbs-full-dev-kit)  
4. a fully open source esp32 based,desk display widget kit I developed (: There's a great community around it and lots of widgets being developed. You can find the github by searching "info orbs Github". I'm very proud of what this has turned into\! : r/arduino \- Reddit, fecha de acceso: mayo 18, 2026, [https://www.reddit.com/r/arduino/comments/1hqplfv/this\_is\_info\_orbs\_a\_fully\_open\_source\_esp32/](https://www.reddit.com/r/arduino/comments/1hqplfv/this_is_info_orbs_a_fully_open_source_esp32/)  
5. Info-orbs | Collection \- MakerWorld: Download Free 3D Models, fecha de acceso: mayo 18, 2026, [https://makerworld.com/en/collections/4322910-info-orbs](https://makerworld.com/en/collections/4322910-info-orbs)  
6. GitHub · Where software is built, fecha de acceso: mayo 18, 2026, [https://github.com/orgs/librosa/followers](https://github.com/orgs/librosa/followers)  
7. Info Orbs v3 \- Free 3D Print Model \- MakerWorld, fecha de acceso: mayo 18, 2026, [https://makerworld.com/en/models/832825-info-orbs-v3](https://makerworld.com/en/models/832825-info-orbs-v3)

[image1]: <data:image/png;base64,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>

[image2]: <data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAwAAAAbCAYAAABIpm7EAAAA3UlEQVR4XmNgGAWDFTADsRgQC6BLYAOTgPgTEB8B4qdAfAyIFZAVIANNIF4LxEJQPj8QHwLiU0DMAsRzgdgYKgd2whUojQzSgfg/ECsC8WoGiCFg0ACVQAeeDBDxHAaIGjAQAeKrQPwVJoAEQE4Aia9hgNgCBlZA/BOIl8IEkABMAw+yIMizb4F4IbIgFLgC8W8gZkQWBIXAHCC+jiTGCcRlDJDgfQbEgkCcDMSyMAUSQLyTARJ0e4D4PhBHMkAiMRWI9wPxdCBmhWmAAUkgFmZAcwIDJLhBmkfBIAEAfr8h+38iw0UAAAAASUVORK5CYII=>