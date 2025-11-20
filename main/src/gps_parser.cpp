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
  // 0–9 → 0–9
  if (in >= '0' && in <= '9') {
    return in - '0';
  }
  // A–F → 10–15
  if (in >= 'A' && in <= 'F') {
    return in - 'A' + 10;
  }
  // a–f → 10–15
  if (in >= 'a' && in <= 'f') {
    return in - 'a' + 10;
  }

  return -1; // invalid hex char
}

// XOR of characters between $ and * (not including them)
int calcChkSum(const char *head) {
  int hash = 0;
  int count = 0;

  while (*head != '*' && *head != '\0') {
    hash ^= (unsigned char)*head;
    head++;
    count++;

    if (count > 128) {
      return -2; // safety guard
    }
  }

  return hash;
}

int verifyChkSum(char *inputData) {
  // Find '$'
  char *start = strchr(inputData, '$');
  if (!start) return 0;
  start++; // skip '$'

  // Compute XOR
  int hash = calcChkSum(start);
  if (hash < 0) return 0;

  // Find '*'
  char *star = strchr(start, '*');
  if (!star) return 0;

  // Require exactly two hex digits after '*'
  if (!star[1] || !star[2]) return 0;

  int hi = charToHex(star[1]);
  int lo = charToHex(star[2]);
  if (hi < 0 || lo < 0) return 0;

  int chkSum = (hi << 4) | lo;

  if (chkSum == hash) {
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
  char buffer[256];
  int index = 0;
  bool startCMD = false;
  bool dataReceved = 0;
  const char GNGGA_CMD[] = "$GNGGA";
  data->vld = 0;
  uint32_t startMs = millis();
  const uint32_t timeoutMs = 6000; //6s timeout
  while (dataReceved == 0) {
    if ((millis() - startMs) > timeoutMs) {
      return;
    }
    if (serPort.available() > 0) {
      char c = serPort.read();
      if (c == '$') { //start of message
        index = 0;
        startCMD = true;
      }
      if (startCMD){
        buffer[index] = c;
        index++;
      }
      if (c == '\n' && startCMD) { //end of message
        buffer[index] = '\0';
        startCMD = false;

        bool isGNGGA = true;
        for (int i = 0; i < 6; i++) {       // 6 tegn: '$', 'G','N','G','G','A'
            if (buffer[i] != GNGGA_CMD[i]) {
                isGNGGA = false;
                break;                     // stop tidligt
            }
        }
        if (isGNGGA){
          dataReceved = parseGNSSData(buffer, data);
        }
      }
      if (index >= sizeof(buffer)){
        startCMD = false;
        index = 0;
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
  while (!serPort) {
    delay(100);
  } //waits until serial port has initialized
  serPort.flush();
  serPort.print(GNSSSTARTCMD);
}
