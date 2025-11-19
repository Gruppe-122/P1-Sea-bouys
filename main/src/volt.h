#ifndef VOLT_H
#define VOLT_H

#include <Arduino.h>
#include "driver/adc.h"
#include "esp_adc_cal.h"

/**
 * @class Volt
 * @brief Reads voltage from an ADC pin
 *
 * Example usage:
 * @code
 * Volt battery(7, 1100, 10000.0, 10000.0, ADC_ATTEN_DB_11, 12);
 * battery.setSampling(20, 20);
 * battery.readBatteryVoltage();
 * @endcode
 */
class Volt
{
public:
    /**
     * @brief Constructor for the Volt class
     * @param pin GPIOpin connected to voltage source, make sure pin has an ADC_Channel.
     */
    Volt(int pin, int vRef, float R1, float R2,
         adc_atten_t atten = ADC_ATTEN_DB_11,
         int resolution = 12);

    /**
     * @brief sets the sampling for avg_ADC()
     * @param samples Amount of samples it calculates the average out of
     * @param tid_m_samples Time between samples
     */
    void set_sampling(int samples, int tid_m_samples);
    /**
     * @brief avg_ADC method for Volt class
     * Takes the average ADC value over a specified time with a specified interval
     * @param samples Amount of samples it calculates the average out of
     * @param tid_m_samples
     * @code
     * analogReadResolution(ADC_RESOLUTION);
     * analogSetAttenuation(ADC_11db);
     * esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 1100, &adc_chars);
     * @endcode
     * @return average ADC value
     */
    int avg_ADC();
    /**
     * @brief read_ADC_voltage method for Volt class
     * gets the voltage based on the average ADC signal from esp_adc_cal_raw_to_voltage
     * @return voltage in mV
     */
    float read_ADC_voltage();
    /**
     * @brief read_battery_voltage_mv method for Volt class
     * gets the voltage from analogReadMilliVolts()
     * Seems to be more accurate than read_voltage_mV
     * @return voltage in mV
     */
    float read_voltage_mV();
    /**
     * @brief read_battery_voltage_mv method for Volt class
     * gets the voltage from analogReadMilliVolts()
     * and divides with divFactor based on R1 and R2
     * Such that you get the voltage from the bouy battery
     * @return voltage in mV
     */
    float read_battery_voltage_mV();

private:
    int _pin;
    int _vRef;
    adc_bits_width_t _resolution_ADC = ADC_WIDTH_BIT_12;
    int _resolution;
    adc_atten_t _atten;

    int _samples = 20;
    int _tid_m_samples = 20;
    float _R1;
    float _R2;
    float _divFactor;
    esp_adc_cal_characteristics_t _adcChars;
};

#endif
