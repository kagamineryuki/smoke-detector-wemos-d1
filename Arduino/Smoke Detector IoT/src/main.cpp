#include <Arduino.h>
#include <ArduinoJson.h>
#include <DHTesp.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
#include <TroykaMQ.h>
#include <ChaCha.h>
#include <Cipher.h>

//Cypher
byte key[16] = "Hello, World!!!";
size_t keySize = 16;
uint8_t rounds = 20; //ChaCha20
byte nonce[8] = {0,0,0,0,0,0,0,0};
byte counter[8] = {0,1,2,3,4,5,6,7};
int msg_length = 0;
ChaCha chacha;

//wifi credential
const char* ssid = "HN-03-1";
const char* password = "<Ir0Be><5D3v1#vW><A!v#N7@f4#L>";
IPAddress ip(192,168,30,3);
IPAddress gw(192,168,30,1);
IPAddress subnet(255,255,255,0);
IPAddress dns(192,168,30,1);

// CloudMQTT
const char* mqttServer = "maqiatto.com";
const int mqttPort = 1883;
const char* mqttUser = "kagamineryuki@gmail.com";
const char* mqttPassword = "kazekazekaze";

// variable
long millisNow = 0;
long lastSendMsg = 0;
long lastSendTH = 0;

// ArduinoJSON
StaticJsonDocument<1500> jsonBuffer;
const size_t capacity = JSON_OBJECT_SIZE(1) + 1024;
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
void Generate_nonce(ChaCha *chacha);
void Encrypt(ChaCha *chacha, char *msg, uint32_t& cycle_start, uint32_t& cycle_stop, ulong& time_start, ulong& time_stop);
void Decrypt(ChaCha *chacha, byte *ciphertext, byte *plaintext, int length, byte *nonce, byte *counter, uint32_t& cycle_start, uint32_t& cycle_stop, ulong& time_start, ulong& time_stop);

void setup() {
  char input_string[200];
  String encrypted_input_json = "";
  String encrypted_input_text = "";
  String buffer_nonce = "";
  String buffer_counter = "";
  ulong time_start, time_stop;
  uint32_t cycle_start, cycle_stop;

 //turn wifi off
  WiFi.mode( WIFI_OFF );
  WiFi.forceSleepBegin();
  delay( 1 );

// start the serial
  Serial.begin(115200);

// interrupt trigger init
  pinMode(D5, OUTPUT);
  pinMode(D5, LOW);

// initialize dht library
  dht.setup(D4, DHTesp::DHT11);

// cipher
  Generate_nonce(&chacha);

// init mq2
  mq2.calibrate();

// Connect to wifi
  WiFi.forceSleepWake();
  delay( 1 );

  WiFi.mode(WIFI_STA);
  WiFi.config(ip, gw, subnet, dns);
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

  Serial.println("Connecting to MQTT Server...");
  if (client.connect("client_1", mqttUser, mqttPassword)){ //trying to connect
    Serial.println("Connected to MQTT Server");
  } else {
    Serial.print("Can't connect to MQTT Server : ");
    Serial.println(client.state()); //print the fault code
    delay(200); //wait 200ms
  }

  jsonSensors.clear();
  jsonBuffer.clear();
  jsonEncryptedSensors.clear();
  encrypted_input_json.clear();

  // make json for sensors
  Get_sensors_val();
  serializeJson(jsonSensors, input_string);

  // encrypt sensor
  Generate_nonce(&chacha);
  Encrypt(&chacha, input_string, cycle_start, cycle_stop, time_start, time_stop);
  
  // nonce to hex nonce
  for(int i = 0; i < 8 ; i++){
    buffer_nonce += String(nonce[i], HEX);
    if(i < 7){
      buffer_nonce += ";";
    }
  }
  buffer_nonce.toUpperCase();
  
  // encryp. to int
  for(int i = 0; i < msg_length ; i++){
    encrypted_input_text += String(input_string[i], HEX);
    if(i < msg_length-1){
      encrypted_input_text += ";";
    }
  }
  encrypted_input_text.toUpperCase();

  // counter to byte
  for(int i = 0; i < 8 ; i++){
    buffer_counter += String(counter[i], HEX); 
    if(i < 7){
      buffer_counter += ";";
    }
  }
  buffer_counter.toUpperCase();

  // build JSON for sensors
  jsonEncryptedSensors["nonce"] = buffer_nonce;
  jsonEncryptedSensors["encrypted"] = encrypted_input_text;
  jsonEncryptedSensors["length"] = msg_length;
  jsonEncryptedSensors["counter"] = buffer_counter;
  jsonEncryptedSensors["time"] = time_stop - time_start;
  jsonEncryptedSensors["cycle"] = cycle_stop - cycle_start;
  serializeJson(jsonEncryptedSensors, encrypted_input_json);

  // publish it to wemos/
  client.publish("kagamineryuki@gmail.com/chacha20/encrypt", (char *)encrypted_input_json.c_str());

  // clear out the temporary variable for the next reading
  input_string[0] = '\0';

  Serial.println();

  for(int i = 0 ; i < 20 ; i++){
    client.loop();
    delay(100);
  }

  WiFi.disconnect();
  delay(1);
  Serial.println("Sleep for 30 secs");
  ESP.deepSleep(15e6, WAKE_RF_DISABLED);
}

void loop() {
}

// callback function from receive message action
void callback(char* topic, byte* payload, unsigned int length) {
  if(strcmp(topic, "kagamineryuki@gmail.com/chacha20/encrypt_decrypt") == 0){
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
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(json_recv.c_str());

      if (!json_recv){
        //decryption goes here    
        byte nonce[8];
        byte counter[8];
        byte encrypted[200];
        byte plaintext[200];
        String decrypted_string;
        String json_result;
        int length = jsonBuffer["length"];
        ulong time_start, time_stop;
        uint32_t cycle_start, cycle_stop;

        // cipher message
        for (int i = 0 ; i < msg_length ; i++){
          unsigned long received_ciphertext = strtoul( jsonBuffer["encrypted"][i], nullptr, 16);
          encrypted[i] = received_ciphertext & 0xFF;
        }

        // counter & nonce
        for (int i = 0 ; i < 8 ; i++){
          unsigned long received_counter = strtoul( jsonBuffer["counter"][i], nullptr, 16);
          unsigned long received_nonce = strtoul( jsonBuffer["nonce"][i], nullptr, 16);
          counter[i] = received_counter & 0xFF;
          nonce[i] = received_nonce & 0xFF;
        }

        Decrypt(&chacha, encrypted, plaintext, length, nonce, counter, cycle_start, cycle_stop, time_start, time_stop);
        Serial.println("DECRYPTED : ");
        for(int i = 0 ; i < length ; i++){
          decrypted_string += String(char(plaintext[i]));
          Serial.print(char(plaintext[i]));
        }
  
        jsonDecryptedSensors.clear();
        jsonDecryptedSensors["time"] = time_stop - time_start;
        jsonDecryptedSensors["cycle"] = cycle_stop - cycle_start;
        jsonDecryptedSensors["machine_id"] = 1;
        jsonDecryptedSensors["encryption_type"] = "chacha20";
        jsonDecryptedSensors["length"] = length;
        jsonDecryptedSensors["decrypted"] = decrypted_string;
        serializeJson(jsonDecryptedSensors, json_result);
        client.publish("kagamineryuki@gmail.com/chacha20/decrypted", (char *)json_result.c_str());
        Serial.println();
      }

      message = "";
  }
}

// get values from sensors and add them to array
void Get_sensors_val(){

  float temperature = dht.getTemperature();
  float humidity = dht.getHumidity();

  jsonSensors["temperature"] = temperature;
  jsonSensors["smoke"] = mq2.readSmoke();
  jsonSensors["humidity"] = humidity;
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

void Encrypt(ChaCha *chacha, char *msg, uint32_t& cycle_start, uint32_t& cycle_stop, ulong& time_start, ulong& time_stop){
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

  digitalWrite(D5, HIGH);
  
  time_start = micros();
  cycle_start = ESP.getCycleCount();
  for (int i = 0 ; i < msg_length ; i += msg_length) {
    int len = msg_length - i;
    if (len > msg_length)
        len = msg_length;
    chacha->encrypt(buffer + i, plaintext + i, msg_length);
  }
  cycle_stop = ESP.getCycleCount();
  time_stop = micros();

  digitalWrite(D5, LOW);

  for (int i = 0 ; i < msg_length ; i++){
      msg[i] = buffer[i];
  }

}

void Decrypt(ChaCha *chacha, byte *ciphertext, byte *plaintext, int length, byte *nonce, byte *counter, uint32_t& cycle_start, uint32_t& cycle_stop, ulong& time_start, ulong& time_stop){

  chacha->setNumRounds(rounds);
  chacha->setKey(key, keySize);
  chacha->setIV(nonce, chacha->ivSize());
  chacha->setCounter(counter, 8);

  digitalWrite(D5, HIGH);

  time_start = micros();
  cycle_start = ESP.getCycleCount();
  for (int i = 0 ; i < length ; i += length) {
      int len = length - i;
      if (len > length)
          len = length;
      chacha->decrypt(plaintext + i, ciphertext + i, length);
  }
  cycle_stop = ESP.getCycleCount();
  time_stop = micros();
  
  digitalWrite(D5, LOW);
}
