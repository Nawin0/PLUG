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
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    String url = serverUrl + String("devices_details/");
    http.begin(url);

    int httpCode = http.GET();

    if (httpCode == 200) {
      String payload = http.getString();
      Serial.println("Response: " + payload);

      StaticJsonDocument<512> doc;
      DeserializationError error = deserializeJson(doc, payload);

      if (error) {
        Serial.println("Failed to parse JSON");
        return;
      }

      // วนลูปแยกค่าจาก JSON
      for (JsonObject elem : doc.as<JsonArray>()) {
        int channelId = elem["channel_id"];
        String channelStatus = elem["channel_status"].as<String>();

        if (channelId == 1) {
          relay1 = (channelStatus == "on") ? 1 : 0;
        }
        if (channelId == 2) {
          relay2 = (channelStatus == "on") ? 1 : 0; 
        }
        if (channelId == 3) {
          relay3 = (channelStatus == "on") ? 1 : 0;
        }
        if (channelId == 4) {
          relay4 = (channelStatus == "on") ? 1 : 0;
        }
      }

      displayRelayStatus(); 

    } else {
      displayRelayStatus();
      Serial.println("Error on HTTP request");
    }

    http.end();
  } else {
    Serial.println("WiFi Disconnected");
  }

  delay(1000);
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
