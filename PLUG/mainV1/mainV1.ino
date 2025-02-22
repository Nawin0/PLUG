#include <Arduino.h>
#include "PZEMModule.h"
#include "RTCModule.h"
#include <WiFi.h>
#include <HTTPClient.h>

int S1 = 18;
int S2 = 5;
int S3 = 17;
int S4 = 16;
int buttonPin = 4;
int blinkPin = 2;
int Buzzer = 19;

// WiFi credentials
const char* ssid = "nawin"; // ใส่ชื่อ Wi-Fi ของคุณ
const char* password = "1234567890"; // ใส่รหัสผ่าน Wi-Fi ของคุณ

// Line Notify API
String lineToken = "7sNg5bHPozMAZVei2mQIH041u5LZ183uGiWnILb9xvn"; // ใส่ Token ที่ได้จาก Line Developers

unsigned long buttonPressStart = 0;
bool buttonPressed = false;
bool relayState = false;
unsigned long lastBlinkTime = 0;
bool pinState = false;

void setup() {
  Serial.begin(115200);

  pinMode(S1, OUTPUT);
  pinMode(S2, OUTPUT);
  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(blinkPin, OUTPUT);
  pinMode(Buzzer, OUTPUT);

  digitalWrite(S1, LOW);
  digitalWrite(S2, LOW);

  // ตั้งค่า WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  setupPZEM();
  setupRTC();
}

void sendLineNotify(String message) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin("https://notify-api.line.me/api/notify");
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    http.addHeader("Authorization", "Bearer " + lineToken);

    String httpRequestData = "message=" + message;
    int httpResponseCode = http.POST(httpRequestData);

    if (httpResponseCode > 0) {
      Serial.println("Line Notify sent successfully: " + String(httpResponseCode));
    } else {
      Serial.println("Error sending Line Notify: " + String(httpResponseCode));
    }

    http.end();
  } else {
    Serial.println("Error: Not connected to WiFi");
  }
}

String getElectricalData() {

  String electricalData = readPZEM();
  return electricalData;
}

String getCurrentTime() {

  String currentTime = getTime();
  return currentTime;
}

void loop() {

  if (digitalRead(buttonPin) == LOW) {
    if (!buttonPressed) {
      buttonPressStart = millis();
      buttonPressed = true;
    } else {
      unsigned long pressDuration = millis() - buttonPressStart;
      if (pressDuration >= 3000) {
        if (relayState) {
          relayState = false;
          digitalWrite(S1, LOW);
          digitalWrite(S2, LOW);
          Serial.println("Relay OFF");
          sendLineNotify("Relay OFF");
          digitalWrite(Buzzer, HIGH);
          delay(500);
          digitalWrite(Buzzer, LOW);
          delay(500);
        } else {
          relayState = true;
          digitalWrite(S1, HIGH);
          digitalWrite(S2, HIGH);
          Serial.println("Relay ON");
          sendLineNotify("Relay ON");
        }

        unsigned long currentMillis = millis();
        if (currentMillis - lastBlinkTime >= 500) {
          lastBlinkTime = currentMillis;
          pinState = !pinState;
          digitalWrite(blinkPin, pinState);
        }
      }
    }
  } else {
    buttonPressed = false;
  }

  // ส่งข้อมูลไปที่ Line ทุกๆ 5 วินาที
  if (millis() - lastBlinkTime >= 5000) {
    String electricalData = getElectricalData();
    String currentTime = getCurrentTime();
    String message = "เวลา: " + currentTime + "\n" + "ข้อมูลไฟฟ้า: \n" + electricalData;
    sendLineNotify(message);
    lastBlinkTime = millis();
  }

  if (!buttonPressed) {
    digitalWrite(blinkPin, LOW);
  }

  delay(200);
}
