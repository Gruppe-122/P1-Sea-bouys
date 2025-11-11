#include "volt.h"

Volt::Volt(int pin, int vRef, float R1, float R2,
           adc_atten_t atten, int resolution)
    : _pin(pin),
      _vRef(vRef),
      _resolution(resolution),
      _atten(atten),
      _samples(8),
      _tid_m_samples(5),
      _R1(R1),
      _R2(R2)
{
    _divFactor = R1 / (R1 + R2);

    analogReadResolution(_resolution);
    analogSetPinAttenuation(_pin, (adc_attenuation_t)_atten);
    esp_adc_cal_characterize(ADC_UNIT_1, (adc_atten_t)_atten, _resolution_ADC, _vRef, &_adcChars);
}

void Volt::set_sampling(int samples, int tid_m_samples)
{
    _samples = samples;
    _tid_m_samples = tid_m_samples;
}

int Volt::avg_ADC()
{
    long sum = 0;
    for (int i = 0; i < _samples; i++)
    {
        sum += analogRead(_pin);
        delay(_tid_m_samples);
    }
    return (float)sum / _samples;
}

float Volt::read_ADC_voltage()
{
    int rawAvg = (int)avg_ADC();
    return esp_adc_cal_raw_to_voltage(rawAvg, &_adcChars);
}

float Volt::read_battery_voltage_mV()
{
    float mv = analogReadMilliVolts(_pin);
    return (mv / _divFactor);
}
