#include <stdlib.h>
#include <HardwareSerial.h>
#include <string.h>

typedef struct nmeaData;

/**
* @brief tells gps module to sleep for "sleepTime" seconds.
* @param sleepTime how long in seconds the gps will sleep.
* @param serPort serial port gps module is conected to.
*/
void sleepGNSS(int sleepTime, HardwareSerial &serPort)
/**
* @brief reads GNSS data from gps module.
* @param data a struct of type "nmeaData", passed as &varName, data will be stored in the struct.
* @param serPort serial port gps module is conected to.
*/
void readGNSS(nmeaData *data, HardwareSerial &serPort)
/**
* @brief initilises gps module on passed serial port.
* @param serPort serial port gps module is conected to.
* @param RX_pin rx pin gps module is conected to.
* @param TX_pin tx pin gps module is conected to.
*/
void initGNSS(HardwareSerial &serPort, int RX_pin, int TX_pin)
