#include <Arduino.h>
#include <ArduinoJson.h>
#include <DHTesp.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <TroykaMQ.h>

// PIN DEFINITIONS
const int GPIO2 = 16;//Relay 1
const int GPIO3 = 5; //Relay 2
const int GPIO4 = 4; //DHT11 Temp & Humidity

//wifi credential
const char* ssid = "Kaze's Hotspot";
const char* password = "5D3v1#vW";
const char* fingerprint = "39:DF:B6:55:54:1D:A4:53:12:70:65:DC:43:11:53:DA:3B:A3:6E:A8";

// Personal MQTT
// const char* mqttServer = "kazeserver.ddns.net";
// const int mqttPort = 1883;
// const char* mqttUser = "kaze";
// const char* mqttPassword = "5D3v1#vW";

// CloudMQTT
const char* mqttServer = "m24.cloudmqtt.com";
const int mqttPort = 22376;
const char* mqttUser = "ikfemazr";
const char* mqttPassword = "1sZHdnmhMuRc";

// variable
long millisNow = 0;
long lastSendMsg = 0;
long lastSendTH = 0;
boolean LED = true;

// ArduinoJSON
StaticJsonDocument<200> jsonBuffer;
const size_t capacity = JSON_OBJECT_SIZE(1) + 200;
DynamicJsonDocument jsonRelay(capacity);
DynamicJsonDocument jsonDHT(capacity);
DynamicJsonDocument jsonMQ2(capacity);

//first init
WiFiClientSecure espClient;
PubSubClient client(espClient);
DHTesp dht;
MQ2 mq2(A0);

// Functions definition
void callback(char* topic, byte* payload, unsigned int length);
void Check_all_relays();
void Send_command_to_relays();
void Get_dht11_val();
void Get_mq2_val();

void setup() {
// Change pinMode for relays
  pinMode(GPIO2, OUTPUT);
  pinMode(GPIO3, OUTPUT);

// reset relay to all off
  digitalWrite(GPIO2, LOW);
  digitalWrite(GPIO3, LOW);

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
      client.subscribe("wemos/cmd"); //subscribe to wemos/cmd
    } else {
      Serial.print("Can't connect to MQTT Server : ");
      Serial.println(client.state()); //print the fault code
      delay(200); //wait 200ms
    }
  }

// Send status of all relay every 1 second
  if(millisNow > lastSendMsg + 1000){
    String relays_status = "";

    // check all relays
    Check_all_relays();

    // make json string
    serializeJson(jsonRelay, relays_status);

    // publish it to wemos/status
    client.publish("wemos/status", (char*)relays_status.c_str());
    relays_status = "";

    lastSendMsg = millisNow;
  }

// Send temperature and humidity
  if(millisNow > lastSendTH + 2000){
    String dht11_string = "";
    String mq2_string = "";


    // make json for temp and humidity
    Get_dht11_val();
    serializeJson(jsonDHT, dht11_string);
    Get_mq2_val();
    serializeJson(jsonMQ2, mq2_string);

    // publish it to wemos/sensors
    client.publish("wemos/sensors_dht", (char*)dht11_string.c_str());
    client.publish("wemos/sensors_mq2", (char*)mq2_string.c_str());

    // clear out the temporary variable for the next reading
    dht11_string = "";
    mq2_string = "";

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

  Send_command_to_relays();

  message = "";
}

// Check all relays status
void Check_all_relays(){
   // Check relay no.1 @ D2
  switch (digitalRead(GPIO2)){
    case 1:
      jsonRelay["relay_1_status"] = false;
      break;
    case 0:
      jsonRelay["relay_1_status"] = true;
      break;
  }

  // Check relay no.2 @ D3
 switch (digitalRead(GPIO3)){
   case 1:
     jsonRelay["relay_2_status"] = false;
     break;
   case 0:
     jsonRelay["relay_2_status"] = true;
     break;
 }

}

// do the commands from wemos/cmd
void Send_command_to_relays(){
  // Relay 1
  if (jsonBuffer["relay_1"]){
    digitalWrite(GPIO2, LOW);
  } else {
    digitalWrite(GPIO2, HIGH);
  }

  // Relay 2
  if (jsonBuffer["relay_2"]){
    digitalWrite(GPIO3, LOW);
  } else {
    digitalWrite(GPIO3, HIGH);
  }

}

// get values from sensors and add them to array
void Get_dht11_val(){
  float temperature = dht.getTemperature();
  float humidity = dht.getHumidity();

// DHT11
  jsonDHT["temperature"] = temperature;
  jsonDHT["humidity"] = humidity;
  jsonDHT["abs_humidity"] = dht.computeAbsoluteHumidity(temperature, humidity, false);
  jsonDHT["dew_point"] = dht.computeDewPoint(temperature, humidity, false);
}

void Get_mq2_val(){
  // MQ2
  jsonMQ2["smoke"] = mq2.readSmoke();
  jsonMQ2["lpg"] = mq2.readLPG();
  jsonMQ2["methane"] = mq2.readMethane();
  jsonMQ2["hydrogen"] = mq2.readHydrogen();
}
