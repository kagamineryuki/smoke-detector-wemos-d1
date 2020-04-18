#include <Arduino.h>
#include <ArduinoJson.h>
#include <DHTesp.h>
#include <TroykaMQ.h>

// get values from sensors and add them to array
void Get_sensors_val(DynamicJsonDocument &jsonSensors, String &result, DHTesp &dht, MQ2 &mq2){

    jsonSensors.clear();
    
    float temperature = dht.getTemperature();
    float humidity = dht.getHumidity();

    // DHT11
    jsonSensors["temperature"] = temperature;
    jsonSensors["smoke"] = mq2.readSmoke();
    jsonSensors["humidity"] = humidity;

    serializeJson(jsonSensors, result);
}