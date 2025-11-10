#include "volt.h"

Volt::Volt(int pin, int vRef, float R1, float R2,
           adc_atten_t atten, adc_bits_width_t resolution)
    : _pin(pin),
      _vRef(vRef),
      _resolution(resolution),
      _atten(atten),
      _samples(8),
      _delayMs(5),
      _R1(R1),
      _R2(R2)
{
    _divFactor = (R1 + R2) / R2;

    analogReadResolution((int)_resolution);
    analogSetPinAttenuation(_pin, (adc_attenuation_t)_atten);

    esp_adc_cal_characterize(ADC_UNIT_1, _atten, _resolution,
                             _vRef, &_adcChars);
}

void Volt::setSampling(int samples, int delayMs)
{
    _samples = samples;
    _delayMs = delayMs;
}

float Volt::avg_ADC()
{
    long sum = 0;
    for (int i = 0; i < _samples; i++)
    {
        sum += analogRead(_pin);
        delay(_delayMs);
    }
    return (float)sum / _samples;
}

float Volt::readAdcVoltage()
{
    int rawAvg = (int)avg_ADC();
    return esp_adc_cal_raw_to_voltage(rawAvg, &_adcChars); // mV
}

float Volt::readBatteryVoltage()
{
    float mv = analogReadMilliVolts(_pin); // mV at divider midpoint
    return (_divFactor * mv) / 1000.0f;    // convert to volts
}
