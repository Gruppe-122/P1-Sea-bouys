#include "current.h"

CurrentSensor::CurrentSensor(int pin, int dcOffset_mV, int modSensitivity_mV_per_A, adc_attenuation_t atten, uint8_t resolution)
{
  currentLog.logln("current objet construtor", "INFO", true);
  _pin = pin;
  _dcOffset_mV = dcOffset_mV;
  _mod_mV_per_A = modSensitivity_mV_per_A;
  _atten = (adc_attenuation_t)atten;
  _adc_resolution = (uint8_t)resolution;
}

void CurrentSensor::begin()
{
  currentLog.logln("set ADC resolution and input attenaution", "INFO", true);
  analogReadResolution(_adc_resolution);          // Set ADC resolution
  analogSetPinAttenuation(_pin, _atten);        // Set input attenuation for 0-3.1V range
}

void CurrentSensor::set_sampling(int samples, int tid_m_samples, int adc_resolution)
{
  currentLog.logln("set sampling", "INFO", true);
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
  currentLog.logln("avg_ADC", "INFO", true);
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
  currentLog.logln("get voltage mV", "INFO", true);
  return analogReadMilliVolts(_pin);
}

float CurrentSensor::measure_current_A()
{
<<<<<<< HEAD
  currentLog.logln("measure current A", "INFO", true);
  uint32_t voltage_mV = get_voltage_mV();
  float amps = (voltage_mV - _dcOffset_mV) / _mod_mV_per_A;
=======
  float voltage_mV = get_voltage_mV();
  // typecast to float to avoid integer division issues
  float amps = (voltage_mV - (float)_dcOffset_mV) / _mod_mV_per_A;
>>>>>>> origin
  return amps;
}

float CurrentSensor::measure_current_mA()
{
  currentLog.logln("measure current mA", "INFO", true);
  return 1000 * measure_current_A();
}
