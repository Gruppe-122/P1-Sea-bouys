#include "bouyGPS.h"

myGps::myGps() : gpsSerial(2) // use UART2
{
    Serial.begin(115200);
    delay(200);
    Serial.println("\nGPS debug: printing RAW NMEA until fix (RX=6, TX=5)");

    // Start the GPS UART (note: begin on the serial, NOT on TinyGPSPlus)
    gpsSerial.begin(GPS_BAUD, SERIAL_8N1, RX_PIN, TX_PIN);
}

void myGps::run()
{
    // Pump GPS -> TinyGPS++ and echo raw NMEA
    while (gpsSerial.available() > 0) {
        int b = gpsSerial.read();
        if (b >= 0) {
            char c = static_cast<char>(b);
            parser.encode(c);     // feed the parser
            Serial.write(c);   // echo raw
        }
    }

    // Status heartbeat every second
    if (millis() - lastReport > 1000) {
        lastReport = millis();
    Serial.print("\n[status] chars=");
    Serial.print(parser.charsProcessed());
        Serial.print(" sats=");
        if (parser.satellites.isValid()) Serial.print(parser.satellites.value());
        else                          Serial.print("-");
        Serial.print(" hdop=");
        if (parser.hdop.isValid())       Serial.print(parser.hdop.hdop(), 1);
        else                          Serial.print("-");
        Serial.print(" loc.valid=");
        Serial.println(parser.location.isValid() ? "yes" : "no");
    }

    // When location becomes valid/updated, print parsed once per second
    if (parser.location.isUpdated()) {
        if (!announcedFix) {
            announcedFix = true;
            Serial.println("\n=== FIX ACQUIRED -> switching to parsed summary below (RAW will still print) ===");
        }
        static unsigned long lastParsed = 0;
        if (millis() - lastParsed > 1000) {
            lastParsed = millis();
            Serial.printf(
                "[parsed] Lat: %.6f  Lon: %.6f  Sats:%u  HDOP:%.1f  Alt:%.1f m  Time:%02d:%02d:%02dZ\n",
                parser.location.lat(),
                parser.location.lng(),
                parser.satellites.isValid() ? parser.satellites.value() : 0,
                parser.hdop.isValid()       ? parser.hdop.hdop()        : -1.0,
                parser.altitude.isValid()   ? parser.altitude.meters()  : NAN,
                parser.time.isValid()       ? parser.time.hour()        : -1,
                parser.time.isValid()       ? parser.time.minute()      : -1,
                parser.time.isValid()       ? parser.time.second()      : -1
            );
        }
    }
}
