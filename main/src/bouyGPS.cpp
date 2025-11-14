#include "bouyGPS.h"

myGps::myGps() : gpsSerial(2) // use UART2
{
    Serial.begin(115200);
    delay(200);
    gpsSerial.begin(GPS_BAUD, SERIAL_8N1, RX_PIN, TX_PIN);
}

// --- in your class (header) ---
// bool sendPMTKAndWaitAck(const char* cmd, int expectedCmdId, uint16_t timeoutMs = 1000);

bool myGps::sendPMTKAndWaitAck(const char* cmd, int expectedCmdId, uint16_t timeoutMs) {
  gpsSerial.print(cmd);
  gpsSerial.print("\r\n");

  unsigned long t0 = millis();
  String line;
  while (millis() - t0 < timeoutMs) {
    while (gpsSerial.available()) {
      char c = gpsSerial.read();
      if (c == '\n') {
        // Example ACK: $PMTK001,161,3*36
        if (line.startsWith("$PMTK001,")) {
          int i1 = line.indexOf(',', 8);
          if (i1 > 0) {
            int cmdId = line.substring(8, i1).toInt();
            if (expectedCmdId < 0 || cmdId == expectedCmdId) return true;
          }
        }
        line = "";
      } else if (c != '\r') {
        line += c;
        if (line.length() > 120) line = "";
      }
    }
  }
  return false;
}


void myGps::sleep() {
  // Many firmwares won't ACK because they sleep immediately.
  sendPMTKAndWaitAck("$PMTK161,0*28", 161, 150); // OK if this returns false
  Serial.println("Sleep command sent (NMEA likely stops now).");
}

void myGps::wake() {
  gpsSerial.write('\r');        // any byte wakes from standby
  Serial.println("Wake byte sent, waiting for NMEA...");
}

void myGps::ping() {
  bool ok = sendPMTKAndWaitAck("$PMTK000*32", 0, 800); // general query
  Serial.println(ok ? "PMTK link OK (ACK received)" : "No PMTK ACK");
}

void myGps::run()
{
    while (gpsSerial.available() > 0)
    {
        int b = gpsSerial.read();
        if (b >= 0)
        {
            char c = static_cast<char>(b);
            parser.encode(c);
        }
    }

    // Status heartbeat every second
    if (millis() - lastReport > 1000)
    {
        lastReport = millis();
        Serial.print("\n[status] chars=");
        Serial.print(parser.charsProcessed());
        Serial.print(" sats=");
        if (parser.satellites.isValid())
            Serial.print(parser.satellites.value());
        else
            Serial.print("-");
        Serial.print(" hdop=");
        if (parser.hdop.isValid())
            Serial.print(parser.hdop.hdop(), 1);
        else
            Serial.print("-");
        Serial.print(" loc.valid=");
        Serial.println(parser.location.isValid() ? "yes" : "no");
    }

    // When location becomes valid/updated, print parsed once per second
    if (parser.location.isUpdated())
    {
        if (!announcedFix)
        {
            announcedFix = true;
            Serial.println("Gps is working");
        }
        static unsigned long lastParsed = 0;
        if (millis() - lastParsed > 1000)
        {
            lastParsed = millis();
            Serial.printf(
                "[parsed] Lat: %.6f  Lon: %.6f  Sats:%u  HDOP:%.1f  Alt:%.1f m  Time:%02d:%02d:%02dZ\n",
                parser.location.lat(),
                parser.location.lng(),
                parser.satellites.isValid() ? parser.satellites.value() : 0,
                parser.hdop.isValid() ? parser.hdop.hdop() : -1.0,
                parser.altitude.isValid() ? parser.altitude.meters() : NAN,
                parser.time.isValid() ? parser.time.hour() : -1,
                parser.time.isValid() ? parser.time.minute() : -1,
                parser.time.isValid() ? parser.time.second() : -1);
        }
    }
}
