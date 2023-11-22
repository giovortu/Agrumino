

#include "main.h"

String m_id = getChipId();

void goToSleep( const String& reason, int blink_times = 5 )
{
    Serial.println( reason );
    //Serial.println( ESP.deepSleepMax() );
    // Blink when the business is done for giving an Ack to the user
    //blinkLed(500, blink_times);
    // Board off before delay/sleep to save battery :)
    agrumino.turnBoardOff();
    delay(100);
    deepSleepSec(SLEEP_TIME_SEC);
    
}

void receiveCallBackFunction(uint8_t *senderMac, uint8_t *incomingData, uint8_t len) 
{
  goToSleep("Received!", SLEEP_TIME_SEC);
}

void sendCallBackFunction(u8 *mac_addr, u8 status) 
{
  //Serial.println("Sent, waiting response...");
}

void setup() 
{ 
  currentMillis = millis();

  WiFi.mode(WIFI_STA);

  Serial.begin(115200);

  // Setup our super cool lib
  agrumino.setup();

  // Turn on the board to allow the usage of the Led
  agrumino.turnBoardOn();

  delay(100);

  Serial.println();
  Serial.println("ESP-Now Sender");
  Serial.printf("Transmitter mac: %s\n", WiFi.macAddress().c_str());

  if (esp_now_init() != 0) 
  {
    Serial.println("ESP_Now init failed...");
    delay(RETRY_INTERVAL);
    ESP.restart();
  }
  esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
  
  esp_now_register_recv_cb(receiveCallBackFunction);
  esp_now_register_send_cb(sendCallBackFunction);

  Serial.println("Slave ready. Waiting for messages...");

  esp_now_add_peer(broadcastAddress, ESP_NOW_ROLE_SLAVE, 1, NULL, 0);

  delay(150);

  sendData();

  
}


void sendData()
{
  float temperature =         agrumino.readTempC();
  unsigned int soilMoisture = agrumino.readSoil();  
  float illuminance =         agrumino.readLux();
  float batteryVoltage =      agrumino.readBatteryVoltage();
  unsigned int batteryLevel = agrumino.readBatteryLevel();
  boolean isAttachedToUSB =   agrumino.isAttachedToUSB();
  boolean isBatteryCharging = agrumino.isBatteryCharging();

  Serial.println("###################################");
  Serial.println("### Your Device name is ###");
  Serial.println("###   --> " + m_id + " <--  ###");
  Serial.println("###################################\n");

  Serial.println("temperature:       " + String(temperature) + "°C");
  Serial.println("soilMoisture:      " + String(soilMoisture) + "%");
  Serial.println("illuminance :      " + String(illuminance) + " lux");
  Serial.println("batteryVoltage :   " + String(batteryVoltage) + " V");
  Serial.println("batteryLevel :     " + String(batteryLevel) + "%");
  Serial.println("isAttachedToUSB:   " + String(isAttachedToUSB));
  Serial.println("isBatteryCharging: " + String(isBatteryCharging));
  Serial.println();

  //TODO: READ I2C SENSOR

  if ( soilMoisture < 50 )
  {
    //canSleep = false;
    isWatering = true;
    //agrumino.turnWateringOn();
    //return;
  }
  else
  {
    isWatering = false;
  }


  String message = getFullJsonString(m_id, temperature, soilMoisture, illuminance, batteryVoltage, batteryLevel, isAttachedToUSB, isBatteryCharging );

  uint8_t len = message.length();

  uint8_t messageArray[len + 1];
  memccpy( messageArray, message.c_str(), '\n', len + 1 );

  esp_now_send(broadcastAddress, messageArray, len);

  Serial.print("Message sent :");
  Serial.println( message );


}

void loop() 
{
  if (millis() - currentMillis >= MAX_WAIT_RESPONSE_TIME) 
  {
    Serial.println("No response, sleep!");
    ESP.deepSleep(0);
  }
}


// Returns the Json body that will be sent to the send data HTTP POST API
String getFullJsonString(String id, float temp, int soil, unsigned int lux, float batt, unsigned int battLevel, boolean usb, boolean charge) 
{
  
  jsonBuffer.clear();
  jsonBuffer["type"] = "agri";
  jsonBuffer["id"] = id;
  jsonBuffer["temp"] = temp;
  jsonBuffer["soil"] =  soil;
  jsonBuffer["lum"]  = lux;
  jsonBuffer["bv"] = batt;
  jsonBuffer["bl"] = battLevel;
  jsonBuffer["charge"] = charge; 
  jsonBuffer["usb"] =  usb;
  jsonBuffer["hum"] =  22;

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
