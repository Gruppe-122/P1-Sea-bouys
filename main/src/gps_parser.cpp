#include "gps_parser.h"

#define GNSSSTARTCMD "$PCAS03,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*03\r\n"

static const bool USE_DECIMAL_DEGREES = false;

double convertTodegrees(double raw) {

  int degrees = (int)(raw / 100);
  double minutes = raw - (degrees * 100);
  double decimal = degrees + (minutes / 60);

  return decimal;
}

void readGGAData(char *inputData, nmeaData *data) {
  char *buff;
  //GGA protocol header
  //strtok replaces the separator character with a string terminator
  //#THIS MODIFIES THE DATA DESTRUCTIVELY#
  buff = strtok(inputData, ",*");
  //UTC time hhmmss.sss
  buff = strtok(NULL, ",");
  data->utc = atof(buff);
  //Latitude ddmm.mmmm
  buff = strtok(NULL, ",");
  data->lat = USE_DECIMAL_DEGREES ? convertTodegrees(atof(buff)) : atof(buff);
  //N/S indication N=North, S=South
  buff = strtok(NULL, ",");
  data->latDir = *buff;
  //Longitude dddmm.mmmm
  buff = strtok(NULL, ",");
  data->lon = USE_DECIMAL_DEGREES ? convertTodegrees(atof(buff)) : atof(buff);
  //E/W indication E=East, W=West
  buff = strtok(NULL, ",");
  data->lonDir = *buff;
  //Positioning 0: not positioned 1: valid Position
  buff = strtok(NULL, ",");
  data->vld = (*buff >= '1') ? 1 : 0;
  //Number of satellites Range 0 to 12 (lies)
  buff = strtok(NULL, ",");
  data->nrSat = atoi(buff);
  //HDOP Horizontal accuracy
  buff = strtok(NULL, ",");
  data->horPosAck = atof(buff);
  //Mean Sea Level Earth is -2.2 M
  buff = strtok(NULL, ",");
  //Differential time When there is no DGPS, invalid
  buff = strtok(NULL, ",");
  //Differential ID
  buff = strtok(NULL, ",");
}

int charToHex(char in) {
  if (in <= '9' && in >= '0') {
    return (in - '0'); //"0" - '0' = 0x00
  }
  if (in <= 'A' && in >= 'F') {
    return (in - 'A' + 10); //0x0A = 10, "A" - 'A' + 10 = 0x0A
  }

  return -1; // invalid hex char
}

int calcChkSum(char *head) {
  int count = 0, hash = 0;
  while ((*head != '*') && (*head != '\0')) { //chunk end is a *
    count++;
    if (count > 128) {
      return -2; //out of bounds
    }
    hash ^= (unsigned char)*head; //bitwise xor
    head++;
  }
  return hash;
}

int verifyChkSum(char *inputData) {
  char *head = inputData;
  int readPos = 0;
  int hash = calcChkSum(head);
  if (hash < 0) {
    return 0; //if error [panik]
  }
  while ((*head != '*') && (readPos < 128)) {
    head++;
    readPos++;
  }
  head++; //point to first of two chars in chksum
  int chkSum = (charToHex(*head) << 4);
  head++;
  chkSum += charToHex(*head);

  if (hash == chkSum) {
    return 1;
  }
  return 0;
}

int parseGNSSData(char *inputData, nmeaData *data) {
  if (verifyChkSum(inputData) == 1) {
    readGGAData(inputData, data); //shreds the string it is passed (if you need it afterwards give it a copy)
    if (data->vld == 1) {
      return 1;
    }
  }
  return 0;
}

void sleepGNSS(int sleepTime, HardwareSerial &serPort) {
  char cmd[24], hex[4]; //checksum is 2 chars of hex
  int chkSum;
  snprintf(cmd, 24, "PCAS12,%d*", sleepTime);
  chkSum = calcChkSum(cmd);
  snprintf(hex, 4, "%X", chkSum);
  serPort.print("$"); //sends sleep command
  serPort.print(cmd);
  serPort.print(hex);
  serPort.print("\r\n");
}

void readGNSS(nmeaData *data, HardwareSerial &serPort) {
  char inbuf[128];
  int inpos = 0;
  bool dataReceved = 0;
  const char GNGGA[] = "GNGGA";
  data->vld = 0;
  uint32_t startMs = millis();
  const uint32_t timeoutMs = 6000; //6s timeout
  while (dataReceved == 0) {
    if ((millis() - startMs) > timeoutMs) {
      return;
    }
    if (serPort.available() > 0) {
      int inByte = serPort.read();
      inbuf[inpos++] = inByte;
      if (inByte == '$') { //start of message
        inpos = 0;
      }
      if (inByte == '\n') { //end of message
        inbuf[inpos++] = 0;
        bool isGGA = true;
        for (int i = 0; i < 5; i++) { //5 chars: 'G','N','G','G','A'
          if (inbuf[i] != GNGGA[i]) {
            isGGA = false;
            break;                     // stop tidligt
          }
        }
        if (isGGA == true) {
          dataReceved = parseGNSSData(inbuf, data);
        }else {
          inpos = 0;
        }
      }
      if (inpos >= 127) {
        inpos = 0;
      }
    }
  }
}

void PrintGPSData(nmeaData &GNSSData){
  Serial.print("lat: ");
  Serial.print(GNSSData.lat, sizeof(double));
  Serial.println(GNSSData.latDir);
  Serial.print("lon: ");
  Serial.print(GNSSData.lon, sizeof(double));
  Serial.println(GNSSData.lonDir);
  Serial.print("UTC: ");
  Serial.println(GNSSData.utc);
  Serial.print("nrSat: ");
  Serial.println(GNSSData.nrSat);
  Serial.print("vld: ");
  Serial.println(GNSSData.vld);
}

void initGNSS(HardwareSerial &serPort, int RX_pin, int TX_pin) { 
  serPort.begin(9600, SERIAL_8N1, RX_pin, TX_pin);
  while (!serPort) {} //waits until serial port has initialized
  serPort.flush();
  serPort.print(GNSSSTARTCMD);
}
