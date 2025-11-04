// -----------------------------------------------------------------------
// | Sketch for measuring current with ESP32-S3 and ACS712 currentsensor |
// |               Pin layout| ACS712 -> ESP32-S3 |                      |
// |                           "Vcc"  -> 5V                              |
// |                           "GND"  -> GND                             |
// |                           "Out"  -> ANALOG_PIN (GPIO19)             |
// -----------------------------------------------------------------------
// Configuration
#define ANALOG_PIN 19           // GPIOpin for currentsensor data
#define ADC_RESOLUTION 12       // ADC resolution for ESP32 is 12bit (4096 bytes). Arduino UNO has 10bit
#define VOLTAGE_REFERENCE 3.3   // ESP32-S3 reference voltage
#define DC_offset 2.5           //DC offset from currentsensor module. Meaning with 0 current through the sensor, the sensor output is 2.5V
#define MOD_SENS 100            //Module sensitivy, which is 100mv/A according to datasheet for 20Ax version

//Variabels
float amps;
float mAmps;

void setup() {
  Serial.begin(115200);
  analogReadResolution(ADC_RESOLUTION);  // Set ADC resolution
  analogSetAttenuation(ADC_11db);        // Set input attenuation for 0-3.3V range
  
  Serial.println(__FILE__);
  Serial.println("ADC value\tVoltage (V)\Current (mA)");
  Serial.println("--------------------------------");
}

void loop() {
  int rawValue = analogRead(ANALOG_PIN);
  float voltage = (rawValue * VOLTAGE_REFERENCE) / (pow(2, ADC_RESOLUTION) - 1); //Converts raw ADC signal into voltage pow(2, ADC_RESOLUTION) is = 2^12=4096
  
  amps = (voltage - DC_offset) / MOD_SENS; //Formula for converting the analog voltage from ACS712 to a current. 
  mAmps = amps * 1000; 

  Serial.print("ADC value: ");
  Serial.print(rawValue);
  Serial.print("  |  Voltage: ");
  Serial.print(voltage, 3);  // 3 decimal places
  Serial.print("V");
  Serial.print("  |  Current: ");
  Serial.print(mAmps, 3);    // 3 decimal places
  Serial.println("mA");
  delay(500);                // Read every 500ms
}