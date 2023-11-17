#ifndef AGRUMINO_H
#define AGRUMINO_H

#include <FS.h>     

#include <Agrumino.h>

#define TIMEZONE_GENERIC_VERSION_MIN_TARGET      "Timezone_Generic v1.10.1"
#define TIMEZONE_GENERIC_VERSION_MIN             1010001


#include <TimeLib.h>    

#include <ESP8266WiFi.h>        // https://github.com/esp8266/Arduino
#include <DNSServer.h>          // Installed from ESP8266 board

#include <WiFiManager.h>        // https://github.com/tzapu/WiFiManager
#include <ArduinoJson.h>        // https://github.com/bblanchon/ArduinoJson

#include <PubSubClient.h>
#include <WiFiUdp.h>
#include <NTPClient.h>

// Time to sleep in second between the readings/data sending
#define SLEEP_TIME_SEC 4200  
//IMPORTANTE: MASSIMO  4294 secondi !!! 

const char *MQTT_BROKER = "10.0.128.128";
//const char *MQTT_BROKER = "192.168.0.227";
const int MQTT_PORT = 1883;
long int lastSendDataMillis = 0;
#define SEND_DATA_EVERY_MS 1500

// Our super cool lib
Agrumino agrumino;

// Used for sending Json POST requests
StaticJsonDocument<200> jsonBuffer;

// Used to create TCP connections and make Http calls
WiFiClient client;
WiFiUDP ntpUDP;

NTPClient timeClient(ntpUDP);

PubSubClient mqtt;
bool canSleep = false;
bool shouldSaveConfig = false;

  
char currentDateTime[64];

int g_mqttConnectionTimeout = 0;
int g_wifiConnectionTimeout = 0;



const String getChipId();
void deepSleepSec(int sec);
bool checkIfResetWiFiSettings();
String getFullJsonString(float temp, int soil, unsigned int lux, float batt, unsigned int battLevel, boolean usb, boolean charge);
String getJsonString(float value) ;
String getJsonString(bool value) ;
String getJsonString(int value) ;
String getJsonString(String value);

String getFullJsonString(float temp, int soil, unsigned int lux, float batt, unsigned int battLevel, boolean usb, boolean charge) ;
boolean checkIfResetWiFiSettings() ;
void blinkLed(int duration, int blinks);
void delaySec(int sec);
void deepSleepSec(int sec);
const String getChipId();
boolean checkIfResetWiFiSettings();
void sendData();
void mqttCallback(const char *topic, byte *message, unsigned int length);
void getDateTime(time_t t, const char *tz, char *buf);
void publish( String _topic, String _payload );



#endif // AGRUMINO_H
