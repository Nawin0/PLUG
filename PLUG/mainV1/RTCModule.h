#ifndef RTCMODULE_H
#define RTCMODULE_H

#include <Wire.h>
#include <RTClib.h>

// กำหนด SDA และ SCL สำหรับ ESP32
#define SDA 21
#define SCL 22

RTC_DS3231 rtc;

void setupRTC() {

    Wire.begin(SDA, SCL);
    
    if (!rtc.begin()) {
        Serial.println("RTC ไม่พบ!");
        Serial.println("ทำงานต่อไปแม้ RTC ไม่พบ");
    } else {

        if (rtc.lostPower()) {
            Serial.println("RTC มีการรีเซ็ต, ตั้งค่าเวลาใหม่...");
            rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
        }
    }
}

String getTime() {
    DateTime now = rtc.now();  // ดึงเวลา
    char buffer[30];
    sprintf(buffer, "%04d-%02d-%02d %02d:%02d:%02d", 
            now.year(), now.month(), now.day(), 
            now.hour(), now.minute(), now.second());
    return String(buffer);  // คืนค่าสตริงของเวลา
}

#endif
