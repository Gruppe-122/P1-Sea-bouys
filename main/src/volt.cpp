#include "volt.h"

Volt::Volt(int pin, float R1, float R2,
           adc_attenuation_t atten, int resolution)
    : _pin(pin),
      _resolution(resolution),
      _atten(atten),
      _samples(20),
      _tid_m_samples(20),
      _R1(R1),
      _R2(R2)
{
    _divFactor = _R2 / (_R1 + _R2);
}

void Volt::begin()
{
    analogReadResolution(_resolution);
    analogSetPinAttenuation(_pin, _atten);
}

void Volt::set_sampling(int samples, int tid_m_samples)
{
    _samples = samples;
    _tid_m_samples = tid_m_samples;
}

int Volt::avg_ADC()
{
    long sum = 0;

    for (int i = 0; i < _samples; i++) {
        sum += analogRead(_pin);
        delay(_tid_m_samples);
    }

    return sum / _samples;
}

float Volt::read_voltage_mV()
{
    return analogReadMilliVolts(_pin);
}

float Volt::read_battery_voltage_mV()
{
    float mv = analogReadMilliVolts(_pin);
    return mv / _divFactor;
}
