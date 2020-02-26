#include <Arduino.h>
#include <ArduinoJson.h>
#include <DHTesp.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <TroykaMQ.h>

// PIN DEFINITIONS
const int GPIO4 = 4; //DHT11 Temp & Humidity
const int GPIO3 = 5; //Green LED
const int GPIO2 = 16;//Red LED

//wifi credential
const char* ssid = "INSERT SSID HERE";
const char* password = "WIFI PASSWORD HERE";
const char* fingerprint = "INSERT SERVER FINGERPRINT HERE";

// CloudMQTT
const char* mqttServer = "m24.cloudmqtt.com";
const int mqttPort = 22376;
const char* mqttUser = "CLOUD MQTT USERNAME";
const char* mqttPassword = "CLOUD MQTT PASSWORD";

// variable
long millisNow = 0;
long lastSendMsg = 0;
long lastSendTH = 0;

// ArduinoJSON
StaticJsonDocument<200> jsonBuffer;
const size_t capacity = JSON_OBJECT_SIZE(1) + 200;
DynamicJsonDocument jsonSensors(capacity);

//first init
WiFiClientSecure espClient;
PubSubClient client(espClient);
DHTesp dht;
MQ2 mq2(A0);

// Functions definition
void callback(char* topic, byte* payload, unsigned int length);
void Get_sensors_val();

void setup() {
// initialize LEDs pin
  pinMode(GPIO2, OUTPUT);
  pinMode(GPIO3, OUTPUT);

  digitalWrite(GPIO2, HIGH);

// initialize dht library
  dht.setup(GPIO4, DHTesp::DHT11);

// init mq2
  mq2.calibrate();

// start the serial
  Serial.begin(115200);

// Connect to wifi
  WiFi.begin(ssid, password);
  espClient.setFingerprint(fingerprint);

  while (WiFi.status() != WL_CONNECTED){
    delay(500); //delay 500ms before trying again
    Serial.println("Trying to connect to SSID");
  }

// print the wifi connection
  Serial.print ("IP : ");
  Serial.println(WiFi.localIP());
  Serial.print ("RSSI : ");
  Serial.println(WiFi.RSSI());

// connect to MQTT server
  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);
}

void loop() {
  // put your main code here, to run repeatedly:
  millisNow = millis();

  //Connect to the mqtt server
  while (!client.connected()){ //loop until connected to server !
    Serial.println("Connecting to MQTT Server...");
    
    if (client.connect("Wemos D1 Trial", mqttUser, mqttPassword)){ //trying to connect

      Serial.println("Connected to MQTT Server");
      digitalWrite(GPIO3, HIGH); // GREEN LED ON
      digitalWrite(GPIO2, LOW); // RED LED OFF
    } else {

      Serial.print("Can't connect to MQTT Server : ");
      digitalWrite(GPIO3, LOW); // GREEN LED OFF
      digitalWrite(GPIO2, HIGH); // RED LED ON
      Serial.println(client.state()); //print the fault code
      delay(200); //wait 200ms
    }
  }

// Send temperature and humidity
  if(millisNow > lastSendTH + 2000){
    String input_string = "";

    // make json for sensors
    Get_sensors_val();
    serializeJson(jsonSensors, input_string);

    // publish it to wemos/sensors
    client.publish("wemos/sensors", (char*)input_string.c_str());

    // clear out the temporary variable for the next reading
    input_string = "";

    lastSendTH = millisNow;
  }

  client.loop();
}

// callback function from receive message action
void callback(char* topic, byte* payload, unsigned int length) {
  String message = "";

// DEBUGGING =======================================
  Serial.print("Message arrived in topic: ");
  Serial.println(topic);
// =================================================

  Serial.print("Message:");
  for (int i = 0; i < length; i++) {
    message = message+(char)payload[i];
  }

  Serial.println(message);

  DeserializationError json_recv = deserializeJson(jsonBuffer,message);

  if (json_recv){
    Serial.println("Parse failed");
  }

  message = "";
}

// get values from sensors and add them to array
void Get_sensors_val(){
  float temperature = dht.getTemperature();

// DHT11
  jsonSensors["temperature"] = temperature;
  jsonSensors["smoke"] = mq2.readSmoke();
  // jsonDHT["humidity"] = humidity;
  // jsonDHT["abs_humidity"] = dht.computeAbsoluteHumidity(temperature, humidity, false);
  // jsonDHT["dew_point"] = dht.computeDewPoint(temperature, humidity, false);
}
