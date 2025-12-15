#include "gps_parser.h"

#define GNSSSTARTCMD "$PCAS03,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*03\r\n"

static const bool USE_DECIMAL_DEGREES = true;

double convertTodegrees(double raw) {
  gpsLog.logln("convert lat or lon to degrees decimal", "INFO", true);

  int degrees = (int)(raw / 100);
  double minutes = raw - (degrees * 100);
  double decimal = degrees + (minutes / 60);

  return decimal;
}

void syncTimeFromGPS(const char *rawTime)
{
  gpsLog.log("rawtime : ", "DEBUG", true);
  gpsLog.logln(rawTime, "DEBUG", false);

  if (!rawTime) return;
  if (strlen(rawTime) < 6) return;  // need at least HHMMSS

  // Parse time: HHMMSS.sss (UTC)
  int hour = (rawTime[0] - '0') * 10 + (rawTime[1] - '0');
  int min  = (rawTime[2] - '0') * 10 + (rawTime[3] - '0');
  int sec  = (rawTime[4] - '0') * 10 + (rawTime[5] - '0');

  int ms = 0;
  if (rawTime[6] == '.' && rawTime[7] && rawTime[8] && rawTime[9]) {
      ms = (rawTime[7] - '0') * 100 +
            (rawTime[8] - '0') * 10 +
            (rawTime[9] - '0');
  }

  time_t now = time(nullptr);
  struct tm t {};
  gmtime_r(&now, &t);

  t.tm_hour = hour;
  t.tm_min  = min;
  t.tm_sec  = sec;

  time_t utc_epoch = mktime(&t);

  struct timeval tv;
  tv.tv_sec  = utc_epoch;
  tv.tv_usec = ms * 1000;

  int res = settimeofday(&tv, nullptr);
  if (res == 0) {
      gpsLog.logln("settimeofday OK", "DEBUG", true);
  } else {
      gpsLog.logln("settimeofday FAILED", "DEBUG", true);
  }

  gpsLog.log("hour: ", "DEBUG", true);
  gpsLog.logln(hour, "DEBUG", false);

  gpsLog.log("min: ", "DEBUG", true);
  gpsLog.logln(min, "DEBUG", false);

  gpsLog.log("sec: ", "DEBUG", true);
  gpsLog.logln(sec, "DEBUG", false);

  // Debug
  struct timeval getTv;
  gettimeofday(&getTv, nullptr);

  struct tm timeinfo;
  localtime_r(&tv.tv_sec, &timeinfo);

  char buffer[32];
  strftime(buffer, sizeof(buffer), "%H:%M:%S", &timeinfo);

  gpsLog.log("tid", "DEBUG", true);
  gpsLog.logln(buffer, "DEBUG", false);
}


void readGGAData(char *inputData, nmeaData *data) {
  gpsLog.logln("decode gps data (GGA DATA)", "INFO", true);
  char *buff;
  //GGA protocol header
  //strsep replaces the separator character with a string terminator
  //#THIS MODIFIES THE DATA DESTRUCTIVELY#
  buff = strsep(&inputData, ",*");
  if (buff == nullptr) return;
  //UTC time hhmmss.sss
  buff = strsep(&inputData, ",*");
  if (buff == nullptr) return;
  strncpy(data->utc, buff, sizeof(data->utc) - 1);
  data->utc[sizeof(data->utc) - 1] = '\0';
  //Latitude ddmm.mmmm
  buff = strsep(&inputData, ",*");
  if (buff == nullptr) return;
  data->lat = USE_DECIMAL_DEGREES ? convertTodegrees(atof(buff)) : atof(buff);
  //N/S indication N=North, S=South
  buff = strsep(&inputData, ",*");
  if (buff == nullptr) return;
  data->latDir = *buff;
  //Longitude dddmm.mmmm
  buff = strsep(&inputData, ",*");
  if (buff == nullptr) return;
  data->lon = USE_DECIMAL_DEGREES ? convertTodegrees(atof(buff)) : atof(buff);
  //E/W indication E=East, W=West
  buff = strsep(&inputData, ",*");
  if (buff == nullptr) return;
  data->lonDir = *buff;
  //Positioning 0: not positioned 1: valid Position
  buff = strsep(&inputData, ",*");
  if (buff == nullptr) return;
  data->vld = (*buff >= '1') ? 1 : 0;
  //Number of satellites Range 0 to 12 (lies)
  buff = strsep(&inputData, ",*");
  if (buff == nullptr) return;
  data->nrSat = atoi(buff);
  //HDOP Horizontal accuracy
  buff = strsep(&inputData, ",*");
  if (buff == nullptr) return;
  data->horPosAck = atof(buff);
  //Mean Sea Level Earth is -2.2 M
  buff = strsep(&inputData, ",*");
  if (buff == nullptr) return;
  //Differential time When there is no DGPS, invalid
  buff = strsep(&inputData, ",*");
  if (buff == nullptr) return;
  //Differential ID
  buff = strsep(&inputData, ",*");
  if (buff == nullptr) return;
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
  gpsLog.logln("calculates checksum", "INFO", true);
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
  gpsLog.logln("verify the checksum", "INFO", true);
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

  gpsLog.log(hash, "DEBUG", true);
  gpsLog.log(" == ", "DEBUG", false);
  gpsLog.logln(chkSum, "DEBUG", false);

  if (hash == chkSum) {
    return 1;
  }
  return 0;
}

int parseGNSSData(char *inputData, nmeaData *data) {
  gpsLog.logln("parse data", "INFO", true);
  if (verifyChkSum(inputData) == 1) {
    readGGAData(inputData, data); //shreds the string it is passed (if you need it afterwards give it a copy)
    if (data->vld == 1) {
      return 1;
    }
  }
  return 0;
}

// wont work

// void sleepGNSS(int sleepTime, HardwareSerial &serPort) {
//   gpsLog.logln("send sleep command to GPS", "INFO", true);
//   char cmd[24], hex[4]; //checksum is 2 chars of hex
//   int chkSum;
//   snprintf(cmd, 24, "PCAS12,%d*", sleepTime);
//   chkSum = calcChkSum(cmd);
//   snprintf(hex, 4, "%X", chkSum);
//   serPort.print("$"); //sends sleep command
//   serPort.print(cmd);
//   serPort.print(hex);
//   serPort.print("\r\n");
// }

void readGNSS(nmeaData *data, HardwareSerial &serPort) {
  gpsLog.logln("reads GPS nema data", "INFO", true);
  char inbuf[128];
  int inpos = 0;
  bool dataReceved = 0;
  const char GNGGA[] = "GNGGA";
  data->vld = 0;
  uint32_t startMs = millis();
  const uint32_t timeoutMs = 30000; //30s timeout
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
        inbuf[inpos++] = '\0';
        gpsLog.logln(inbuf, "DEBUG", true);

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
  gpsLog.log("lat: ", "DEBUG", true);
  gpsLog.log(GNSSData.lat, "DEBUG", false);
  gpsLog.logln(GNSSData.latDir, "DEBUG", false);

  gpsLog.log("lon: ", "DEBUG", true);
  gpsLog.log(GNSSData.lon, "DEBUG", false);
  gpsLog.logln(GNSSData.lonDir, "DEBUG", false);

  gpsLog.log("UTC: ", "DEBUG", true);
  gpsLog.logln(GNSSData.utc, "DEBUG", false);

  gpsLog.log("nrSat: ", "DEBUG", true);
  gpsLog.logln(GNSSData.nrSat, "DEBUG", false);

  gpsLog.log("vld: ", "DEBUG", true);
  gpsLog.logln(GNSSData.vld, "DEBUG", false);
}

void initGNSS(HardwareSerial &serPort, int RX_pin, int TX_pin) { 
  gpsLog.logln("Init GPS", "INFO", true);
  serPort.begin(9600, SERIAL_8N1, RX_pin, TX_pin);
  while (!serPort) {} //waits until serial port has initialized
  serPort.flush();
  serPort.print(GNSSSTARTCMD);
}
