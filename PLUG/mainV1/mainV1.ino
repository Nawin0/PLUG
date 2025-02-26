#include <Arduino.h>
#include "PZEMModule.h"
#include "RTCModule.h"
#include <WiFi.h>
#include <WiFiManager.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

int S1 = 18;
int S2 = 5;
int S3 = 32;
int S4 = 15;
int buttonPin = 4;
int blinkPin = 2;
int Buzzer = 19;

const char* serverUrl = "https://apiplug.nareubad.work/";
HTTPClient http;

WiFiManager wifiManager;

unsigned long buttonPressStart = 0;
bool buttonPressed = false;
bool relayState = false;
unsigned long lastBlinkTime = 0;
unsigned long buttonPressDuration = 0;
bool isWifiReset = false;
bool inConfigPortal = false;

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
    Serial.println("Failed to connect to Wi-Fi. Restarting ESP32...");
    delay(1000);
    ESP.restart();
  }

  setupPZEM();
  setupRTC();

  xTaskCreatePinnedToCore(
    sendDataToAPI_Task,
    "SendDataToAPI",
    10000,
    NULL,
    1,
    NULL,
    1);

  xTaskCreatePinnedToCore(
    blinkLED_Task,
    "BlinkLED",
    2048,
    NULL,
    1,
    NULL,
    1);
}

void sendDataToAPI(float voltage, float current, float power, float energy, float frequency, float powerFactor, String currentTime) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = serverUrl + String("devices_logs/");

    http.begin(url);
    http.addHeader("Content-Type", "application/json");

    StaticJsonDocument<300> jsonDoc;
    jsonDoc["devices_id"] = 1;
    jsonDoc["voltage_value"] = voltage;
    jsonDoc["current_value"] = current;
    jsonDoc["power_value"] = power;
    jsonDoc["energy_value"] = energy;
    jsonDoc["frequency_value"] = frequency;
    jsonDoc["power_factor_value"] = powerFactor;

    String jsonData;
    serializeJson(jsonDoc, jsonData);
    Serial.println(jsonData);

    int httpResponseCode = http.POST(jsonData);

    if (httpResponseCode > 0) {
      Serial.println("Data sent to API successfully: " + String(httpResponseCode));
    } else {
      Serial.println("Error sending data to API: " + String(httpResponseCode));
    }

    http.end();
  } else {
    Serial.println("Error: Not connected to WiFi");
  }
}

void sendDataToAPI_Task(void* parameter) {
  while (true) {
    if (millis() - lastBlinkTime >= 300000) {

      if (!relayState) {
        current = power = energy = frequency = powerFactor = 0;
        Serial.println("Relay OFF: Reset energy data, no API call.");
      } else {
        voltage = pzem.voltage();
        current = pzem.current();
        power = pzem.power();
        energy = pzem.energy();
        frequency = pzem.frequency();
        powerFactor = pzem.pf();

        currentTime = getTime();
      }
      if (WiFi.status() == WL_CONNECTED) {
        sendDataToAPI(voltage, current, power, energy, frequency, powerFactor, currentTime);
      } else if (WiFi.status() != WL_CONNECTED) {
        checkconnectToWiFi();
      }

      lastBlinkTime = millis();
    }
    delay(200);
  }
}

void blinkLED_Task(void* parameter) {
  while (true) {
    if (inConfigPortal) {
      digitalWrite(blinkPin, !digitalRead(blinkPin));
      delay(200);
    } else {
      digitalWrite(blinkPin, LOW);
      delay(500);
    }
  }
}

void checkconnectToWiFi() {
  wifiManager.setTimeout(10);
  if (!wifiManager.autoConnect("PLUG")) {
    Serial.println("Failed to connect, waiting...");
  } else {
    Serial.println("Connected to WiFi.");
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
      if (blinkInterval % 2 == 0) {
        digitalWrite(blinkPin, HIGH);
      } else {
        digitalWrite(blinkPin, LOW);
      }

      if (buttonPressDuration >= 5000 && !isWifiReset) {
        Serial.println("Button pressed for 5 seconds. Resetting WiFi...");
        inConfigPortal = true;
        wifiManager.resetSettings();
        wifiManager.startConfigPortal();
        inConfigPortal = false;
        isWifiReset = true;
      }

      if (buttonPressDuration >= 3000 && buttonPressDuration < 5000) {
        if (relayState) {
          relayState = false;
          digitalWrite(S1, LOW);
          digitalWrite(S2, LOW);
          digitalWrite(S3, LOW);
          digitalWrite(S4, LOW);
          delay(2000);
          Serial.println("Relay OFF");

        } else {
          relayState = true;
          digitalWrite(S1, HIGH);
          digitalWrite(S2, HIGH);
          digitalWrite(S3, HIGH);
          digitalWrite(S4, HIGH);
          digitalWrite(Buzzer, HIGH);
          delay(500);
          digitalWrite(Buzzer, LOW);
          delay(2000);
          Serial.println("Relay ON");
        }
      }
    }
  } else {
    buttonPressed = false;
    digitalWrite(blinkPin, LOW);
  }

  delay(200);
}
