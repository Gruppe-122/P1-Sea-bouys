// -----------------------------------------------------------------------
// | Land station sketch Buoy project                                    |
// | Board: Heltec WiFi LoRa 32(V3)                                      |                                                     |
// -----------------------------------------------------------------------

// Landstationskode

#include "src/mesh.h"
#include <math.h>

// Definitions
#define BUOY_AMOUNT 5
#define AMOUNT_BEFORE_ALARM 3
#define METERS_PER_DEGREE_LAT 111111.0
#define DIST_THRESH 30.0
// This is the ID on the buoy we're sending to:
#define BUOY_ID_TEST 3 
#define TEST true

//Variables
meshalternativ buoy;
BuoyData fakeData;
BuoyData receivedData;

struct errorMessage {
  int buoy_number;
  bool missing = false;
  int lantern;
  bool accelerometerHit = false;
  double gps_lon;
  double gps_lat;
};

struct errorMessage listBuoys[BUOY_AMOUNT];

logger meshLog = logger("MESH", "INFO");

int idCheck[BUOY_AMOUNT];
uint8_t signalFrom[BUOY_AMOUNT];
uint8_t batteryStatus[BUOY_AMOUNT];
uint8_t accelerometerHit[BUOY_AMOUNT];
int receivedIDs = 0;
int initialTest = 0;
int errorSendMessage;
int startTest;
bool idAbove = 1;
uint64_t fakeWiFiTimer = 0;
uint64_t maximumWiFiTimer = 0;
double location[][2] = {
  // Locations for:
  // Buoy 1
  {12.345678, 1.234567},
  // Buoy 2
  {12.345678, 1.234567},
  // Buoy 3
  {57.055533, 9.925497},
  // Buoy 4
  {12.345678, 1.234567},
  // Buoy 5
  {12.345678, 1.234567}
};

double maxDistance = ((DIST_THRESH * DIST_THRESH) / (METERS_PER_DEGREE_LAT * METERS_PER_DEGREE_LAT));

// Placing global variable onto the RTC module that gets remembered through deep sleep
// 8 KB to do with -> better than flash despite flash being bigger
// Flash wears over time, not good for many iterations
// These are to count the amount 
RTC_DATA_ATTR uint8_t missingCount[BUOY_AMOUNT];
RTC_DATA_ATTR uint8_t gpsCount[BUOY_AMOUNT];
RTC_DATA_ATTR uint8_t lanternCount[BUOY_AMOUNT];

double convertTodegrees(double raw) {

  int degrees = (int)(raw / 100);
  double minutes = raw - (degrees * 100);
  double decimal = degrees + (minutes / 60);

  return decimal;
}

void testBuoy() {
  // Insert the first test 
  fakeData.buoy_number = BUOY_ID_TEST - 1;
  fakeData.sent_from = BUOY_ID_TEST - 1;
  delay(1000);
  buoy.send_data(fakeData);
  delay(2000);
  receivedData.buoy_number = 0;
  buoy.receive_data(receivedData);
  // Shouldn't receive anything!
  if (receivedData.buoy_number == 0) {
    Serial.println("Buoy correctly disregarded data!");
  }
  else if (receivedData.buoy_number == 3) {
    Serial.println("Error! Got data back!");
  }
  else {
    Serial.println("Error! Buoy ID was neither 0 nor 3!");
    Serial.println("Error happened during transmission of ID under buoy!");
  }


  // Send from ID 1 above, which should instantly send back.
  // Also, send message again later, to see if buoy ignores second message
  for (int i = 0; i < 2; i++) {
    fakeData.buoy_number = BUOY_ID_TEST + 1;
    fakeData.sent_from = BUOY_ID_TEST + 1;
    delay(1000);
    buoy.send_data(fakeData);
    delay(2000);
    receivedData.buoy_number = 0;
    buoy.receive_data(receivedData);
    // 
    if (i == 0) {
      Serial.println("This should have the value 3:");
      Serial.println(receivedData.buoy_number);
      // Resets to check if we get the message again when we send it a second time
      receivedData.buoy_number = 0;
    }
    // If sent a second time
    else if (i == 1 && receivedData.buoy_number == 3) {
      Serial.println("It failed! I received data again from the buoy.");
    }
    else if (i == 1 && receivedData.buoy_number == 0) {
      Serial.println("Buoy corrected disregarded data!");
    }
    else {
      Serial.println("Error! Buoy ID was neither 0 nor 3!");
      Serial.println("This happened during ID + 1, the 'send same message twice' check, to see if data would be sent twice");
    }
    // If something is missing, save!
    // Check also if something was sent back here, to see if buoy sends same ID twice

  }

  // Check if sending a signal from far away triggers the buoy instantly or waits
  fakeData.buoy_number = BUOY_ID_TEST + 2;
  fakeData.sent_from = BUOY_ID_TEST + 2;
  delay(1000);
  Serial.println("Sent from ID 2 above");
  buoy.send_data(fakeData);
  // Small delay to make sure it could send data back in case something's wrong
  // Delay shouldn't be too big, buoy sends data after 600 milliseconds if it doesn't get from another buoy
  delay(200);
  // If no data below, check is correct, nothing was sent back!
  fakeData.buoy_number = 0;
  buoy.receive_data(receivedData);
  Serial.println("Should be 0:");
  Serial.println(fakeData.buoy_number);
  // NOW see if it gets the message if you pretend to send it from the one it's supposed to hear from
  if (idAbove == 1) {
  fakeData.sent_from = BUOY_ID_TEST + 1;
  fakeData.buoy_number = BUOY_ID_TEST + 2;
  buoy.send_data(fakeData);
  delay(200);
  fakeData.buoy_number = 0;
  buoy.receive_data(receivedData);
  Serial.println("Got ID 2 info from ID 1!");
  Serial.println("ID should be 3:");
  Serial.println(fakeData.buoy_number);
  }
  else {
    delay(1000);
    fakeData.buoy_number = 0;
    buoy.receive_data(receivedData);
    Serial.println("Got ID 2 info from ID 2!");
    Serial.println("ID should be 3:");
    Serial.println(fakeData.buoy_number);
  }
    
  // If something is missing, save!
    
  // Check also ID above 3
  fakeData.buoy_number = BUOY_ID_TEST + 3;
  delay(1000);
  buoy.send_data(fakeData);
  fakeData.buoy_number = 0;
  delay(2000);
  buoy.receive_data(receivedData);
  Serial.println("Should be 0:");
  Serial.println(fakeData.buoy_number);

  // We now need to make sure the landstation thinks it's gotten a whole system of buoys
  for (int i = 0; i < BUOY_AMOUNT; i++) {
    if (i != (BUOY_ID_TEST - 1)) {
      listBuoys[i].buoy_number = i + 1;
      listBuoys[i].lantern = 0;
      listBuoys[i].gps_lon = 12.345678;
      listBuoys[i].gps_lat = 1.234567;
    }
  }
  // First Buoy disappeared
  listBuoys[0].buoy_number = 0;
  // Second Buoy was hit
  listBuoys[1].accelerometerHit = true;
  // Third Buoy is out of location
  listBuoys[4].gps_lat = 3.123456;
}

void setup() {
  delay(1000);
  Serial.begin(115200);
  buoy.start_radio();
  receivedData.buoy_number = 0;
}

void loop() {
  if (initialTest == 0) {
    // Wait a certain amount of time by looking at time via WiFi:
    // Pretend code is here, then we just add a millis based on it:
    // Also an extra maximum time in case a buoy alarm keeps going off for 25 minutes not resetting the system
    fakeWiFiTimer = millis();
    maximumWiFiTimer = millis();
    initialTest = 1;
  }
  
  // Pretend Sleep here is timed with the Internet

  buoy.receive_data(receivedData);
  if (receivedData.buoy_number > 0) {
    // Check here if you have seen the buoy before
    if (idCheck[receivedData.buoy_number - 1] != receivedData.buoy_number) {
      idCheck[receivedData.buoy_number - 1] = receivedData.buoy_number;

      // Needs to make sure that buoy number 1 is in position 0 in the array
      signalFrom[receivedData.buoy_number - 1] = 1;

      // If there's no lamp current
      // Check if Lantern is supposed to be on based on TIME here
      // If it's supposed to be on:
      if (receivedData.lamp_current == false) {
        lanternCount[receivedData.buoy_number - 1]++;
      }

      // Accelerometer check here
      if (receivedData.accelerometer_jerk > 0) {
        accelerometerHit[receivedData.buoy_number - 1] = receivedData.accelerometer_jerk;
      }

      // Add GPS to the received buoy whether or not it's within range
      listBuoys[receivedData.buoy_number - 1].gps_lon = receivedData.gps_longitude;
      listBuoys[receivedData.buoy_number - 1].gps_lat = receivedData.gps_latitude;

      // Original position to GPS position
      double lat_diff_meters =  convertTodegrees(receivedData.gps_latitude) - location[receivedData.buoy_number - 1][0];

      // Longitude degree per meter changes from how far up you are, use original location to get a guesstimate
      double lon_diff_meters = (convertTodegrees(receivedData.gps_longitude) - location[receivedData.buoy_number - 1][1]) * cos(location[receivedData.buoy_number - 1][1] * PI / 180);
      
      // Pythagoras to figure out how far away it is
      double distance = (lon_diff_meters * lon_diff_meters) + (lat_diff_meters * lat_diff_meters);

      // If it's over 30 meters away (30*30 = 900) - An extra check in case the buoy doesn't know it's out of its position
      if (distance > maxDistance) {
        gpsCount[receivedData.buoy_number - 1]++;
        // If program is expanded upon, add code here that sends to a struct so 
      }
      else {
        gpsCount[receivedData.buoy_number - 1] = 0;
      }

      // Print alle værdier fra HER for at vise, at vi fik fra Bøje 3
      Serial.print("Fået information! Fra bøje nr. ");
      Serial.println(receivedData.buoy_number);
      Serial.print("Sent fra: nr. ");
      Serial.println(receivedData.sent_from);
      Serial.println("Batteri spænding:");
      Serial.println(receivedData.battery_voltage);
      Serial.println("GPS Latitude:");
      Serial.println(receivedData.gps_latitude);
      Serial.println("GPS Longitude:");
      Serial.println(receivedData.gps_longitude);
      Serial.println("Accelerometer hit:");
      Serial.println(receivedData.accelerometer_jerk);
      Serial.println("Er lampen tændt?:");
      Serial.println(receivedData.lamp_current);


      // Reset timer because we received a message
      fakeWiFiTimer = millis();

      // Make sure the loop doesn't run again with the same data set
      receivedData.buoy_number = 0;

      // Test a single buoy system
      if (TEST) {
        testBuoy();
      }
    }
  }

  // If 30 seconds passes without a signal or if 25 minutes pass
  if (30000 < millis() - fakeWiFiTimer || 1500000 < millis() - maximumWiFiTimer) {


    // Check all buoys
    for (int i = 0; i < BUOY_AMOUNT; i++) {

      // Reset something is wrong bool
      bool somethingIsWrong = false;
      bool isMissing = false;
      bool batteryLow = false;
      bool lampOff = false;

      // If it didn't get a signal from one of the buoys
      if (signalFrom[i] == 0) {
        missingCount[i]++;
      }
      // If I did get a signal, reset counter
      else if (signalFrom[i] == 1) {
        missingCount[i] = 0;
      }
      else {
        Serial.println("Warning! Something went wrong! Couldn't detect if buoy was missing or not.");
        Serial.println("signalFrom array can only have 0 or 1 in the array. It had neither.");
      }

      // If it hasn't heard from a buoy 3 times
      if (missingCount[i] >= AMOUNT_BEFORE_ALARM) {
        missingCount[i] = 0;
        somethingIsWrong = true;
        isMissing = true;
      }

      // Check if battery is okay
      if (batteryStatus[i] > 0) {
        // Insert a message here talking about status on the battery, either medium, low or battery dead / no signal
        somethingIsWrong = true;
        batteryLow = true;
      }
      // Check if a lantern hasn't been active in the last 3 checks AND it's during a time where it's suppoesed to be active
      if (lanternCount[i] >= AMOUNT_BEFORE_ALARM) {
        lanternCount[i] = 0;
        lampOff = true;
      }

      if (somethingIsWrong == true) {
        listBuoys[i].buoy_number = i + 1;
        // Send Buoy data here!!!! Code is Serial Print instead
        Serial.print("Status on buoy number ");
        Serial.print(listBuoys[i].buoy_number);
        Serial.println(":");
        if (isMissing) {
          Serial.println("Haven't received from Buoy after 3 attempts!");
        }
        if (batteryLow) {
          switch (batteryStatus[i]) {
            case 1:
              Serial.println("Battery is at 40%");
              break;
            case 2:
              Serial.println("Battery is at 15%");
              break;
            case 3:
              Serial.println("Warning: Battery is either dead or couldn't be read.");
              break;
            default:
              Serial.println("ERROR: Battery was registered to be beyond 3, something went wrong in calculating battery level!");
          }
        }
        if (lampOff) {
          Serial.println("Lamp isn't functioning!");
        }
        Serial.println("");
      }
    }


    // Amonut of time the ESP should sleep for, in the real version should be accounted for with WiFi
    // ULL = Unsigned Long Long
    esp_sleep_enable_timer_wakeup(10ULL * 1000000ULL);  // 10 seconds as an example of sleep instead of WiFi
    esp_deep_sleep_start();
  }
}
