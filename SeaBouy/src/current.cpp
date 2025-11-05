#include "current.h"
#include <Arduino.h>
#include "driver/adc.h"
#include "esp_adc_cal.h"

CurrentSensor::CurrentSensor(int pin, int dcOffset_mV, int modSensitivity_mV_per_A)
{
  _pin = pin;
  _dcOffset_mV = dcOffset_mV;
  _mod_mV_per_A = modSensitivity_mV_per_A;
}

void CurrentSensor::begin()
{
  analogReadResolution(_adc_resolution); // Set ADC resolution
  analogSetAttenuation(ADC_11db);       // Set input attenuation for 0-3.3V range
  esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 1100, &adc_chars);
}

void CurrentSensor::set_sampling(int samples, int tid_m_samples, int adc_resolution)
{
  _samples = samples;
  _tid_m_samples = tid_m_samples;
  _adc_resolution = adc_resolution;
}

int CurrentSensor::avg_ADC(int samples, int tid_m_samples)
{
  long sum = 0;
  int adc = 0;
  for (int i = 0; i < samples; i++)
  {
    adc = analogRead(_pin);
    sum += adc;
    delay(tid_m_samples);
  }
  return sum / samples;
}

float CurrentSensor::get_voltage_mV()
{

  int avg_adc_val = avg_ADC(_samples, _tid_m_samples);
  return esp_adc_cal_raw_to_voltage(avg_adc_val, &adc_chars);
}

float CurrentSensor::measure_current_A()
{
  int avg_adc_val = avg_ADC(_samples, _tid_m_samples);
  uint32_t voltage_mV = get_voltage_mV();
  float amps = (voltage_mV - _dcOffset_mV) / _mod_mV_per_A;
  return amps;
}

float CurrentSensor::measure_current_mA()
{
  return 1000 * measure_current_A();
}
