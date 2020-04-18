#ifndef GET_SENSORS
#define GET_SENSORS
#include <Arduino.h>
#include <ArduinoJson.h>
#include <DHTesp.h>
#include <TroykaMQ.h>

void Get_sensors_val(DynamicJsonDocument &jsonSensors, String &result, DHTesp &dht, MQ2 &mq2);

#endif