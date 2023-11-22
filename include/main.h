#ifndef AGRUMINO_H
#define AGRUMINO_H

#include <FS.h>     

#include <Agrumino.h>

#define TIMEZONE_GENERIC_VERSION_MIN_TARGET      "Timezone_Generic v1.10.1"
#define TIMEZONE_GENERIC_VERSION_MIN             1010001


#include <TimeLib.h>    
#include <ESP8266WiFi.h>
#include <espnow.h>

#include <ArduinoJson.h>        // https://github.com/bblanchon/ArduinoJson

// Time to sleep in second between the readings/data sending
#define SLEEP_TIME_SEC (60*20)
//IMPORTANTE: MASSIMO  4294 secondi !!! 
//                      14835777529 max
//                       4294000000

#define RETRY_INTERVAL 50

static uint8_t broadcastAddress[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

long currentMillis = millis();
#define MAX_WAIT_RESPONSE_TIME 10000

Agrumino agrumino;

StaticJsonDocument<200> jsonBuffer;

bool canSleep = false;

bool isWatering = false;

const String getChipId();
void deepSleepSec(uint64_t sec);
void receiveCallBackFunction(uint8_t *senderMac, uint8_t *incomingData, uint8_t len);
void sendCallBackFunction(u8 *mac_addr, u8 status);

String getFullJsonString(String id, float temp, int soil, unsigned int lux, float batt, unsigned int battLevel, boolean usb, boolean charge);

void blinkLed(int duration, int blinks);
void delaySec(int sec);

const String getChipId();
void sendData();



#endif // AGRUMINO_H
