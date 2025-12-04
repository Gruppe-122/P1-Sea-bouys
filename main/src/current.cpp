#include "current.h"

CurrentSensor::CurrentSensor(int pin, int dcOffset_mV, int modSensitivity_mV_per_A, adc_attenuation_t atten, uint8_t resolution)
{
  _pin = pin;
  _dcOffset_mV = dcOffset_mV;
  _mod_mV_per_A = modSensitivity_mV_per_A;
  _atten = (adc_attenuation_t)atten;
  _adc_resolution = (uint8_t)resolution;
}

void CurrentSensor::begin()
{
  analogReadResolution(_adc_resolution); // Set ADC resolution
  analogSetPinAttenuation(_pin, _atten); // Set input attenuation for 0-3.1V range
}

void CurrentSensor::set_sampling(int samples, int tid_m_samples, int adc_resolution)
{
  _samples = samples;
  _tid_m_samples = tid_m_samples;
  _adc_resolution = adc_resolution;
}

// Direct import from volt.cpp
uint CurrentSensor::moving_avg_ADC()
{
    uint new_sample = analogRead(_pin);

    // Remove oldest sample from sum
    ma_sum -= ma_buffer[ma_index];

    // Insert newest sample
    ma_buffer[ma_index] = new_sample;
    ma_sum += new_sample;

    // Next index
    ma_index++;
    if (ma_index >= _ma_samples)
    {
        ma_index = 0;
        ma_full = true;
    }

    if (!ma_full)
        return ma_sum / ma_index;

    return ma_sum / _ma_samples;
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
// Potential improvement: Use moving average filter instead of simple average
// and lookup table for better accuracy
float CurrentSensor::get_voltage_mV()
{
  return analogReadMilliVolts(_pin);
}

float CurrentSensor::measure_current_A()
{
  float voltage_mV = get_voltage_mV();
  // typecast to float to avoid integer division issues
  float amps = (voltage_mV - (float)_dcOffset_mV) / _mod_mV_per_A;
  return amps;
}

float CurrentSensor::measure_current_mA()
{
  return 1000 * measure_current_A();
}
