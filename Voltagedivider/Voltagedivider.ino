#include "driver/adc.h"
#include "esp_adc_cal.h"
/*
   Battery voltage measurement via voltage divider
   R1 = 47kΩ (top), R2 = 10kΩ (bottom)
   Input: up to ~18 V  →  ADC pin sees ~3.15 V
   Board: ESP32-S3 (Arduino core)
*/

#include <Arduino.h>

// ---- Divider parameters ----
#define R1 47000.0
#define R2 10000.0
const float DIV_FACTOR = (R1 + R2) / R2;  // Vin = Vout * DIV_FACTOR

// ---- ADC configuration ----
#define ADC_PIN 34        // change to your actual ADC-capable GPIO
#define ADC_SAMPLES 8
#define ADC_ATTEN ADC_11db  // allows up to ≈3.3 V input
#define ADC_WIDTH ADC_WIDTH_BIT_12 // 12-bit resolution
#define NOMINAL_VREF 1100.0 // mV, will be corrected if eFuse calibration found

// ---- Sleep period (1 h) ----
constexpr uint64_t SLEEP_US = 3600ULL * 1000000ULL;

// --- optional low-pass RC constants for timing ---
constexpr float Rth = (R1 * R2) / (R1 + R2);
constexpr float C_input = 100e-9;  // 0.1 µF
constexpr int settle_ms = (int)(5.0 * Rth * C_input * 1000.0); // ≈4 ms

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Starting battery measurement...");

  // ---- Configure ADC ----
  analogReadResolution(ADC_WIDTH);
  analogSetPinAttenuation(ADC_PIN, ADC_ATTEN);

  // Check if ADC is factory-calibrated (eFuse reference)
  esp_err_t status;
  esp_adc_cal_characteristics_t characteristics;
  status = esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_TP);
  bool calibrated = (status == ESP_OK);
  Serial.printf("Calibration eFuse present: %s\n", calibrated ? "YES" : "NO");

  if (calibrated) {
    esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH, NOMINAL_VREF, &characteristics);

    Serial.println("ADC calibrated using eFuse data.");
  } else {
    Serial.println("Using nominal Vref (uncalibrated).");
  }

  // ---- Settling delay ----
  delay(settle_ms);

  // ---- Read and average samples ----
  uint32_t total = 0;
  for (int i = 0; i < ADC_SAMPLES; i++) {
    total += analogRead(ADC_PIN);
    delay(5); // small delay between samples
  }
  uint32_t raw = total / ADC_SAMPLES;

  // ---- Convert raw to mV ----
  uint32_t mv;
  if (calibrated) {
    mv = esp_adc_cal_raw_to_voltage(raw, &characteristics);
  } else {
    mv = (uint32_t)((raw * NOMINAL_VREF) / 4095.0);
  }

  // ---- Compute actual battery voltage ----
  float battery_mv = mv * DIV_FACTOR;
  Serial.printf("ADC raw: %lu  |  mid-node: %lu mV  |  battery: %.2f V\n",
                raw, mv, battery_mv / 1000.0);

  // ---- Enter deep sleep for 1 hour ----
  Serial.println("Entering deep sleep for 1 hour...");
  esp_sleep_enable_timer_wakeup(SLEEP_US);
  esp_deep_sleep_start();
}

void loop() {
  // never runs; device sleeps after setup()
}
