#pragma once

#include <Arduino.h>

#include "Class/LineParsing.h"
#include "Class/SensorConverting.h"
#include "Global.h"

class SensorAnalog : public LineParsing public SensorConverting {
   public:
    SensorAnalog() : LineParsing(){};

    void SensorAnalogInit() {
        //if (_pin != "") {
        //    pinMode(_pin.toInt(), INPUT);
        //}
    }

    int SensorAnalogRead(String key, String pin) {
        int pinInt = pin.toInt();
        int value;
#ifdef ESP32
        value = analogRead(pinInt);
#endif
#ifdef ESP8266
        pinInt = pinInt;
        value = analogRead(A0);
#endif

        eventGen(key, "");
        jsonWriteInt(configLiveJson, key, value);
        MqttClient::publishStatus(key, String(value));

        Serial.println("[I] sensor '" + key + "' data: " + String(value));
        return value;
    }
};
extern SensorAnalog* mySensorAnalog;