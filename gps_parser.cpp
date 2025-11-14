#include "gps_parser.h"

#define GNSSSTARTCMD "$PCAS03,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*03\r\n"

typedef struct {
  bool vld;
  char latDir, lonDir;
  int nrSat;
  float lat, lon, utc, horPosAck;
} nmeaData;

void readGGAData(char *inputData, nmeaData *data) {
  char *buff;
  //GGA protocol header
  buff = strtok(inputData, ",*");
  //UTC time hhmmss.sss
  buff = strtok(NULL, ",");
  data->utc = atof(buff);
  //Latitude ddmm.mmmm
  buff = strtok(NULL, ",");
  data->lat = atof(buff);
  //N/S indication N=North, S=South
  buff = strtok(NULL, ",");
  data->latDir = *buff;
  //Longitude dddmm.mmmm
  buff = strtok(NULL, ",");
  data->lon = atof(buff);
  //E/W indication E=East, W=West
  buff = strtok(NULL, ",");
  data->lonDir = *buff;
  //Positioning 0: not positioned 1: valid Position
  buff = strtok(NULL, ",");
  if (*buff >= '1') {
    data->vld = 1;
  }else {
    data->vld = 0;
  }
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
  int out = 0;
  if (in <= '9') {
    out = in - '0'; //"0" - '0' = 0x00
  }else {
    out = in - 'A' + 10; //0x0A = 10, "A" - 'A' + 10 = 0x0A
  }
  return out;
}

int calcChkSum(char *head) {
  int count = 0, hash = 0;
  while ((*head != '*') && (*head != '\0')) {//chunk end is a *
    count++;
    if (128 < count) {
      return -2;
    }
    hash ^= *head; //bitwise xor
    head++;
  }
  return hash;
}

int verifyChkSum(char *inputData) {
  char *head = inputData;
  int hash = 0, chkSum;
  calcChkSum(head);
  head++;  // point to first of two chars in chksum
  chkSum = (charToHex(*head) * 16);
  head++;
  chkSum += charToHex(*head);

  if (hash == chkSum) {
    return 1;
  }
  return 0;
}

int parseGNSSData(char *inputData, nmeaData *data) {
  if (verifyChkSum(inputData) == 1) {
    readGGAData(inputData, data);
    if (data->vld == 1) {
      return 1;
    }
  }
  if (testMode == 1) {
    Serial.println(inputData);
  }
  return 0;
}

void sleepGNSS(int sleepTime, HardwareSerial &serPort) {
  char cmd[16], hex[2];
  int chkSum;
  snprintf(cmd, 18, "PCAS12,%d*", sleepTime);
  chkSum = calcChkSum(cmd);
  snprintf(hex, 4, "%X", chkSum);
  serPort.print("$");
  serPort.print(cmd);
  serPort.print(hex);
  serPort.print("\r\n");
}

void readGNSS(nmeaData *data, HardwareSerial &serPort) {
  char inbuf[128];
  int inpos = 0;
  bool dataReceved = 0;
  data->vld = 0;
  while (dataReceved == 0) {
    if (serPort.available() > 0) {
      int inByte = serPort.read();
      inbuf[inpos++] = inByte;
      if (inByte == '$') {
        inpos = 0;
      }
      if (inByte == '\n') {
        inbuf[inpos++] = 0;
        dataReceved = parseGNSSData(inbuf, data);
      }
    }
  }
}

void initGNSS(HardwareSerial &serPort) {
  serPort.begin(9600);
  while (!serPort) {}
  serPort.print(GNSSSTARTCMD);
}