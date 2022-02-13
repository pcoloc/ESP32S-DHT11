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

#define uS_TO_S_FACTOR 1000000 /* Conversion factor for microseconds */
#define TIME_TO_SLEEP 120

#define DHTPIN 27     // Digital pin connected to the DHT sensor
// Uncomment the type of sensor in use:
#define DHTTYPE    DHT11     // DHT 11
//#define DHTTYPE    DHT22     // DHT 22 (AM2302)
//#define DHTTYPE    DHT21     // DHT 21 (AM2301)

// Replace with your network credentials
const char* ssid = "SSID_WIFI";
const char* password = "PASS_WIFI";
const char* mqttServer = "MQTT_SERVER_IP";
const int mqttPort = 1883;
const char* mqttUser = "MQTT_USER";
const char* mqttPassword = "MQTT_PASS";

const char* ntpserver = "es.pool.ntp.org";
const long  gmtOffset_sec = 0;
const int   daylightOffset_sec = 3600;

// Variables to save date and time
String formattedDate;
String dayStamp;
String timeStamp;

RTC_DATA_ATTR int bootCount = 0;

// Define NTP Client to get time

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

//MQTT begin
WiFiClient espClient;
PubSubClient client(espClient);

DHT dht(DHTPIN, DHTTYPE);
/* METHODS */
void print_wakeup_reason(){
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch(wakeup_reason)
  {
    /*case ESP_SLEEP_WAKEUP_EXT0 : Serial.println("Wakeup caused by external signal using RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1 : Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;*/
    case ESP_SLEEP_WAKEUP_TIMER : Serial.println("Wakeup caused by timer"); break;
   /* case ESP_SLEEP_WAKEUP_TOUCHPAD : Serial.println("Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP : Serial.println("Wakeup caused by ULP program"); break;
    default : Serial.printf("Wakeup was not caused by deep sleep: %d\n",wakeup_reason); break;*/
  }
}

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
    Serial.print("Temperature: ");
    Serial.print(t);
    Serial.println("ºC");
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
    Serial.print("Humidity: ");
    Serial.print(h);
    Serial.println("%");
    return h;
  }
}
/* VOID AND LOOP METHODS */
void setup(){
  // Serial port for debugging purposes
  Serial.begin(115200);
 
  //dht
  dht.begin();
  
  ++bootCount;
  Serial.println("Boot number: " + String(bootCount));
  print_wakeup_reason();
  
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  Serial.println("Setup ESP32 to sleep for every " + String(TIME_TO_SLEEP) +
  " Seconds");

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }
  Serial.println("Connected to Wifi. ;D ");

  // Print ESP32 Local IP Address
  Serial.println("My IP is: " + WiFi.localIP());
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
    while(!timeClient.update()) {
    timeClient.forceUpdate();
  }
  formattedDate = timeClient.getEpochTime();
  Serial.println( "Time: " + formattedDate + "ms");
  
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
 Serial.println("Going to sleep now");
  delay(1000);
  Serial.flush(); 
  
  esp_deep_sleep_start();
  Serial.println("This will never be printed");
}
 
void loop(){
  
}
