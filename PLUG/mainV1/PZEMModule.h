#ifndef PZEMMODULE_H
#define PZEMMODULE_H

#include <PZEM004Tv30.h>

#define RXD2 16
#define TXD2 17

PZEM004Tv30 pzem(Serial2, TXD2, RXD2);

void setupPZEM() {
    Serial2.begin(9600, SERIAL_8N1, TXD2, RXD2);
}

String readPZEM() {
    float voltage = pzem.voltage();
    float current = pzem.current();
    float power = pzem.power();
    float energy = pzem.energy();

    String data = "Voltage: " + String(voltage) + " V\n" +
                  "Current: " + String(current) + " A\n" +
                  "Power: " + String(power) + " W\n" +
                  "Energy: " + String(energy) + " kWh";
    return data;
}

#endif
