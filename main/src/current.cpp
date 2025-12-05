#include "current.h"

CurrentSensor::CurrentSensor(int pin, int dcOffset_mV, int modSensitivity_mV_per_A, adc_attenuation_t atten)
{
  currentLog.logln("current objet construtor", "INFO", true);
  _pin = pin;
  _dcOffset_mV = dcOffset_mV;
  _mod_mV_per_A = modSensitivity_mV_per_A;
  _atten = atten;
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

float CurrentSensor::get_voltage_mV()
{
  currentLog.logln("get voltage mV", "INFO", true);
  return analogReadMilliVolts(_pin);
}

float CurrentSensor::measure_current_A()
{
  currentLog.logln("measure current A", "INFO", true);
  uint32_t voltage_mV = get_voltage_mV();
  float amps = (voltage_mV - _dcOffset_mV) / _mod_mV_per_A;
  return amps;
}

float CurrentSensor::measure_current_mA()
{
  currentLog.logln("measure current mA", "INFO", true);
  return 1000 * measure_current_A();
}
