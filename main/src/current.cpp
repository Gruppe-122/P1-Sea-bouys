#include "current.h"

CurrentSensor::CurrentSensor(int pin, int dcOffset_mV, int modSensitivity_mV_per_A, adc_attenuation_t atten)
{
  _pin = pin;
  _dcOffset_mV = dcOffset_mV;
  _mod_mV_per_A = modSensitivity_mV_per_A;
  _atten = atten;
}

void CurrentSensor::begin()
{
  analogReadResolution(_adc_resolution);          // Set ADC resolution
  analogSetPinAttenuation(_pin, _atten);        // Set input attenuation for 0-3.1V range
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

  return analogReadMilliVolts(_pin);
}

float CurrentSensor::measure_current_A()
{
  uint32_t voltage_mV = get_voltage_mV();
  float amps = (voltage_mV - _dcOffset_mV) / _mod_mV_per_A;
  return amps;
}

float CurrentSensor::measure_current_mA()
{
  return 1000 * measure_current_A();
}
