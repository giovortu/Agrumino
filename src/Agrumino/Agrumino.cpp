

/*
  AgruminoIFTTTWithCaptiveWiSampleWhitDeepSleep.ino - Sample project for Agrumino board using the Agrumino library.
  Created by antonio.solinas@lifely.cc , based on sample by giuseppe.broccia@lifely.cc on October 2017.

  @see Agrumino.h for the documentation of the lib
*/

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

const String getChipId();

// Time to sleep in second between the readings/data sending
#define SLEEP_TIME_SEC 4200  
//IMPORTANTE: MASSIMO  4294 secondi !!! 

const char *MQTT_BROKER = "10.0.128.128";
//const char *MQTT_BROKER = "192.168.0.227";
const int MQTT_PORT = 1883;

String m_id = getChipId();

// Our super cool lib
Agrumino agrumino;

// Used for sending Json POST requests
StaticJsonDocument<200> jsonBuffer;

// Used to create TCP connections and make Http calls
WiFiClient client;
WiFiUDP ntpUDP;

NTPClient timeClient(ntpUDP);

void mqttCallback(const char *topic, byte *message, unsigned int length);
PubSubClient mqtt;
bool canSleep = false;
bool shouldSaveConfig = false;

String getFullJsonString(float temp, int soil, unsigned int lux, float batt, unsigned int battLevel, boolean usb, boolean charge) ;
boolean checkIfResetWiFiSettings() ;
void blinkLed(int duration, int blinks);
void delaySec(int sec);
void deepSleepSec(int sec);
const String getChipId();
boolean checkIfResetWiFiSettings();
void sendData();

String dweetThingName = "sensor_" + getChipId();
  
int g_mqttConnectionTimeout = 0;
int g_wifiConnectionTimeout = 0;

bool mqttConnect()
{  
  bool retval = false;

  mqtt.setBufferSize(4096);
  mqtt.setServer( MQTT_BROKER, MQTT_PORT  );
  mqtt.setCallback( mqttCallback );
  mqtt.setClient( client );
  
  if (!mqtt.connected())
  {
    g_mqttConnectionTimeout = millis();

    Serial.println( "Attempting MQTT connection...");

    while (!mqtt.connected() )
    {
      // we try for 30 seconds otherwise we give up

      if (  millis() - g_mqttConnectionTimeout > 30000 )
      {
        Serial.println( "Timeout connecting to MQTT" );
        break;        
      }

      if (mqtt.connect( m_id.c_str() ))
      {
          Serial.println( "MQTT connected!" );
          sendData();
          retval = true;
      }
      else
      {
        Serial.print( ".");
        delay(1000);
      }
      

    }
    Serial.println("\n");
  }
  else
  {
    retval = true;
  }
  
  return retval;
}

void goToSleep( const String& reason, int blink_times = 5 )
{
    Serial.println( reason );
    delay(3000);
    // Reset and try again, or maybe put it to deep sleep
    
    // Blink when the business is done for giving an Ack to the user
    blinkLed(500, blink_times);
    // Board off before delay/sleep to save battery :)
    agrumino.turnBoardOff();
    deepSleepSec(SLEEP_TIME_SEC);
}

void setup() 
{
 
  Serial.begin(115200);


  // Setup our super cool lib
  agrumino.setup();

  // Turn on the board to allow the usage of the Led
  agrumino.turnBoardOn();


  WiFiManager wifiManager;


  // If the S1 button is pressed for 5 seconds then reset the wifi saved settings.
  if (checkIfResetWiFiSettings())
   {
    wifiManager.resetSettings();
    Serial.println("Reset WiFi Settings Done!");
    // Blink led for confirmation :)
    blinkLed(100, 10);
  }

  // Set timeout until configuration portal gets turned off
  // useful to make it all retry or go to sleep in seconds
  wifiManager.setTimeout(180); // 3 minutes

  // Customize the web configuration web page
  wifiManager.setCustomHeadElement("<h1>Agrumino</h1>");

  // Fetches ssid and pass and tries to connect
  // If it does not connect it starts an access point with the specified name here
  // and goes into a blocking loop awaiting configuration
  
  String ssidAP = "Agrumino-AP-" + getChipId();

  if (!wifiManager.autoConnect(ssidAP.c_str()))
  {
    goToSleep( "Failed to connect to WiFi and hit timeout\n" , 10 );
  }

  // If you get here you have connected to the WiFi :D
  Serial.println("Connected to WiFi!\n");

  timeClient.begin();

  
  while (!timeClient.update() );

  if ( ! mqttConnect() )
  {
    goToSleep( "Failed to connect to MQTT broker\n", 15 );
  }

  float temperature =         agrumino.readTempC();
  unsigned int soilMoisture = agrumino.readSoil();
  float illuminance =         agrumino.readLux();
  float batteryVoltage =      agrumino.readBatteryVoltage();
  unsigned int batteryLevel = agrumino.readBatteryLevel();
  boolean isAttachedToUSB =   agrumino.isAttachedToUSB();
  boolean isBatteryCharging = agrumino.isBatteryCharging();

  Serial.println("epoch      :       " + String ( timeClient.getEpochTime() ));
  Serial.println("temperature:       " + String(temperature) + "°C");
  Serial.println("soilMoisture:      " + String(soilMoisture) + "%");
  Serial.println("illuminance :      " + String(illuminance) + " lux");
  Serial.println("batteryVoltage :   " + String(batteryVoltage) + " V");
  Serial.println("batteryLevel :     " + String(batteryLevel) + "%");
  Serial.println("isAttachedToUSB:   " + String(isAttachedToUSB));
  Serial.println("isBatteryCharging: " + String(isBatteryCharging));
  Serial.println();
  
}

void mqttCallback(const char *topic, byte *message, unsigned int length)
{

}

void printDateTime(time_t t, const char *tz, char *buf)
{
    sprintf(buf, "%.4d-%.2d-%.2dT%.2d:%.2d:%.2d%s", year(t),month(t),day(t),hour(t),minute(t),second(t), tz);
}

void sendData()
{
  mqtt.loop();
  float temperature =         agrumino.readTempC();
  unsigned int soilMoisture = agrumino.readSoil();
  
  float illuminance =         agrumino.readLux();
  float batteryVoltage =      agrumino.readBatteryVoltage();
  unsigned int batteryLevel = agrumino.readBatteryLevel();
  boolean isAttachedToUSB =   agrumino.isAttachedToUSB();
  boolean isBatteryCharging = agrumino.isBatteryCharging();

  
  String topic = String("/casa/sensors/") + dweetThingName;

  String full = getFullJsonString( temperature, soilMoisture, illuminance, batteryVoltage, batteryLevel, isAttachedToUSB, isBatteryCharging );
  
  mqtt.publish( topic.c_str() , full.c_str()  );

  String lum = "{\"unit\":\"lumen\",\"type\":\"luminosity\",\"value\":" + String( illuminance ) + "}";
  mqtt.publish("/ufficio28/acquario/sensors/luminosity", lum.c_str() );

  String temp = "{\"unit\":\"°C\",\"type\":\"temperature\",\"value\":" + String( temperature ) + "}";
  mqtt.publish("/ufficio28/acquario/sensors/temperature", temp.c_str() );

  String hum = "{\"unit\":\"%\",\"type\":\"humidity\",\"value\":" + String( soilMoisture ) + "}";
  mqtt.publish("/ufficio28/acquario/sensors/humidity", hum.c_str() );

  canSleep = true;
}

void loop() 
{

  mqtt.loop();


  Serial.println("###################################");
  Serial.println("### Your Device name is ###");
  Serial.println("###   --> " + dweetThingName + " <--  ###");
  Serial.println("###################################\n");
 


  // Blink when the business is done for giving an Ack to the user
  blinkLed(200, 1);

  if ( canSleep )
  {
    // Board off before delay/sleep to save battery :)
    agrumino.turnBoardOff();
  
    delaySec(10); // The ESP8266 stays powered, executes the loop repeatedly
    deepSleepSec(SLEEP_TIME_SEC); // ESP8266 enter in deepSleep and after the selected time starts back from setup() and then loop()
  }
}


// Returns the Json body that will be sent to the send data HTTP POST API
String getSendDataBodyJsonString(float temp, int soil, unsigned int lux, float batt, unsigned int battLevel, boolean usb, boolean charge)
{  
  String data = getFullJsonString( temp,  soil, lux, batt,  battLevel,  usb,  charge);

  jsonBuffer.clear();
  jsonBuffer["value1"] = data;

  String jsonPostString;
  serializeJson( jsonBuffer, jsonPostString);

  return jsonPostString;
 }


// Returns the Json body that will be sent to the send data HTTP POST API
String getFullJsonString(float temp, int soil, unsigned int lux, float batt, unsigned int battLevel, boolean usb, boolean charge) 
{
  
  unsigned long epoch = timeClient.getEpochTime();
  char buf[64];
  printDateTime(epoch, "UTC", buf);

  jsonBuffer.clear();
  jsonBuffer["timestamp"] =  buf;
  jsonBuffer["temperature"] = temp;
  jsonBuffer["soil"] =  soil;
  jsonBuffer["lux"]  = lux;
  jsonBuffer["battery_voltage"] = batt;
  jsonBuffer["battery_level"] = battLevel;
  jsonBuffer["charging"] = charge; 
  jsonBuffer["usb_connected"] =  usb;

  String jsonPostString;
  serializeJson( jsonBuffer, jsonPostString);

  return jsonPostString;
 }


/////////////////////
// Utility methods //
/////////////////////

void blinkLed(int duration, int blinks) {
  for (int i = 0; i < blinks; i++) {
    agrumino.turnLedOn();
    delay(duration);
    agrumino.turnLedOff();
    if (i < blinks) {
      delay(duration); // Avoid delay in the latest loop ;)
    }
  }
}

void delaySec(int sec) {
  delay (sec * 1000);
}

void deepSleepSec(int sec) {
  Serial.print("\nGoing to deepSleep for ");
  Serial.print(sec);
  Serial.println(" seconds... (ー。ー) zzz\n");
  ESP.deepSleep(sec * 1000000,WAKE_RF_DEFAULT); // microseconds
  delay(100);
}

const String getChipId() {
  // Returns the ESP Chip ID, Typical 7 digits
  return String(ESP.getChipId());
}

// If the Agrumino S1 button is pressed for 5 seconds then reset the wifi saved settings.
boolean checkIfResetWiFiSettings() {

  int remainingsLoops = (5 * 1000) / 100;

  Serial.print("Check if reset WiFi settings: ");

  while (agrumino.isButtonPressed() && remainingsLoops > 0) {
    delay(100);
    remainingsLoops--;
    Serial.print(".");
  }

  if (remainingsLoops == 0) {
    // Reset Wifi Settings
    Serial.println(" YES!");
    return true;
  } else {
    Serial.println(" NO");
    return false;
  }
}
