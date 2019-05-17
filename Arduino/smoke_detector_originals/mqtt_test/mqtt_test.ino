#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

const int D2 = 16;

//wifi credential
const char* ssid = "HN-03";
const char* password = "5D3v1#vW";

//mqtt
const char* mqttServer = "kazeserver.ddns.net";
const int mqttPort = 1883;
const char* mqttUser = "kaze";
const char* mqttPassword = "5D3v1#vW";

// variable
long millisNow = 0;
long lastSendMsg = 0;
boolean LED = true;
const size_t capacity = JSON_OBJECT_SIZE(1) + 50;
StaticJsonDocument<200> jsonBuffer;
DynamicJsonDocument jsonSerial(capacity);

//first init
WiFiClient espClient;
PubSubClient client(espClient);

void setup() {

  pinMode(D2, OUTPUT);

  Serial.begin(115200);

  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED){
    delay(500); //delay 500ms before trying again
    Serial.println("Trying to connect to SSID");
  }

  Serial.print ("IP : ");
  Serial.println(WiFi.localIP());
  Serial.print ("RSSI : ");
  Serial.println(WiFi.RSSI());

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
      client.subscribe("wemos/cmd");
    } else {
      Serial.print("Can't connect to MQTT Server : ");
      Serial.println(client.state()); //print the fault code
      delay(200); //wait 200ms
    }
  }

  if(millisNow > lastSendMsg + 1000){
    String relay_1_status = "";
    
    if(digitalRead(D2) == 1){
      jsonSerial["relay_1_status"] = true;
    } else {
      jsonSerial["relay_1_status"] = false;
    }

    serializeJson(jsonSerial, relay_1_status);
    
    client.publish("wemos/status", (char*)relay_1_status.c_str());
    relay_1_status = "";

    lastSendMsg = millisNow;
  }
  
  client.loop();
}

void callback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  
  Serial.print("Message arrived in topic: ");
  Serial.println(topic);
 
  Serial.print("Message:");
  for (int i = 0; i < length; i++) {
//    Serial.print((char)payload[i]);
    message = message+(char)payload[i];
  }

  Serial.println(message);
  
  DeserializationError json_recv = deserializeJson(jsonBuffer,message);

  if (json_recv){
    Serial.println("Parse failed");
  }

  if(jsonBuffer["relay_1"] == true){ 
    digitalWrite(D2, HIGH);
  } else if (jsonBuffer["relay_1"] == false) {
    digitalWrite(D2, LOW);
  }
 
  message = "";
}
