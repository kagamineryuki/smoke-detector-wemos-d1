#include <Arduino.h>
#include <ArduinoJson.h>
#include <DHTesp.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
#include <TroykaMQ.h>
#include <ChaCha.h>
#include <Cipher.h>
#include <Base64.h>

// PIN DEFINITIONS
const int GPIO4 = 4;  //DHT11 Temp & Humidity
const int GPIO3 = 5;  //Green LED
const int GPIO2 = 16; //Red LED
const int GPIO5 = 14; // beeper

//Cypher
byte key[16] = "Hello, World!!!";
size_t keySize = 16;
uint8_t rounds = 20; //ChaCha20
byte nonce[8] = {0,0,0,0,0,0,0,0};
byte counter[8] = {0,1,2,3,4,5,6,7};
int msg_length = 0;
ChaCha chacha;

//wifi credential
const char* ssid = "ArduinoUno";
const char* password = "qwerty123";

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
const size_t capacity = JSON_OBJECT_SIZE(1) + 2048;
DynamicJsonDocument jsonSensors(capacity);
DynamicJsonDocument jsonStatus(capacity);
DynamicJsonDocument jsonEncryptedSensors(capacity);
DynamicJsonDocument jsonEncryptedStatus(capacity);

//first init
WiFiClient espClient;
PubSubClient client(espClient);
DHTesp dht;
MQ2 mq2(A0);

// Functions definition
void callback(char* topic, byte* payload, unsigned int length);
void Get_sensors_val();
void Generate_nonce(ChaCha *chacha);
void Encrypt(ChaCha *chacha, char *msg);
void Decrypt(ChaCha *chacha, char *ciphertext);

void setup() {
// start the serial
  Serial.begin(115200);

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
  if(millisNow > lastSendTH + 3000){
    char input_string[1024];
    char status_string[1024];
    String encrypted_input_json = "";
    String encrypted_status_json = "";
    String encrypted_input_text = "";
    String encrypted_status_text = "";
    String buffer_nonce = "";

    jsonSensors.clear();
    jsonStatus.clear();
    jsonEncryptedSensors.clear();
    jsonEncryptedStatus.clear();
    encrypted_status_json.clear();
    encrypted_input_json.clear();
    jsonBuffer.clear();

    // make json for sensors
    Get_sensors_val();
    serializeJson(jsonSensors, input_string);
    serializeJson(jsonStatus, status_string);

    // encrypt sensor
    Generate_nonce(&chacha);
    Encrypt(&chacha, input_string);
    
    // nonce to hex nonce
    for(int i = 0; i < 8 ; i++){
      buffer_nonce += String(nonce[i], HEX);
      buffer_nonce += ";";
    }
    
    // encryp. to int
    for(int i = 0; i < msg_length ; i++){
      encrypted_input_text += String(input_string[i], HEX);
      encrypted_input_text += ";";
    }

    // build JSON for sensors
    jsonEncryptedSensors["nonce"] = buffer_nonce;
    jsonEncryptedSensors["encrypted"] = encrypted_input_text;
    serializeJson(jsonEncryptedSensors, encrypted_input_json);

    //encrypt status
    Generate_nonce(&chacha);
    Encrypt(&chacha, status_string);
    
    // nonce to hex nonce
    buffer_nonce = "";
    for(int i = 0; i < 8 ; i++){
      buffer_nonce += String(nonce[i], HEX);
      buffer_nonce += ";";
    }

    // encryp. to int
    for(int i = 0; i < msg_length ; i++){
      encrypted_status_text += String(status_string[i], HEX);
      encrypted_status_text += ";";
    }
    
    // build JSON for status
    jsonEncryptedStatus["nonce"] = buffer_nonce;
    jsonEncryptedStatus["encrypted"] = encrypted_status_text;
    serializeJson(jsonEncryptedStatus, encrypted_status_json);

    // publish it to wemos/
    if(client.publish("wemos/sensors", (char *)encrypted_input_json.c_str())){
      Serial.println("wemos/sensors | published");
      Serial.println("wemos/sensors | " + encrypted_input_json);
    } else {
      Serial.println("wemos/sensors | failed");
      Serial.println("wemos/sensors | " + encrypted_input_json);
    }

    if(client.publish("wemos/status", (char *)encrypted_status_json.c_str())){
      Serial.println("wemos/status | published");
      Serial.println("wemos/status | " + encrypted_status_json);
    } else {
      Serial.println("wemos/sensors | failed");
      Serial.println("wemos/status | " + encrypted_status_json);
    }

    // clear out the temporary variable for the next reading
    input_string[0] = '\0';
    status_string[0] = '\0';

    Serial.println();

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
  } else {
    if (jsonBuffer["beeper"]){
      digitalWrite(GPIO5, HIGH);
    } else {
      digitalWrite(GPIO5, LOW);
    }
  }

  message = "";
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
}

void Encrypt(ChaCha *chacha, char *msg){
  msg_length = strlen(msg);
  byte buffer[msg_length];
  byte plaintext[msg_length];
  
  chacha->setNumRounds(rounds);
  chacha->setKey(key, keySize);
  chacha->setIV(nonce, chacha->ivSize());
  chacha->setCounter(counter, 8);

  Serial.println();
  for (int i = 0 ; i < msg_length ; i++){
      plaintext[i] = msg[i];
  }

   for (int i = 0 ; i < msg_length ; i += msg_length) {
      int len = msg_length - i;
      if (len > msg_length)
          len = msg_length;
      chacha->encrypt(buffer + i, plaintext + i, msg_length);
   }

  for (int i = 0 ; i < msg_length ; i++){
      msg[i] = buffer[i];
  }

  Serial.println("CIPHERTEXT : ");

  for (int i = 0 ; i < msg_length ; i++){
    Serial.print(buffer[i], HEX);
    Serial.print(", ");
  }

  Serial.println();

}

void Decrypt(ChaCha *chacha, char *ciphertext){
  byte buffer[msg_length];
  byte plaintext[msg_length];
  String plain;

  chacha->setNumRounds(rounds);
  chacha->setKey(key, keySize);
  chacha->setIV(nonce, chacha->ivSize());
  chacha->setCounter(counter, 8);

  for(int i = 0 ; i < msg_length ; i++){
    plaintext[i] = ciphertext[i];
  }

  for (int i = 0 ; i < msg_length ; i += msg_length) {
      int len = msg_length - i;
      if (len > msg_length)
          len = msg_length;
      chacha->decrypt(buffer + i, plaintext + i, msg_length);
   }

  Serial.println("DECRYPTED : ");
  for(int i = 0 ; i < msg_length ; i++){
    ciphertext[i] = char(buffer[i]);
    Serial.print(char(buffer[i]));
  }
  
  Serial.println();
}
