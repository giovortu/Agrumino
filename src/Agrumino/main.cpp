

#include "main.h"

String m_id = "";

void goToSleep( const String& reason, int blink_times = 5 )
{
#ifdef DEBUG  
    Serial.println( reason );
#endif
    agrumino.turnBoardOff();
    delay(50);
    deepSleepSec(SLEEP_TIME_SEC);    
}

void receiveCallBackFunction(uint8_t *senderMac, uint8_t *incomingData, uint8_t len) 
{
  String rec = String( (char*) incomingData );
  goToSleep("Received:" +rec, SLEEP_TIME_SEC);
}

void sendCallBackFunction(u8 *mac_addr, u8 status) 
{
#ifdef DEBUG  
    Serial.println("Sent, waiting response...");
#endif
}


void setup() 
{ 
  agrumino.setup();  
  currentMillis = millis();

#ifdef USEGY21
  Wire.begin(SDA, SCL);
#endif
  delay(40);
  
  WiFi.mode(WIFI_STA);

#ifdef USE_MAC_AS_ID  
  m_id = WiFi.macAddress();
  m_id.replace(":", "" );
#else
  m_id = ESP.getChipId();
#endif 

#ifdef DEBUG  
  Serial.begin(115200);
#endif
  
  agrumino.turnBoardOn();

#ifdef DEBUG  
  Serial.println();
  Serial.println("ESP-Now Sender");
  Serial.printf("Transmitter mac: %s\n", WiFi.macAddress().c_str());
#endif

  if (esp_now_init() != 0) 
  {
  #ifdef DEBUG  
    Serial.println("ESP_Now init failed...");
  #endif
    delay(RETRY_INTERVAL);
    ESP.restart();
  }
  esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
  
  esp_now_register_recv_cb(receiveCallBackFunction);
  esp_now_register_send_cb(sendCallBackFunction);

#ifdef DEBUG  
  Serial.println("Slave ready. Waiting for messages...");
#endif
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
#ifdef USEGY21
  float hum = sensor.GY21_Humidity();
#else
  float hum = 0;
#endif 

#ifdef DEBUG  
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
#endif

  String message = getFullJsonString(m_id, temperature, soilMoisture, illuminance, hum, batteryVoltage, batteryLevel, isAttachedToUSB, isBatteryCharging );

  uint8_t len = message.length();

  uint8_t messageArray[ 200 ];
  memset(messageArray, '\0', 200);
  memccpy( messageArray, message.c_str(), '\n', len + 1 );

  esp_now_send(broadcastAddress, messageArray, len);

#ifdef DEBUG  
  Serial.print("Message sent :");
  Serial.println( message );
#endif
}

void loop() 
{
  if (millis() - currentMillis >= MAX_WAIT_RESPONSE_TIME) 
  {
#ifdef DEBUG      
    Serial.println("No response, sleep!");
#endif    
    deepSleepSec( SLEEP_TIME_SEC_NO_RESP );    
  }
}


// Returns the Json body that will be sent to the send data HTTP POST API
String getFullJsonString(String id, float temp, int soil, unsigned int lux, float hum, float batt, unsigned int battLevel, boolean usb, boolean charge) 
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
  //jsonBuffer["delta"] = SLEEP_TIME_MIN;

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

const String getChipId() 
{
  return m_id;
}
