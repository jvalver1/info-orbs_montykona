#include "MQTTWidget.h"
#include "CrashTrace.h"
#include "MQTTTranslations.h"
#include "DebugHelper.h"

#define WIDGET_PREFIX "[MQTT]"

// Initialize the static instance pointer
MQTTWidget *MQTTWidget::instance = nullptr;

// Static callback proxy to forward MQTT messages to the class instance
void MQTTWidget::staticCallback(char *topic, byte *payload, unsigned int length) {
    if (instance) {
        instance->callback(topic, payload, length);
    }
}

// Constructor
MQTTWidget::MQTTWidget(ScreenManager &manager, ConfigManager &config)
    : Widget(manager, config),
      m_drawTimer(addDrawRefreshFrequency(MQTT_DRAW_DELAY)),
      m_updateTimer(addUpdateRefreshFrequency(MQTT_UPDATE_DELAY)),
      mqttClient(wifiClient) {

    // Assign the current instance to the static pointer
    instance = this;

// Set defaults from config.h
#ifdef MQTT_WIDGET_HOST
    mqttHost = MQTT_WIDGET_HOST;
#endif
#ifdef MQTT_WIDGET_PORT
    mqttPort = MQTT_WIDGET_PORT;
#endif
#ifdef MQTT_SETUP_TOPIC
    mqttSetupTopic = MQTT_SETUP_TOPIC;
#endif
#ifdef MQTT_WIDGET_USER
    mqttUser = MQTT_WIDGET_USER;
#endif
#ifdef MQTT_WIDGET_PASS
    mqttPass = MQTT_WIDGET_PASS;
#endif
    m_enabled = (INCLUDE_MQTT == WIDGET_ON);
    m_config.addConfigBool("MqttWidget", "mqttEnabled", &m_enabled, t_enableWidget);
    m_config.addConfigString("MqttWidget", "mqttHost", &mqttHost, 30, t_mqttHost, true);
    m_config.addConfigInt("MqttWidget", "mqttPort", &mqttPort, t_mqttPort, true);
    m_config.addConfigString("MqttWidget", "mqttSetupTopic", &mqttSetupTopic, 100, t_mqttSetupTopic, true);
    m_config.addConfigString("MqttWidget", "mqttUser", &mqttUser, 20, t_mqttUser, true);
    m_config.addConfigString("MqttWidget", "mqttPass", &mqttPass, 50, t_mqttPass, true);

    // Set MQTT broker server and port
    mqttClient.setServer(mqttHost.c_str(), mqttPort);
    mqttClient.setBufferSize(2048);

    // Set the static callback proxy
    mqttClient.setCallback(staticCallback);
}

// Helper function to map color strings to uint16_t color values
uint16_t MQTTWidget::getColorFromString(const String &colorStr) {
    if (colorStr.equalsIgnoreCase("TFT_BLACK")) {
        return TFT_BLACK;
    } else if (colorStr.equalsIgnoreCase("TFT_NAVY")) {
        return TFT_NAVY;
    } else if (colorStr.equalsIgnoreCase("TFT_DARKGREEN")) {
        return TFT_DARKGREEN;
    } else if (colorStr.equalsIgnoreCase("TFT_DARKCYAN")) {
        return TFT_DARKCYAN;
    } else if (colorStr.equalsIgnoreCase("TFT_MAROON")) {
        return TFT_MAROON;
    } else if (colorStr.equalsIgnoreCase("TFT_PURPLE")) {
        return TFT_PURPLE;
    } else if (colorStr.equalsIgnoreCase("TFT_OLIVE")) {
        return TFT_OLIVE;
    } else if (colorStr.equalsIgnoreCase("TFT_LIGHTGREY")) {
        return TFT_LIGHTGREY;
    } else if (colorStr.equalsIgnoreCase("TFT_DARKGREY")) {
        return TFT_DARKGREY;
    } else if (colorStr.equalsIgnoreCase("TFT_BLUE")) {
        return TFT_BLUE;
    } else if (colorStr.equalsIgnoreCase("TFT_GREEN")) {
        return TFT_GREEN;
    } else if (colorStr.equalsIgnoreCase("TFT_CYAN")) {
        return TFT_CYAN;
    } else if (colorStr.equalsIgnoreCase("TFT_RED")) {
        return TFT_RED;
    } else if (colorStr.equalsIgnoreCase("TFT_MAGENTA")) {
        return TFT_MAGENTA;
    } else if (colorStr.equalsIgnoreCase("TFT_YELLOW")) {
        return TFT_YELLOW;
    } else if (colorStr.equalsIgnoreCase("TFT_WHITE")) {
        return TFT_WHITE;
    } else if (colorStr.equalsIgnoreCase("TFT_ORANGE")) {
        return TFT_ORANGE;
    } else if (colorStr.equalsIgnoreCase("TFT_GREENYELLOW")) {
        return TFT_GREENYELLOW;
    } else if (colorStr.equalsIgnoreCase("TFT_PINK")) {
        return TFT_PINK;
    } else if (colorStr.equalsIgnoreCase("TFT_BROWN")) {
        return TFT_BROWN;
    } else if (colorStr.equalsIgnoreCase("TFT_GOLD")) {
        return TFT_GOLD;
    } else if (colorStr.equalsIgnoreCase("TFT_SILVER")) {
        return TFT_SILVER;
    } else if (colorStr.equalsIgnoreCase("TFT_SKYBLUE")) {
        return TFT_SKYBLUE;
    } else if (colorStr.equalsIgnoreCase("TFT_VIOLET")) {
        return TFT_VIOLET;
    }
    // Add more color mappings as needed

    return TFT_BLACK; // Default color if unknown
}

void MQTTWidget::setup() {
    WIDGET_HEAP_SNAP(WIDGET_PREFIX, "after_init");
}

// Update method
void MQTTWidget::update(bool force) {
    CrashTrace::mark("mqtt:update");
    //    Log.traceln("Inside update method - %s" + mqttClient.connected().c_str());

    if (!mqttClient.connected()) {
        reconnect();
    }
    mqttClient.loop(); // Process incoming MQTT messages
}

// Draw method: Draws all orbs (can be used for initial drawing or full refresh)
void MQTTWidget::draw(bool force) {
    CrashTrace::mark("mqtt:draw");
    m_manager.setFont(DEFAULT_FONT);

    if (force) {
        for (const auto &orb : orbConfigs) {
            drawOrb(orb.orbid);
        }
    }
}

// ChangeMode method
void MQTTWidget::changeMode() {
    // Implement mode changes if applicable
    draw(true);
}

// Callback function for MQTT messages
void MQTTWidget::callback(char *topic, byte *payload, unsigned int length) {
    // Convert payload to String
    String message;
    for (unsigned int i = 0; i < length; i++) {
        message += (char) payload[i];
    }

    String receivedTopic = String(topic);
    WIDGET_LOG_TRACE(WIDGET_PREFIX, "Message arrived [%s]: %s", receivedTopic.c_str(), message.c_str());

    if (receivedTopic.equals(mqttSetupTopic.c_str())) {
        handleSetupMessage(message);
    } else {
        // Handle data messages for orbs
        auto it = orbDataMap.find(receivedTopic);
        if (it != orbDataMap.end()) {
            // Identify the orb configuration associated with this topic
            OrbConfig *orb = nullptr;
            for (auto &o : orbConfigs) {
                if (o.topicSrc.equals(receivedTopic)) {
                    orb = &o;
                    break;
                }
            }

            if (orb) {
                if (orb->jsonField.length() > 0) {
                    // The orb expects a specific JSON field
                    WIDGET_HEAP_SNAP(WIDGET_PREFIX, "pre-deserialization");
                    JsonDocument dataDoc;
                    DeserializationError dataError = deserializeJson(dataDoc, message);
                    if (dataError) {
                        WIDGET_LOG_ERROR(WIDGET_PREFIX, "Failed to parse data JSON: %s", dataError.c_str());
                        return;
                    }
                    WIDGET_HEAP_SNAP(WIDGET_PREFIX, "post-deserialization");

                    // Extract the specified JSON field using dynamic path traversal
                    JsonVariant fieldValue = dataDoc.as<JsonVariant>();
                    char path[100];
                    strcpy(path, orb->jsonField.c_str());
                    char *token = strtok(path, ".");

                    // Traverse the path tokens to get the desired field value
                    while (token != nullptr && !fieldValue.isNull()) {
                        // Check if the token contains an array index (e.g., "power_delivered[0]")
                        char *arrayBracket = strchr(token, '[');
                        if (arrayBracket) {
                            *arrayBracket = '\0'; // Null terminate to get the key part
                            fieldValue = fieldValue[token]; // Access the array key

                            // Extract the index part
                            int index = atoi(arrayBracket + 1); // Get the number after '['
                            if (fieldValue.is<JsonArray>()) {
                                fieldValue = fieldValue[index]; // Access the array element by index
                            } else {
                                WIDGET_LOG_ERROR(WIDGET_PREFIX, "Error: Expected an array for %s", token);
                                return;
                            }
                        } else {
                            // Regular object traversal
                            fieldValue = fieldValue[token];
                        }
                        token = strtok(nullptr, ".");
                    }

                    // If we successfully extracted the value
                    if (!fieldValue.isNull()) {
                        String extractedValue;
                        if (fieldValue.is<float>()) {
                            extractedValue = String(fieldValue.as<float>(), 3); // 3 decimal places
                        } else if (fieldValue.is<int>()) {
                            extractedValue = String(fieldValue.as<int>());
                        } else if (fieldValue.is<const char *>()) {
                            extractedValue = String(fieldValue.as<const char *>());
                        } else {
                            extractedValue = String(fieldValue.as<String>());
                        }

                        // Compare with the stored value to decide if we need to update
                        if (orb->lastValuesMap[orb->jsonField] != extractedValue) {
                            // Store the new value
                            orb->lastValuesMap[orb->jsonField] = extractedValue;

                            // Update the display only if the value has actually changed
                            it->second = extractedValue;
                            WIDGET_LOG_TRACE(WIDGET_PREFIX, "Parsed %s : %s", orb->jsonField.c_str(), extractedValue.c_str());

                            // Redraw the orb with updated data
                            drawOrb(orb->orbid);
                        } else {
                            WIDGET_LOG_TRACE(WIDGET_PREFIX, "No change detected for field: %s", orb->jsonField.c_str());
                        }
                    } else {
                        WIDGET_LOG_WARN(WIDGET_PREFIX, "JSON field '%s' not found in payload.", orb->jsonField.c_str());
                        return;
                    }
                } else {
                    // The orb does not expect a JSON field; use the entire payload
                    if (it->second != message) {
                        it->second = message;
                        WIDGET_LOG_TRACE(WIDGET_PREFIX, "Updated data for %s : %s", receivedTopic.c_str(), message.c_str());
                        drawOrb(orb->orbid);
                    } else {
                        WIDGET_LOG_TRACE(WIDGET_PREFIX, "No change detected for topic: %s", receivedTopic.c_str());
                    }
                }
            } else {
                WIDGET_LOG_WARN(WIDGET_PREFIX, "No orb configuration found for topic: %s", receivedTopic.c_str());
            }
        } else {
            WIDGET_LOG_TRACE(WIDGET_PREFIX, "Received message for unknown topic: %s", receivedTopic.c_str());
        }
    }
}

// Handle setup message to configure orbs
void MQTTWidget::handleSetupMessage(const String &message) {
    //    WIDGET_LOG_TRACE(WIDGET_PREFIX, "Handling setup message...");

    // Parse JSON configuration
    WIDGET_HEAP_SNAP(WIDGET_PREFIX, "pre-setup-deserialization");
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, message);

    if (error) {
        WIDGET_LOG_ERROR(WIDGET_PREFIX, "Failed to parse setup JSON: %s", error.c_str());
        return;
    }
    WIDGET_HEAP_SNAP(WIDGET_PREFIX, "post-setup-deserialization");

    // Clear existing configurations and data
    orbConfigs.clear();
    orbDataMap.clear();

    // Parse "orbs" array
    JsonArray orbs = doc["orbs"].as<JsonArray>();
    for (JsonObject orbObj : orbs) {
        OrbConfig config;
        config.orbid = orbObj["orbid"].as<int>();
        config.topicSrc = orbObj["topicsrc"].as<String>();
        config.orbvalunit = orbObj["orbvalunit"].as<String>();
        config.orbdesc = orbObj["orbdesc"].as<String>();
        config.xpostxt = orbObj["xpostxt"].as<int>();
        config.ypostxt = orbObj["ypostxt"].as<int>();
        config.xposval = orbObj["xposval"].as<int>();
        config.yposval = orbObj["yposval"].as<int>();
        config.orbsize = orbObj["orbsize"].as<int>();
        String bgColorStr = orbObj["orb-bg"].as<String>();
        String textColorStr = orbObj["orb-textcol"].as<String>();

        // Parse "jsonfield" (optional)
        if (orbObj["jsonfield"].is<String>()) {
            config.jsonField = orbObj["jsonfield"].as<String>();
        } else {
            config.jsonField = ""; // Default to empty if not provided
        }

        // Convert color strings to actual color values using helper function
        config.orbBgColor = getColorFromString(bgColorStr);
        config.orbTextColor = getColorFromString(textColorStr);

        orbConfigs.push_back(config);
        WIDGET_LOG_INFO(WIDGET_PREFIX, "Configured Orb: %d -> %s", config.orbid, config.orbdesc.c_str());

        // Initialize data map with empty strings
        orbDataMap[config.topicSrc] = "";
    }

    buttonTopic = doc["buttontopic"].as<String>();

    // Subscribe to all configured topics
    subscribeToOrbs();

    // Trigger a redraw to display the configured orbs
    draw(true);
}

// Subscribe to all orb topics
void MQTTWidget::subscribeToOrbs() {
    //    WIDGET_LOG_TRACE(WIDGET_PREFIX, "Inside subscribeToOrbs method");

    for (const auto &orb : orbConfigs) {
        bool success = mqttClient.subscribe(orb.topicSrc.c_str());
        if (success) {
            WIDGET_LOG_TRACE(WIDGET_PREFIX, "Subscribed to topic: %s", orb.topicSrc.c_str());
        } else {
            WIDGET_LOG_WARN(WIDGET_PREFIX, "Failed to subscribe to topic: %s", orb.topicSrc.c_str());
        }
    }
}

#define RECONNECT_INTERVAL 5000
// Handle MQTT reconnection
void MQTTWidget::reconnect() {
    //    WIDGET_LOG_TRACE(WIDGET_PREFIX, "Inside reconnect method");

    // Loop until reconnected
    if (!mqttClient.connected()) {
        // Check if it's time to attempt a reconnection
        unsigned long now = millis();
        if (now - lastReconnectAttempt < RECONNECT_INTERVAL) {
            return;
        }

        WIDGET_LOG_TRACE(WIDGET_PREFIX, "Attempting MQTT connection...");
        lastReconnectAttempt = now;

        // Generate a random client ID
        String clientId = "MQTTWidgetClient-";
        clientId += String(random(0xffff), HEX);

        bool connected;

        // Check if username and password are provided
        if (!mqttUser.empty() && !mqttPass.empty()) {
            // Attempt to connect with username and password
            connected = mqttClient.connect(clientId.c_str(), mqttUser.c_str(), mqttPass.c_str());
            WIDGET_LOG_TRACE(WIDGET_PREFIX, "Attempting MQTT connection with authentication...");
        } else {
            // Attempt to connect without authentication
            connected = mqttClient.connect(clientId.c_str());
            WIDGET_LOG_TRACE(WIDGET_PREFIX, "Attempting MQTT connection without authentication...");
        }

        // Check the result of the connection attempt
        if (connected) {
            WIDGET_LOG_TRACE(WIDGET_PREFIX, "MQTT connected");
            // Once connected, subscribe to the setup topic
            if (mqttClient.subscribe(mqttSetupTopic.c_str())) {
                WIDGET_LOG_TRACE(WIDGET_PREFIX, "Subscribed to setup topic2: %s", mqttSetupTopic.c_str());
            } else {
                WIDGET_LOG_WARN(WIDGET_PREFIX, "Failed to subscribe to setup topic: %s", mqttSetupTopic.c_str());
            }
        } else {
            WIDGET_LOG_WARN(WIDGET_PREFIX, "failed, rc=%d", mqttClient.state());
            WIDGET_LOG_WARN(WIDGET_PREFIX, "try again in 5 seconds");
        }
    }
}

// New method to draw a single orb based on orbid
void MQTTWidget::drawOrb(int orbid) {
    //    WIDGET_LOG_TRACE(WIDGET_PREFIX, "Inside drawOrb method");

    // Select the screen corresponding to the orbid
    m_manager.selectScreen(orbid);

    // Find the orb configuration
    OrbConfig *orb = nullptr;
    for (auto &o : orbConfigs) {
        if (o.orbid == orbid) {
            orb = &o;
            break;
        }
    }

    if (orb == nullptr) {
        WIDGET_LOG_WARN(WIDGET_PREFIX, "Orb not found for orbid: %d", orbid);
        return;
    }

    m_manager.fillScreen(orb->orbBgColor);

    // Define the position and size of the orb (adjust as needed)
    int x = 0; // Starting X position
    int y = 0; // Starting Y position
    int width = 240; // Width of the orb
    int height = 240; // Height of the orb
    int centre = 120;
    int screenWidth = SCREEN_SIZE;
    // int screenHeight = display.height();

    // Clear the display area with the background color
    // display.fillRect(x, y, screenWidth, screenHeight, orb->orbBgColor);

    // Set text properties
    m_manager.setFontColor(orb->orbTextColor, orb->orbBgColor);
    // m_manager.setTextSize(orb->orbsize);

    // Display orb description/title
    // display.drawString(orb->orbdesc, centre, orb->xpostxt, orb->ypostxt);
    m_manager.drawString(orb->orbdesc, orb->xpostxt, orb->ypostxt, orb->orbsize, Align::MiddleCenter);
    // m_manager.drawString(orb->orbdesc, centre, orb->ypostxt, orb->orbsize, Align::MiddleCenter);

    // Display orb data
    String data = orbDataMap[orb->topicSrc];
    // display.drawString(data + orb->orbvalunit, centre, orb->xposval, orb->yposval);
    m_manager.drawString(data + orb->orbvalunit, orb->xposval, orb->yposval, orb->orbsize, Align::MiddleCenter);
}

String MQTTWidget::getName() {
    return "MQTTWidget";
}

const char *const buttonNames[] = {"invalid", "left", "middle", "right"};
#define NAME_SIZE ((sizeof(buttonNames) / sizeof(buttonNames[0])) - 1)
const char *const buttonStates[] = {"nothing", "short", "medium", "long"};
#define STATE_SIZE ((sizeof(buttonStates) / sizeof(buttonStates[0])) - 1)

void MQTTWidget::buttonPressed(uint8_t buttonId, ButtonState state) {
    if (!buttonTopic.isEmpty()) {
        // generate a json dictionary to publish to the button topic
        JsonDocument doc;
        doc["button"] = buttonNames[buttonId > NAME_SIZE ? 0 : buttonId];
        doc["state"] = buttonStates[state > STATE_SIZE ? 0 : state];
        // Serialize the JSON document to a string
        char buffer[256];
        size_t n = serializeJson(doc, buffer);
        // Publish the serialized JSON string
        mqttClient.publish(buttonTopic.c_str(), buffer, false);
    } else {
        if (buttonId == BUTTON_OK && state == BTN_SHORT)
            changeMode();
    }
}
