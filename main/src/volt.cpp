#include "volt.h"

Volt::Volt(int pin, float R1, float R2, int ma_samples,
           adc_attenuation_t atten, int resolution)
    : _pin(pin),
      _R1(R1),
      _R2(R2),
      _ma_samples(ma_samples),
      _resolution(resolution),
      _atten(atten)
{
    _divFactor = _R2 / (_R1 + _R2);
    // Suggestion for code improvement: dynamically allocate ma_buffer based on ma_samples
    // meaning you have to use new and delete in constructor and destructor like this
    // ma_buffer = new int[ma_samples];
    // and delete[] ma_buffer; in destructor
}

void Volt::begin()
{
    analogReadResolution(_resolution);
    analogSetPinAttenuation(_pin, _atten);
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

float Volt::ADC_to_mV(int adc)
{
    // Handle out-of-bounds cases
    // Lower than lowest datapoint
    if (adc <= adc_table[0])
        return mv_table[0];

    // Higher than highest datapoint
    if (adc >= adc_table[N - 1])
        return mv_table[N - 1];

    // Linear interpolation using to-punkts bestemmelse
    for (int i = 0; i < N - 1; i++)
    {
        // Check if adc is between two datapoints
        if (adc >= adc_table[i] && adc <= adc_table[i + 1])
        {
            // to-punkts bestemmelse
            float slope = (float)(mv_table[i + 1] - mv_table[i]) /
                          (float)(adc_table[i + 1] - adc_table[i]);

            return mv_table[i] + slope * (adc - adc_table[i]);
        }
    }

    return 0;
}

uint Volt::moving_avg_ADC()
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