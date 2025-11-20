#include "gps_parser.h"

#define GNSSSTARTCMD "$PCAS03,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*03\r\n"

static const bool USE_DECIMAL_DEGREES = false;

double convertTodegrees(const char *buff)
{
  double raw = atof(buff);
  double absraw = fabs(raw);

  int degrees = (int)(absraw / 100);
  double minutes = absraw - (degrees * 100);
  double decimal = degrees + (minutes / 60);

  if (raw < 0)
    decimal = -decimal;

  return decimal;
}

void readGGAData(char *inputData, nmeaData *data)
{
  char *buff;

  // GGA protocol header
  buff = strtok(inputData, ",*");

  // UTC time hhmmss.sss
  buff = strtok(NULL, ",");
  data->utc = atof(buff);

  // Latitude ddmm.mmmm
  buff = strtok(NULL, ",");
  double latitude = USE_DECIMAL_DEGREES ? convertTodegrees(buff) : atof(buff);

  // N/S indication N=North, S=South
  buff = strtok(NULL, ",");
  data->latDir = *buff;
  if (data->latDir == 'S')
    data->lat = -latitude;
  else
    data->lat = latitude;

  // Longitude dddmm.mmmm
  buff = strtok(NULL, ",");
  double longitude = USE_DECIMAL_DEGREES ? convertTodegrees(buff) : atof(buff);

  // E/W indication E=East, W=West
  buff = strtok(NULL, ",");
  data->lonDir = *buff;
  if (data->lonDir == 'W')
    data->lon = -longitude;
  else
    data->lon = longitude;

  // Positioning 0: not positioned 1: valid Position
  buff = strtok(NULL, ",");
  if (*buff >= '1')
    data->vld = 1;
  else
    data->vld = 0;

  // Number of satellites Range 0 to 12 (lies)
  buff = strtok(NULL, ",");
  data->nrSat = atoi(buff);

  // HDOP Horizontal accuracy
  buff = strtok(NULL, ",");
  data->horPosAck = atof(buff);

  // Mean Sea Level Earth is -2.2 M
  buff = strtok(NULL, ",");

  // Differential time When there is no DGPS, invalid
  buff = strtok(NULL, ",");

  // Differential ID
  buff = strtok(NULL, ",");
}


int charToHex(char in)
{ // converts a char (0-9,A-F) to int (0-9,10-15)
  int out = 0;
  if (in <= '9')
  {
    out = in - '0'; //"0" - '0' = 0x00
  }
  else
  {
    out = in - 'A' + 10; // 0x0A = 10, "A" - 'A' + 10 = 0x0A
  }
  return out;
}

int calcChkSum(char *head)
{
  int count = 0, hash = 0;
  while ((*head != '*') && (*head != '\0'))
  { // chunk end is a *
    count++;
    if (128 < count)
    {
      return -2;
    }
    hash ^= *head; // bitwise xor
    head++;
  }
  return hash;
}

int verifyChkSum(char *inputData)
{
  if (inputData == NULL)
    return 0;
  // skip leading '$' if present
  char *start = inputData;
  if (*start == '$')
    start++;

  // find '*' that precedes checksum
  char *aster = strchr(start, '*');
  if (aster == NULL)
    return 0;

  int hash = calcChkSum(start);
  // checksum is two hex digits after '*'
  if (!isxdigit((unsigned char)aster[1]) || !isxdigit((unsigned char)aster[2]))
    return 0;
  int chkSum = (charToHex(aster[1]) << 4) + charToHex(aster[2]);

  return (hash == chkSum) ? 1 : 0;
}

int parseGNSSData(char *inputData, nmeaData *data)
{
  if (verifyChkSum(inputData) == 1)
  {
    readGGAData(inputData, data); // shreds the string it is passed (if you need it afterwards give it a copy)
    if (data->vld == 1)
    {
      return 1;
    }
  }
  return 0;
}

void sleepGNSS(int sleepTime, HardwareSerial &serPort)
{
  char cmd[32] = {0};
  char hex[8] = {0};
  int chkSum;
  // build command WITHOUT leading '$'
  snprintf(cmd, sizeof(cmd), "PCAS12,%d*", sleepTime);
  chkSum = calcChkSum(cmd);
  snprintf(hex, sizeof(hex), "%02X", chkSum & 0xFF);
  serPort.print("$"); // sends sleep command
  serPort.print(cmd);
  serPort.print(hex);
  serPort.print("\r\n");
}

void readGNSS(nmeaData *data, HardwareSerial &serPort)
{
  char inbuf[256];
  bool dataReceived = false;
  bool collecting = false;
  String line = "";

  data->vld = 0;
  uint32_t startMs = millis();
  const uint32_t timeoutMs = 3000; // 3s timeout for at undgå at blokere

  while (!dataReceived)
  {
    if ((millis() - startMs) > timeoutMs)
    {
      Serial.println("readGNSS: timeout waiting for GNSS data");
      return;
    }

    while (serPort.available() > 0)
    {
      int inByte = serPort.read();

      // start på ny NMEA-sætning
      if (inByte == '$')
      {
        collecting = true;
        line = "";
      }

      if (collecting)
      {
        if (inByte == '\n')
        {
          // slut på linje
          collecting = false;

          // vi forventer en GGA-sætning
          if (!line.startsWith("$GNGGA"))
          {
            line = "";
            continue;
          }

          // kopier til C-buffer
          size_t len = line.length();
          if (len >= sizeof(inbuf))
            len = sizeof(inbuf) - 1;
          line.toCharArray(inbuf, len + 1);

          dataReceived = parseGNSSData(inbuf, data);

          if (!dataReceived)
          {
            Serial.println("readGNSS: message checksum/valid failed");
          }
          else
          {
            return; // alt ok, data->vld er sat
          }
        }
        else if (inByte != '\r')
        {
          // tilføj alle tegn undtagen CR
          line += (char)inByte;
        }
      }
    }
  }
}

void PrintGPSData(nmeaData &GNSSData){
  Serial.print("lat: ");
  Serial.println(GNSSData.lat, sizeof(double));
  Serial.print("lon: ");
  Serial.println(GNSSData.lon, sizeof(double));
  Serial.print("UTC: ");
  Serial.println(GNSSData.utc);
  Serial.print("nrSat: ");
  Serial.println(GNSSData.nrSat);
}

void initGNSS(HardwareSerial &serPort, int RX_pin, int TX_pin)
{
  serPort.begin(9600, SERIAL_8N1, RX_pin, TX_pin);
  // small delay to allow port to initialize
  delay(100);
  serPort.flush();
  serPort.print(GNSSSTARTCMD);
  Serial.println("initGNSS: GNSS start command sent");
}