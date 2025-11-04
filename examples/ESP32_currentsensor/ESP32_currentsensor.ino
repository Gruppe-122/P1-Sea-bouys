// -----------------------------------------------------------------------
// | Sketch for measuring current with ESP32-S3 and ACS712 currentsensor |
// | Pin layout:                                                         |
// |  ACS712  -> ESP32-S3                                                |
// |   "Vcc"  -> 5V                                                      |
// |   "GND"  -> GND                                                     |
// |   "Out"  -> ANALOG_PIN (GPIO19)                                     |
// | Make sure ESP32 is properly inserted in breadboard or DC_OFFSET     |
// | will increase                                                       |
// -----------------------------------------------------------------------

//libraries
#include "driver/adc.h"
#include "esp_adc_cal.h"

// Configuration
#define ANALOG_PIN 19          // GPIOpin for currentsensor data
#define ADC_RESOLUTION 12      // ADC resolution for ESP32 is 12bit (4096 bytes). Arduino UNO has 10bit
#define VOLTAGE_REFERENCE 3.3  // ESP32-S3 reference voltage
#define DC_OFFSET 2545         //DC offset from currentsensor module. Meaning with 0 current through the sensor. 2,5 according to datasheet.
#define MOD_SENS 100           //Module sensitivy, which is 100mv/A according to datasheet for 20Ax version
#define SAMPLE_SIZE 20         //Take the average signal of SAMPLE_SIZE inputs
#define TIME_SAMPLE 20         //Time between reading ADC values (ms)
#define LOOP_DELAY  500        //Delay time executed in the end of every loop (ms)

//Variabels
float amps;
float mAmps;
esp_adc_cal_characteristics_t adc_chars;
int avg_adc_val;

void setup() {
  Serial.begin(115200);
  analogReadResolution(ADC_RESOLUTION);  // Set ADC resolution
  analogSetAttenuation(ADC_11db);        // Set input attenuation for 0-3.3V range
  esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 1100, &adc_chars);

  Serial.println(__FILE__);
  Serial.println("ADC value\tVoltage (V)\tCurrent (mA)");
  Serial.println("--------------------------------");
}
//Functions
//Get the average ADC input on pin
int avg_ADC(int pin, int samples) {
  long sum = 0;
  int adc = 0;
  for (int i = 0; i < samples; i++) {
    adc = analogRead(pin);
    sum += adc;
    delay(TIME_SAMPLE);
  }
  return sum / samples;
}

void loop() {
  avg_adc_val = avg_ADC(ANALOG_PIN, SAMPLE_SIZE);
  uint32_t voltage_mV = esp_adc_cal_raw_to_voltage(avg_adc_val, &adc_chars);
  amps = (voltage_mV - DC_OFFSET) / (float)MOD_SENS;  //Formula for converting the analog voltage from ACS712 to a current.
  mAmps = amps * 1000;
  Serial.print("ADC value: ");
  Serial.print(avg_adc_val);
  Serial.print("  |  Voltage: ");
  Serial.print(voltage_mV);
  Serial.print("mV");
  Serial.print("  |  Current: ");
  Serial.print(mAmps);
  Serial.println("mA");
  delay(LOOP_DELAY);
}