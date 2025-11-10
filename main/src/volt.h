#ifndef VOLT_H
#define VOLT_H

#include <Arduino.h>
#include "driver/adc.h"
#include "esp_adc_cal.h"

class Volt {
public:
    Volt(int pin, int vRef, float R1, float R2,
         adc_atten_t atten = ADC_ATTEN_DB_11,
         adc_bits_width_t resolution = ADC_WIDTH_BIT_12);

    void setSampling(int samples, int delayMs);
    float avg_ADC();            // average of N raw ADC reads
    float readAdcVoltage();     // mV measured at divider midpoint
    float readBatteryVoltage(); // scaled V battery in volts

private:
    int _pin;
    int _vRef;
    adc_bits_width_t _resolution;
    adc_atten_t _atten;
    int _samples = 20;
    int _delayMs = 20;
    float _R1;
    float _R2;
    float _divFactor;
    esp_adc_cal_characteristics_t _adcChars;
};

#endif
