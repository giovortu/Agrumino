

/*
  AgruminoIFTTTWithCaptiveWiSampleWhitDeepSleep.ino - Sample project for Agrumino board using the Agrumino library.
  Created by antonio.solinas@lifely.cc , based on sample by giuseppe.broccia@lifely.cc on October 2017.

  @see Agrumino.h for the documentation of the lib
*/

#include "main.h"

String m_id = getChipId();
String dweetThingName = "sensor_" + m_id;



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
    //Serial.println( ESP.deepSleepMax() );
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

  epoch = timeClient.getEpochTime();
  
  getDateTime(epoch, "UTC", currentDateTime);

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

  Serial.println("###################################");
  Serial.println("### Your Device name is ###");
  Serial.println("###   --> " + dweetThingName + " <--  ###");
  Serial.println("###################################\n");
  Serial.println("timestamp  :       " + String ( currentDateTime ));
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

void getDateTime(time_t t, const char *tz, char *buf)
{
    sprintf(buf, "%.4d-%.2d-%.2dT%.2d:%.2d:%.2d%s", year(t),month(t),day(t),hour(t),minute(t),second(t), tz);
}

void publish( String _topic, String _payload )
{
  mqtt.publish( _topic.c_str() , _payload.c_str()  );
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

  String st = "";
  
  st =getJsonString( temperature );
  publish( topic + "/temperature" , st  );

    st = getJsonString( (int)soilMoisture );
  publish( topic + "/soil_moisture" , st  );

  st = getJsonString( illuminance );
  publish( topic + "/luminosity" , st  );

  st = getJsonString( batteryVoltage );
  publish( topic + "/battery_voltage" , st  );

  st = getJsonString( (int)batteryLevel );
  publish( topic + "/battery_level" , st  );

  st = getJsonString( isAttachedToUSB  );
  publish( topic + "/is_attached_to_usb" , st  );

  st = getJsonString( isBatteryCharging );
  publish( topic + "/is_battery_charging" , st  );

  if ( soilMoisture < 50 )
  {
    //canSleep = false;
    isWatering = true;
    //agrumino.turnWateringOn();
    publish( topic + "/log" , "Watering ON"  );
    //return;
  }
  else
  {
    isWatering = false;
  }

  st = getJsonString( isWatering );
  publish( topic + "/is_watering" , st  );

#ifdef TEST
  String lum = "{\"unit\":\"lumen\",\"type\":\"luminosity\",\"value\":" + String( illuminance ) + "}";
  mqtt.publish("/ufficio28/acquario/sensors/luminosity", lum.c_str() );

  String temp = "{\"unit\":\"°C\",\"type\":\"temperature\",\"value\":" + String( temperature ) + "}";
  mqtt.publish("/ufficio28/acquario/sensors/temperature", temp.c_str() );

  String hum = "{\"unit\":\"%\",\"type\":\"humidity\",\"value\":" + String( soilMoisture ) + "}";
  mqtt.publish("/ufficio28/acquario/sensors/humidity", hum.c_str() );
#endif

  canSleep = true;

}

void loop() 
{
  mqtt.loop();

  // Blink when the business is done for giving an Ack to the user
  blinkLed(200, 1);

  if ( canSleep )
  {
    // Board off before delay/sleep to save battery :)
    agrumino.turnBoardOff();
  
    //delaySec(1); // The ESP8266 stays powered, executes the loop repeatedly
    goToSleep("Bye bye...", SLEEP_TIME_SEC); // ESP8266 enter in deepSleep and after the selected time starts back from setup() and then loop()
  }
  else
  {
    if ( millis() - lastSendDataMillis > SEND_DATA_EVERY_MS )
    {
      lastSendDataMillis = millis();
      sendData();
    }
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
  
  jsonBuffer.clear();
  jsonBuffer["timestamp"] =  String(currentDateTime);
  jsonBuffer["epoch"] =  epoch;
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

String getJsonString(float value ) 
{
  jsonBuffer.clear();
  jsonBuffer["timestamp"] =  String(currentDateTime);
  jsonBuffer["epoch"] =  epoch;
  jsonBuffer[ "value" ] = value;

  String jsonPostString;
  serializeJson( jsonBuffer, jsonPostString);

  return jsonPostString;
 }

String getJsonString(String value) 
{
  
  jsonBuffer.clear();
  jsonBuffer["timestamp"] =  String(currentDateTime);
  jsonBuffer["epoch"] =  epoch;
  jsonBuffer[ "value" ] = value;

  String jsonPostString;
  serializeJson( jsonBuffer, jsonPostString);

  return jsonPostString;
 }

String getJsonString(bool value) 
{
  
  jsonBuffer.clear();
  jsonBuffer["timestamp"] =  String(currentDateTime);
  jsonBuffer["epoch"] =  epoch;
  jsonBuffer[ "value" ] = value;

  String jsonPostString;
  serializeJson( jsonBuffer, jsonPostString);

  return jsonPostString;
 }

String getJsonString(int value) 
{
  
  jsonBuffer.clear();
  jsonBuffer["timestamp"] = String(currentDateTime);
  jsonBuffer["epoch"] =  epoch;
  jsonBuffer[ "value" ] = value;

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

void deepSleepSec(uint64_t sec) {
  Serial.print("\nGoing to deepSleep for ");
  Serial.print(sec);
  Serial.println(" seconds... (ー.ー) zzz\n");
  ESP.deepSleep(sec * 1000000,WAKE_RF_DEFAULT); // microseconds

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
