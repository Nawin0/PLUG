#include <Arduino.h>
#include "PZEMModule.h"
#include "RTCModule.h"
#include <WiFi.h>
#include <WiFiManager.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

int S1 = 18, S2 = 5, S3 = 32, S4 = 15;
int buttonPin = 4, blinkPin = 2, Buzzer = 19;
int value_max = 0;
int value_min = 0;
String device = "1";

const char* serverUrl = "https://apiplug.nareubad.work/";
WiFiManager wifiManager;
HTTPClient http;

bool relayState = false;
bool buttonPressed = false;
bool isWifiReset = false;
bool inConfigPortal = false;

unsigned long lastBlinkTime = 0;
unsigned long buttonPressStart = 0;
unsigned long buttonPressDuration = 0;

float voltage, current, power, energy, frequency, powerFactor;
String currentTime = "";

void setup() {
  Serial.begin(115200);

  pinMode(S1, OUTPUT);
  pinMode(S2, OUTPUT);
  pinMode(S3, OUTPUT);
  pinMode(S4, OUTPUT);
  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(blinkPin, OUTPUT);
  pinMode(Buzzer, OUTPUT);

  digitalWrite(S1, LOW);
  digitalWrite(S2, LOW);
  digitalWrite(S3, LOW);
  digitalWrite(S4, LOW);

  wifiManager.setTimeout(120);
  if (!wifiManager.autoConnect("PLUG")) {
    Serial.println("WiFi connect failed. Restarting ESP32...");
    delay(1000);
    ESP.restart();
  }

  setupPZEM();
  setupRTC();

  xTaskCreatePinnedToCore(sendDataToAPI_Task, "SendDataToAPI", 10000, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(blinkLED_Task, "BlinkLED", 2048, NULL, 1, NULL, 1);

  getValueAPI();
  getRelayStatusFromAPI();
}

void checkWiFiConnection() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected, reconnecting...");
    wifiManager.setTimeout(10);
    if (!wifiManager.autoConnect("PLUG")) {
      Serial.println("WiFi reconnect failed.");
    } else {
      Serial.println("WiFi reconnected.");
    }
  }
}

void sendDataToAPI(float voltage, float current, float power, float energy, float frequency, float powerFactor, String currentTime) {
  checkWiFiConnection();
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = serverUrl + String("devices/") + device;
    http.begin(url);
    http.addHeader("Content-Type", "application/json");

    StaticJsonDocument<300> jsonDoc;
    jsonDoc["id"] = 1;
    jsonDoc["device_status"] = WL_CONNECTED ? "on" : "off";
    jsonDoc["voltage_value"] = voltage;
    jsonDoc["current_value"] = current;
    jsonDoc["power_value"] = power;
    jsonDoc["energy_value"] = energy;
    jsonDoc["frequency_value"] = frequency;
    jsonDoc["power_factor_value"] = powerFactor;
    jsonDoc["update_at"] = currentTime;

    String jsonData;
    serializeJson(jsonDoc, jsonData);
    Serial.println("Sending data: " + jsonData);

    int httpResponseCode = http.POST(jsonData);

    if (httpResponseCode > 0) {
      Serial.println("Data sent successfully: " + String(httpResponseCode));
    } else {
      Serial.println("Error sending data: " + String(httpResponseCode));
    }

    http.end();
  } else {
    Serial.println("Cannot send data: WiFi not connected.");
  }
  getRelayStatusFromAPI();
  getValueAPI();
}

void getRelayStatusFromAPI() {
  checkWiFiConnection();

  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = serverUrl + String("devices_details/") + device;
    http.begin(url);

    int httpCode = http.GET();
    if (httpCode > 0) {
      String payload = http.getString();
      Serial.println("API Response: " + payload);

      StaticJsonDocument<512> doc;
      DeserializationError error = deserializeJson(doc, payload);

      if (!error) {
        for (JsonObject obj : doc.as<JsonArray>()) {
          int channel_id = obj["channel_id"];
          String status = obj["channel_status"];
          bool relayStatus = (status == "on");

          switch (channel_id) {
            case 1: digitalWrite(S1, relayStatus); break;
            case 2: digitalWrite(S2, relayStatus); break;
            case 3: digitalWrite(S3, relayStatus); break;
            case 4: digitalWrite(S4, relayStatus); break;
          }

          Serial.print("Relay ");
          Serial.print(channel_id);
          Serial.print(" Status: ");
          Serial.println(status);
        }
      } else {
        Serial.println("JSON parsing failed!");
      }
    } else {
      Serial.println("Failed to get data from API");
    }
    http.end();
  }
}

void getValueAPI() {
  checkWiFiConnection();

  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = serverUrl + String("value_controllers/")  + device;
    http.begin(url);

    int httpCode = http.GET();
    if (httpCode > 0) {
      String payload = http.getString();
      Serial.println("API Response: " + payload);

      StaticJsonDocument<512> doc;
      DeserializationError error = deserializeJson(doc, payload);

      if (!error) {
        for (JsonObject obj : doc.as<JsonArray>()) {
          value_max = obj["value_max"];
          value_min = obj["value_min"];

          Serial.print("Max Value: ");
          Serial.print(value_max);
          Serial.print(", Min Value: ");
          Serial.println(value_min);
        }
      } else {
        Serial.println("JSON parsing failed!");
      }
    } else {
      Serial.println("Failed to get data from API");
    }
    http.end();
  }
}


void sendRelayStateToAPI() {
  HTTPClient http;
  String url = serverUrl + String("devices_details/") + device;
  http.begin(url);

  StaticJsonDocument<512> doc;
  JsonArray arr = doc.to<JsonArray>();

  JsonObject relay1State = arr.createNestedObject();
  relay1State["devices_id"] = 1;
  relay1State["channel_id"] = 1;
  relay1State["channel_status"] = (digitalRead(S1) == HIGH) ? "on" : "off";

  JsonObject relay2State = arr.createNestedObject();
  relay2State["devices_id"] = 1;
  relay2State["channel_id"] = 2;
  relay2State["channel_status"] = (digitalRead(S2) == HIGH) ? "on" : "off";

  JsonObject relay3State = arr.createNestedObject();
  relay3State["devices_id"] = 1;
  relay3State["channel_id"] = 3;
  relay3State["channel_status"] = (digitalRead(S3) == HIGH) ? "on" : "off";

  JsonObject relay4State = arr.createNestedObject();
  relay4State["devices_id"] = 1;
  relay4State["channel_id"] = 4;
  relay4State["channel_status"] = (digitalRead(S4) == HIGH) ? "on" : "off";

  String payload;
  serializeJson(doc, payload);
  Serial.println("Sending relay state: " + payload);

  http.addHeader("Content-Type", "application/json");

  int httpCode = http.PUT(payload);

  if (httpCode == 200) {
    Serial.println("PUT request successful");
  } else {
    Serial.println("Error on HTTP request: " + String(httpCode));
  }

  http.end();
}

void checkBuzzerAlert(float value) {
  if (power > value_max) {
    digitalWrite(Buzzer, HIGH);
    Serial.println("buzzer ON");
    Serial.println("MAX");
    Serial.println(power);
    Serial.println(value_max);
  } else if (power < value_min) {
    digitalWrite(Buzzer, HIGH);
    Serial.println("buzzer ON");
    Serial.println("MIN");
    Serial.println(power);
    Serial.println(value_min);
  } else {
    digitalWrite(Buzzer, LOW);
  }
}


void sendDataToAPI_Task(void* parameter) {
  while (true) {
    voltage = pzem.voltage();
    current = pzem.current();
    power = pzem.power();
    energy = pzem.energy();
    frequency = pzem.frequency();
    powerFactor = pzem.pf();
    currentTime = getTime();

    checkBuzzerAlert(power);

    sendDataToAPI(voltage, current, power, energy, frequency, powerFactor, currentTime);

    delay(5000);
  }
}

void blinkLED_Task(void* parameter) {
  while (true) {
    digitalWrite(blinkPin, inConfigPortal ? !digitalRead(blinkPin) : LOW);
    delay(inConfigPortal ? 200 : 500);
  }
}

void loop() {
  if (digitalRead(buttonPin) == LOW) {
    if (!buttonPressed) {
      buttonPressStart = millis();
      buttonPressed = true;
    } else {
      buttonPressDuration = millis() - buttonPressStart;
      unsigned long blinkInterval = buttonPressDuration / 800;
      digitalWrite(blinkPin, (blinkInterval % 2 == 0));

      if (buttonPressDuration >= 5000 && !isWifiReset) {
        Serial.println("Resetting WiFi...");
        inConfigPortal = true;
        wifiManager.resetSettings();
        wifiManager.startConfigPortal();
        inConfigPortal = false;
        isWifiReset = true;
      }

      if (buttonPressDuration >= 3000 && buttonPressDuration < 5000) {
        relayState = !relayState;
        digitalWrite(S1, relayState);
        digitalWrite(S2, relayState);
        digitalWrite(S3, relayState);
        digitalWrite(S4, relayState);

        if (relayState) {
          digitalWrite(Buzzer, HIGH);
          delay(500);
          digitalWrite(Buzzer, LOW);
        }

        Serial.println(relayState ? "Relay ON" : "Relay OFF");
        sendRelayStateToAPI();
      }
    }
  } else {
    buttonPressed = false;
    digitalWrite(blinkPin, LOW);
  }

  delay(200);
}
