
#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <DHT.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "secrets.h"

#define DHT_PIN 15
#define FAN_PIN 2

const char* WIFI_SSID = "Wokwi-GUEST";
const char* WIFI_PASSWORD = "";

const char* API_URL =
    "http://iot-skyethomas.mexicocentral.cloudapp.azure.com";

const char* MQTT_NAME = "skyethomas";
const char* MQTT_SERVER = "135.232.203.144";
const int MQTT_PORT = 1883;

const unsigned long READING_INTERVAL = 10000;
const unsigned long RETRY_INTERVAL = 5000;

DHT dht(DHT_PIN, DHT22);
WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);

String mac;
String statusTopic;
String settingsTopic;
String stateTopic;

String fanMode = "off";
float maxHumidity = 65.0;
float currentHumidity = NAN;

bool settingsReceived = false;
bool fanOn = false;
bool statePublished = false;

unsigned long lastReading = 0;
unsigned long lastMqttAttempt = 0;

// B4 and B5: Control LED and publish state when it changes
void updateFan() {
    bool newState = false;

    if (settingsReceived) {
        if (fanMode == "on") {
            newState = true;
        } else if (fanMode == "auto") {
            newState = !isnan(currentHumidity) &&
                       currentHumidity > maxHumidity;
        }
    }

    digitalWrite(FAN_PIN, newState ? HIGH : LOW);

    if (newState != fanOn) {
        fanOn = newState;
        statePublished = false;
    }

    if (!statePublished && mqtt.connected()) {
        if (mqtt.publish(
                stateTopic.c_str(),
                fanOn ? "on" : "off",
                true
            )) {
            statePublished = true;
        }
    }
}

// B3: Receive and validate MQTT settings
void mqttCallback(char* topic, byte* payload, unsigned int length) {
    String message;

    for (unsigned int i = 0; i < length; i++) {
        message += (char)payload[i];
    }

    Serial.print("MQTT received: ");
    Serial.println(message);

    if (String(topic) != settingsTopic) {
        return;
    }

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, message);

    if (error ||
        !doc["mode"].is<const char*>() ||
        !(doc["max_humidity"].is<int>() ||
          doc["max_humidity"].is<float>() ||
          doc["max_humidity"].is<double>())) {
        Serial.println("Invalid settings JSON - ignored");
        return;
    }

    String newMode = doc["mode"].as<String>();
    float newMax = doc["max_humidity"].as<float>();

    if (newMode != "auto" &&
        newMode != "on" &&
        newMode != "off") {
        Serial.println("Invalid mode - ignored");
        return;
    }

    if (newMax < 0 || newMax > 100) {
        Serial.println("Invalid humidity limit - ignored");
        return;
    }

    fanMode = newMode;
    maxHumidity = newMax;
    settingsReceived = true;

    Serial.printf(
        "Mode: %s, Max humidity: %.1f\n",
        fanMode.c_str(),
        maxHumidity
    );

    updateFan();
}

// B2: Connect to MQTT and subscribe to settings
void connectMQTT() {
    if (mqtt.connected()) {
        return;
    }

    if (millis() - lastMqttAttempt < RETRY_INTERVAL) {
        return;
    }

    lastMqttAttempt = millis();

    String clientId =
        "bathroom-" + String(MQTT_NAME) + "-" +
        String((uint32_t)ESP.getEfuseMac(), HEX);

    Serial.println("Connecting to MQTT...");

    if (mqtt.connect(
            clientId.c_str(),
            statusTopic.c_str(),
            1,
            true,
            "offline"
        )) {
        Serial.println("MQTT connected");

        mqtt.publish(statusTopic.c_str(), "online", true);

        bool subscribed = mqtt.subscribe(settingsTopic.c_str());

        Serial.print("Subscribed to settings: ");
        Serial.println(subscribed ? "YES" : "NO");

        statePublished = false;
        updateFan();
    } else {
        Serial.printf("MQTT failed, state: %d\n", mqtt.state());
    }
}

// B1: Send humidity reading every 10 seconds
void sendReading(float humidity) {
    if (WiFi.status() != WL_CONNECTED) {
        return;
    }

    HTTPClient http;

    String url = String(API_URL) + "/readings";
    http.begin(url);

    http.addHeader("Content-Type", "application/json");
    http.addHeader("X-API-Key", DEVICE_KEY);

    String body =
        "{\"mac\":\"" + mac +
        "\",\"humidity\":" +
        String(humidity, 2) + "}";

    int response = http.POST(body);

    Serial.print("POST /readings: ");
    Serial.println(response);

    if (response < 0) {
        Serial.println(http.errorToString(response));
    } else {
        Serial.println(http.getString());
    }

    http.end();
}

void setup() {
    Serial.begin(115200);

    pinMode(FAN_PIN, OUTPUT);
    digitalWrite(FAN_PIN, LOW);

    dht.begin();

    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    Serial.print("Connecting to Wi-Fi");

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("\nWi-Fi connected");

    mac = WiFi.macAddress();

    Serial.print("ESP32 MAC: ");
    Serial.println(mac);

    String baseTopic = String("ecse3038/") + MQTT_NAME;

    statusTopic = baseTopic + "/status";
    settingsTopic = baseTopic + "/fan/settings";
    stateTopic = baseTopic + "/fan/state";

    mqtt.setServer(MQTT_SERVER, MQTT_PORT);
    mqtt.setCallback(mqttCallback);
    mqtt.setBufferSize(256);
    mqtt.setKeepAlive(30);

    lastMqttAttempt = millis() - RETRY_INTERVAL;
    lastReading = millis() - READING_INTERVAL;

    connectMQTT();
}

void loop() {
    if (WiFi.status() != WL_CONNECTED) {
        WiFi.reconnect();
        delay(500);
        return;
    }

    if (!mqtt.connected()) {
        connectMQTT();
    }

    mqtt.loop();

    if (millis() - lastReading >= READING_INTERVAL) {
        lastReading = millis();

        float humidity = dht.readHumidity();

        if (isnan(humidity)) {
            Serial.println("DHT22 reading failed");
            return;
        }

        currentHumidity = humidity;

        Serial.printf("Humidity: %.1f%%\n", humidity);

        updateFan();
        sendReading(humidity);
    }
}
