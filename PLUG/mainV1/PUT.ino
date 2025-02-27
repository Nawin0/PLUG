#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

const char* ssid = "PLUG";
const char* password = "1234567890";

int relay1 = 0;
int relay2 = 0;
int relay3 = 0;
int relay4 = 0;

String serverUrl = "http://apiplug.nareubad.work/";

unsigned long previousMillis = 0;
const long interval = 4000;

void setup() {
  Serial.begin(115200);

  WiFi.begin(ssid, password);
  Serial.println();
  Serial.print("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected to WiFi");

  displayRelayStatus();
}

void loop() {
  unsigned long currentMillis = millis();

  if (WiFi.status() == WL_CONNECTED) {
    if (currentMillis - previousMillis >= interval) {
      previousMillis = currentMillis;

      relay1 = (relay1 == 1) ? 0 : 1;
      relay2 = (relay2 == 1) ? 0 : 1;
      relay3 = (relay3 == 1) ? 0 : 1;
      relay4 = (relay4 == 1) ? 0 : 1;

      sendRelayStateToAPI();
      displayRelayStatus();
    }
  } else {
    Serial.println("WiFi Disconnected");
  }

  delay(100);
}

void sendRelayStateToAPI() {
  HTTPClient http;
  String url = serverUrl + String("devices_details/");
  http.begin(url);

  StaticJsonDocument<512> doc;
  JsonArray arr = doc.to<JsonArray>();

  JsonObject relay1State = arr.createNestedObject();
  relay1State["devices_id"] = 1;
  relay1State["channel_id"] = 1;
  relay1State["channel_status"] = (relay1 == 1) ? "on" : "off";

  JsonObject relay2State = arr.createNestedObject();
  relay2State["devices_id"] = 1;
  relay2State["channel_id"] = 2;
  relay2State["channel_status"] = (relay2 == 1) ? "on" : "off";

  JsonObject relay3State = arr.createNestedObject();
  relay3State["devices_id"] = 1;
  relay3State["channel_id"] = 3;
  relay3State["channel_status"] = (relay3 == 1) ? "on" : "off";

  JsonObject relay4State = arr.createNestedObject();
  relay4State["devices_id"] = 1;
  relay4State["channel_id"] = 4;
  relay4State["channel_status"] = (relay4 == 1) ? "on" : "off";

  String payload;
  serializeJson(doc, payload);

  http.addHeader("Content-Type", "application/json");

  int httpCode = http.PUT(payload);

  if (httpCode == 200) {
    Serial.println("PUT request successful");
  } else {
    Serial.println("Error on HTTP request: " + String(httpCode));
  }

  http.end();
}

void displayRelayStatus() {
  Serial.print("Relay1 Status: ");
  Serial.println(relay1 == 1 ? "ON" : "OFF");

  Serial.print("Relay2 Status: ");
  Serial.println(relay2 == 1 ? "ON" : "OFF");

  Serial.print("Relay3 Status: ");
  Serial.println(relay3 == 1 ? "ON" : "OFF");

  Serial.print("Relay4 Status: ");
  Serial.println(relay4 == 1 ? "ON" : "OFF");
}
