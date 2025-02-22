#ifndef RTCMODULE_H
#define RTCMODULE_H

#include <Wire.h>
#include <RTClib.h>

// กำหนด SDA และ SCL สำหรับ ESP32
#define SDA 21
#define SCL 22

RTC_DS3231 rtc;

void setupRTC() {
    // เริ่มต้นการสื่อสาร I2C ด้วยการกำหนด SDA และ SCL
    Wire.begin(SDA, SCL);
    
    // ตรวจสอบว่า RTC เชื่อมต่อได้หรือไม่
    if (!rtc.begin()) {
        Serial.println("RTC ไม่พบ!");
        Serial.println("ทำงานต่อไปแม้ RTC ไม่พบ");
    } else {
        // หาก RTC มีการสูญเสียพลังงาน รีเซ็ตเวลา
        if (rtc.lostPower()) {
            Serial.println("RTC มีการรีเซ็ต, ตั้งค่าเวลาใหม่...");
            rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));  // ตั้งค่าเวลาให้ตรงกับเวลาในคอมพิวเตอร์
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
