## AgriNow
This is a version of the firmware for the Lifely Agrumino Lemon using Esp-now for data transfer.

### Configuration
It is possible to modify the behaviour using defines.h

```
#define USEGY21
#define DEBUG
#define USE_MAC_AS_ID
```

- USEGY21 enables the I2C sensor
- DEBUG enables the serial port and the log
- USE_MAC_AS_ID enable the use of the MAC address instead of ESP the chip ID


#### Related projects
https://github.com/giovortu/esp_now_router
https://github.com/giovortu/AgriNowManager
https://github.com/giovortu/BTicinoEsp
https://github.com/giovortu/ESPReceiver

### Ref.
https://www.lifely.cc/it/
https://www.espressif.com/en/solutions/low-power-solutions/esp-now



