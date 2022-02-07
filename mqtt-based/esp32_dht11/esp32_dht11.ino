// Import required libraries
#include "WiFi.h"
#include "ESPAsyncWebServer.h"
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "time.h"
#include <NTPClient.h>
#include <WiFiUdp.h>

// Replace with your network credentials
const char* ssid = "SSID";
const char* password = "PASS";
const char* mqttServer = "IP_MQTT";
const int mqttPort = "PORT_MQTT";
const char* mqttUser = "MQTT_USER";
const char* mqttPassword = "MQTT_PASSWORD";

const char* ntpserver = "es.pool.ntp.org";
const long  gmtOffset_sec = 0;
const int   daylightOffset_sec = 3600;

// Variables to save date and time
String formattedDate;
String dayStamp;
String timeStamp;

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

//MQTT begin
WiFiClient espClient;
PubSubClient client(espClient);
  
#define DHTPIN 27     // Digital pin connected to the DHT sensor

// Uncomment the type of sensor in use:
#define DHTTYPE    DHT11     // DHT 11
//#define DHTTYPE    DHT22     // DHT 22 (AM2302)
//#define DHTTYPE    DHT21     // DHT 21 (AM2301)

DHT dht(DHTPIN, DHTTYPE);

float readDHTTemperature() {
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();
  // Read temperature as Fahrenheit (isFahrenheit = true)
  //float t = dht.readTemperature(true);
  // Check if any reads failed and exit early (to try again).
  if (isnan(t)) {    
    Serial.println("Failed to read from DHT sensor!");
    return -1000;
  }
  else {
    Serial.println(t);
    return t;
  }
}

float readDHTHumidity() {
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float h = dht.readHumidity();
  if (isnan(h)) {
    Serial.println("Failed to read from DHT sensor!");
    return -1000;
  }
  else {
    Serial.println(h);
    return h;
  }
}

void setup(){
  // Serial port for debugging purposes
  Serial.begin(115200);
  
  //dht
  dht.begin();
  
  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }
  Serial.println("Connected to Wifi. ;D ");

  // Print ESP32 Local IP Address
  Serial.println(WiFi.localIP());
  // Initialize a NTPClient to get time
  timeClient.begin();
  timeClient.setTimeOffset(3600); // GMT +1 
  
  //Okay Lets go MQTT
  client.setServer(mqttServer, mqttPort);
  while (!client.connected()){
    Serial.println("Connecting to MQTT...");
    if(client.connect("ESP32Client", mqttUser, mqttPassword) ){
      Serial.println("Connected.");
    } 
    else {
      Serial.print("failed with state ");
      Serial.println(client.state());
      delay(2000);
    }
  }
}
 
void loop(){
   while(!timeClient.update()) {
    timeClient.forceUpdate();
  }
  formattedDate = timeClient.getEpochTime();
  Serial.println(formattedDate);
  
  //ArduinoJSON 6
  StaticJsonDocument<300> JSONdocument;
  JSONdocument["device"] = "ESP32-DHT";
  JSONdocument["Time"] = formattedDate;
  JSONdocument["sensorType"] = "Temperature/Humidity";
  JSONdocument["Medition"]["Temperature"] = readDHTTemperature();
  JSONdocument["Medition"]["Humidity"] = readDHTHumidity();
  char buffer[256];
  size_t json = serializeJson(JSONdocument, buffer);
 
  Serial.println("Sending message to MQTT topic..");
  
  if (client.publish("SensorTH", buffer, json) == true) {
    Serial.println("Success sending message");
  } else {
    Serial.println("Error sending message");
  }
 
  client.loop();
  Serial.println("-------------");
 
  delay(10000);
}
