#include <Arduino.h>
#include <ArduinoJson.h>
#include <DHTesp.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
#include <TroykaMQ.h>
#include <ChaCha.h>
#include <Cipher.h>

// PIN DEFINITIONS
const int GPIO4 = 4;  //DHT11 Temp & Humidity
const int GPIO3 = 5;  //Green LED
const int GPIO2 = 16; //Red LED
const int GPIO5 = 14; // beeper

//Cypher
byte key[16] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
size_t keySize = 16;
uint8_t rounds = 20; //ChaCha20
byte nonce[8] = {0,0,0,0,0,0,0,0};
byte counter[8] = {0,1,2,3,4,5,6,7};
ChaCha chacha;

//wifi credential
const char* ssid = "HN-03";
const char* password = "<Ir0Be><5D3v1#vW><A!v#N7@f4#L>";

// CloudMQTT
const char* mqttServer = "m24.cloudmqtt.com";
const int mqttPort = 12376;
const char* mqttUser = "ikfemazr";
const char* mqttPassword = "bQklMK_j41al";

// variable
long millisNow = 0;
long lastSendMsg = 0;
long lastSendTH = 0;

// ArduinoJSON
StaticJsonDocument<200> jsonBuffer;
const size_t capacity = JSON_OBJECT_SIZE(1) + 200;
DynamicJsonDocument jsonSensors(capacity);
DynamicJsonDocument jsonStatus(capacity);

//first init
WiFiClient espClient;
PubSubClient client(espClient);
DHTesp dht;
MQ2 mq2(A0);

// Functions definition
void callback(char* topic, byte* payload, unsigned int length);
void Get_sensors_val();
void Generate_nonce(ChaCha *chacha);
void Encrypt(ChaCha *chacha, String msg);

void setup() {
// start the serial
  Serial.begin(230400);

// initialize LEDs pin
  pinMode(GPIO2, OUTPUT);
  pinMode(GPIO3, OUTPUT);
  digitalWrite(GPIO2, HIGH);

// initialize dht library
  dht.setup(GPIO4, DHTesp::DHT11);

// initialize beeper pin
  pinMode(GPIO5, OUTPUT);
  digitalWrite(GPIO5, HIGH);

// cipher
  Generate_nonce(&chacha);

// init mq2
  mq2.calibrate();

// Connect to wifi
  WiFi.begin(ssid, password);

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

  digitalWrite(GPIO5, LOW);
  delay(5000);
}

void loop() {
  // put your main code here, to run repeatedly:
  millisNow = millis();

  //Connect to the mqtt server
  while (!client.connected()){ //loop until connected to server !
    Serial.println("Connecting to MQTT Server...");

    if (client.connect("client_1", mqttUser, mqttPassword)){ //trying to connect

      client.subscribe("wemos/command");

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
    String status_string = "";

    // make json for sensors
    Get_sensors_val();
    serializeJson(jsonSensors, input_string);
    serializeJson(jsonStatus, status_string);

    // encrypt
    Generate_nonce(&chacha);
    Encrypt(&chacha, input_string);

    // publish it to wemos/sensors
    client.publish("wemos/sensors", (char*)input_string.c_str());
    client.publish("wemos/status", (char*)status_string.c_str());
    // Serial.println(input_string);
    // Serial.println(status_string);

    // clear out the temporary variable for the next reading
    input_string = "";

    lastSendTH = millisNow;
  }

  client.loop();
}

// callback function from receive message action
void callback(char* topic, byte* payload, unsigned int length) {
//   String message = "";

// // DEBUGGING =======================================
//   Serial.print("Message arrived in topic: ");
//   Serial.println(topic);
// // =================================================

//   Serial.print("Message:");
//   for (int i = 0; i < length; i++) {
//     message = message+(char)payload[i];
//   }

//   Serial.println(message);

//   DeserializationError json_recv = deserializeJson(jsonBuffer,message);

//   if (json_recv){
//     Serial.println("Parse failed");
//   } else {
//     if (jsonBuffer["beeper"]){
//       digitalWrite(GPIO5, HIGH);
//     } else {
//       digitalWrite(GPIO5, LOW);
//     }
//   }

//   message = "";
}

// get values from sensors and add them to array
void Get_sensors_val(){

  float temperature = dht.getTemperature();
  float humidity = dht.getHumidity();

// DHT11
  jsonSensors["temperature"] = temperature;
  jsonSensors["smoke"] = mq2.readSmoke();
  jsonSensors["humidity"] = humidity;
  jsonSensors["abs_humidity"] = dht.computeAbsoluteHumidity(temperature, humidity, false);
  jsonSensors["dew_point"] = dht.computeDewPoint(temperature, humidity, false);

  if(digitalRead(GPIO5)){
    jsonStatus["beeper"] = true;
  }  else {
    jsonStatus["beeper"] = false;
  }
}

void Generate_nonce(ChaCha *chacha){
  Serial.print("Nonce : {");

  for (int i = 0 ; i < 8 ; i++){
    nonce[i] = random(256);
    Serial.print(String(nonce[i]));
    Serial.print(", ");
  }
  Serial.print("}");
  Serial.println("");

  Serial.print("Counter : {");
  for (int i = 0 ; i < 8 ; i++){
    if (counter[i] == 255){
      counter[i] = 0;
    } else {
      counter[i] = counter[i] + 1;
    }

    Serial.print(String(counter[i]));
    Serial.print(", ");
  }
  Serial.print("}");
  Serial.println("");

  chacha->clear();
  chacha->setNumRounds(rounds);
  chacha->setKey(key, keySize);
  chacha->setIV(nonce, chacha->ivSize());
  chacha->setCounter(counter, 8);
}

void Encrypt(ChaCha *chacha, String msg){
  byte buffer[300];
  byte plaintext[300];

  for (size_t i = 0; i < msg.length(); i++) {
      int len = msg.length() - i;
      plaintext[i] = byte(msg.charAt(i));
      chacha->encrypt(buffer , plaintext , len);
      Serial.print(buffer[i], HEX);
      Serial.print(",");
  }

  Serial.println();
}
