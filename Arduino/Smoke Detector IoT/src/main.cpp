#include <Arduino.h>
#include <ArduinoJson.h>
#include <DHTesp.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
#include <TroykaMQ.h>
#include <AES.h>
#include <Cipher.h>

// PIN DEFINITIONS
const int GPIO4 = 2;  //DHT11 Temp & Humidity
const int GPIO3 = 5;  //Green LED
const int GPIO2 = 16; //Red LED
const int GPIO5 = 14; // beeper

//Cipher
byte key[16] = {0x48,0x65,0x6c,0x6c,0x6f,0x2c,0x20,0x57,0x6f,0x72,0x6c,0x64,0x21,0x21,0x21};
int msg_length = 0;
AES128 aes;

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
StaticJsonDocument<1500> jsonBuffer;
const size_t capacity = JSON_OBJECT_SIZE(1) + 2048;
DynamicJsonDocument jsonSensors(capacity);
DynamicJsonDocument jsonEncryptedSensors(capacity);
DynamicJsonDocument jsonDecryptedSensors(capacity);

//first init
WiFiClient espClient;
PubSubClient client(espClient);
DHTesp dht;
MQ2 mq2(A0);

// Functions definition
void callback(char* topic, byte* payload, unsigned int length);
void Get_sensors_val();
void Encrypt(BlockCipher *aes, String msg, byte output[], uint32_t *cycle_start, uint32_t *cycle_stop, long *time_start, long *time_stop);
void Decrypt(BlockCipher *aes, byte ciphertext[], byte plaintext[], int length, int aes_size, uint32_t *cycle_start, uint32_t *cycle_stop, long *time_start, long *time_stop);
void approximate_aes_size(int& length, int& aes_length);

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

      client.subscribe("wemos/encrypt_decrypt");

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
    String input_string;
    byte result_encrypt[200];
    String encrypted_input_json = "";
    String encrypted_input_text = "";
    uint32_t cycle_start, cycle_stop;
    long start_time, stop_time;
    int aes_size;

    jsonSensors.clear();
    jsonBuffer.clear();
    jsonEncryptedSensors.clear();
    encrypted_input_json.clear();

    // make json for sensors
    Get_sensors_val();
    serializeJson(jsonSensors, input_string);
    msg_length = input_string.length();

    // encrypt sensor
    approximate_aes_size(msg_length, aes_size);
    Encrypt(&aes, input_string, result_encrypt, &cycle_start, &cycle_stop, &start_time, &stop_time);

    for(int i = 0 ; i < aes_size ; i++){
      encrypted_input_text += String(result_encrypt[i], HEX);
      if (i < aes_size-1){
        encrypted_input_text += ";";
      }
    }

    // build JSON for sensors
    jsonEncryptedSensors["encrypted"] = encrypted_input_text;
    jsonEncryptedSensors["time"] = String(stop_time - start_time);
    jsonEncryptedSensors["length"] = msg_length;
    jsonEncryptedSensors["aes_size"] = aes_size;
    jsonEncryptedSensors["machine_id"] = 1;
    jsonEncryptedSensors["encryption_type"] = "aes-128";
    serializeJson(jsonEncryptedSensors, encrypted_input_json);

    // publish it to wemos/
    client.publish("wemos/encrypt", (char *)encrypted_input_json.c_str());

    // clear out the temporary variable for the next reading
    result_encrypt[0] = '\0';

    Serial.println();

    lastSendTH = millisNow;
  }

  client.loop();
}

// callback function from receive message action
void callback(char* topic, byte* payload, unsigned int length) {  
  String message = "";

// DEBUGGING =======================================
  Serial.print("RECEIVED :");
  for (int i = 0; i < length; i++) {
    message = message+(char)payload[i];
  }
  Serial.println();
// =================================================

  Serial.println(message);

  DeserializationError json_recv = deserializeJson(jsonBuffer,message);
  Serial.print(F("deserializeJson() failed: "));
  Serial.println(json_recv.c_str());

  if (json_recv){
    Serial.println("Parse failed");
  } else {
    //decryption goes here    
    byte encrypted[200];
    byte decrypted[200];
    int length = jsonBuffer["length"];
    int aes_size = jsonBuffer["aes_size"];
    uint32_t cycle_start, cycle_stop;
    long time_start, time_stop;
    String json_result;

    // cipher message
    for (int i = 0 ; i < aes_size ; i++){
      unsigned long received_ciphertext = strtoul( jsonBuffer["encrypted"][i], nullptr, 16);
      encrypted[i] = received_ciphertext & 0xFF;
    }

    Decrypt(&aes, encrypted, decrypted, length, aes_size, &cycle_start, &cycle_stop,&time_start, &time_stop);
    
    jsonDecryptedSensors["machine_id"] = jsonBuffer["machine_id"];
    jsonDecryptedSensors["encryption_type"] = jsonBuffer["encryption_type"];
    jsonDecryptedSensors["decrypted"] = decrypted;
    jsonDecryptedSensors["time"] = time_stop - time_start;
    jsonDecryptedSensors["cycle"] = cycle_stop - cycle_start;
    serializeJson(jsonDecryptedSensors, json_result);
    
    client.publish("wemos/decrypted", (char *)json_result.c_str());
  }

  message = "";
}

// get values from sensors and add them to array
void Get_sensors_val(){

  float temperature = dht.getTemperature();
  float humidity = dht.getHumidity();

  jsonSensors["temperature"] = temperature;
  jsonSensors["smoke"] = mq2.readSmoke();
  jsonSensors["humidity"] = humidity;
}

void approximate_aes_size(int& length, int& aes_length){
  if (length > 16){
      aes_length = 16 * (ceil(length / 16) + 1);
    } else {
      aes_length = 16;
    }
}

void Encrypt(BlockCipher *aes, String msg, byte output[], uint32_t *cycle_start, uint32_t *cycle_stop, long *time_start, long *time_stop){
  byte plaintext[200];
  byte *pointer_plaintext = plaintext;
  byte *pointer_output = output;
  int aes_size;

  msg.getBytes(plaintext, msg_length+1);
  approximate_aes_size(msg_length, aes_size);
  
  aes->setKey(key, aes->keySize());

  *time_start = micros();
  *cycle_start = ESP.getCycleCount();
  for (int i = 0 ; i < aes_size+1 ; i += 16){
    aes->encryptBlock(pointer_output + i, pointer_plaintext + i);
  }
  *cycle_stop = ESP.getCycleCount();
  *time_stop = micros();

  Serial.println();
}

void Decrypt(BlockCipher *aes, byte ciphertext[], byte plaintext[], int length, int aes_size, uint32_t *cycle_start, uint32_t *cycle_stop, long *time_start, long *time_stop){
  byte *pointer_plaintext = plaintext;
  byte *pointer_ciphertext = ciphertext;

  aes->setKey(key, aes->keySize());

  *time_start = micros();
  *cycle_start = ESP.getCycleCount();
  for (int i = 0 ; i < aes_size ; i += 16){
    aes->decryptBlock(pointer_plaintext + i, pointer_ciphertext + i);
  }
  *cycle_stop = ESP.getCycleCount();
  *time_stop = micros();

  Serial.println("DECRYPTED : ");
  for (int i = 0 ; i < length ; i++){
    Serial.print(char(plaintext[i]));
  }

  Serial.println();
}
