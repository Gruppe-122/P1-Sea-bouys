#ifndef CURRENT_H
#define CURRENT_H

#include <Arduino.h>

/**
 * @class CurrentSensor
 * @brief Reads current from an ACS712 sensor using the ESP32 ADC.
 *
 * Example usage:
 * @code
 * CurrentSensor currentSensor(19, 2545, 100);
 * currentSensor.begin();
 * float current_mA = currentSensor.measure_current_mA();
 * @endcode
 */

class CurrentSensor
{
public:
    /**
     * @brief Constructor for the CurrentSensor class.
     * @param pin GPIO pin connected to sensor output, make sure PIN has an ADC_Channel.
     * @param dcOffset_mV Offset voltage in millivolts (around 2550 mV for ACS712).
     * @param modSensitivity_mV_per_A Sensitivity of sensor in mV per ampere. Is 185 for 5amp version
     */
    CurrentSensor(int pin, int dcOffset_mV, int modSensitivity_mV_per_A=185, adc_attenuation_t atten=ADC_11db);
    /**
     * @brief begin method for CurrentSensor class
     * It executes the following code
     * @code
     * analogReadResolution(ADC_RESOLUTION);
     * analogSetAttenuation(ADC_11db);
     * esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 1100, &adc_chars);
     * @endcode
     * @note Must be called once before taking any readings.
     */
    void begin();
    /**
     * @brief sets the sampling for avg_ADC()
     * @param samples Amount of samples it calculates the average out of
     * @param tid_m_samples Time between samples
     * @param adc_resolution ADC resolution in bits 12 for ESP32-S3
     */
    void set_sampling(int samples = 20, int tid_m_samples = 20, int adc_resolution = 12);
    /**
     * @brief avg_ADC method for CurrentSensor class
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
    int avg_ADC(int samples, int tid_m_samples);
    /**
     * @brief get_voltage_mV method for CurrentSensor class
     * gets the voltage from API analogReadMilliVolts()
     * @return voltage in mV
     */
    float get_voltage_mV();
    /**
     * @brief Measures the current going through the module
     * @return current in amps
     */
    float measure_current_A();
    /**
     * @brief Measures the current going through the module and multiplies with 1000
     * @return current in milliamps
     */
    float measure_current_mA();

private:
    int _samples;
    int _tid_m_samples;
    int _adc_resolution = 12;
    int _pin;
    int _dcOffset_mV;
    float _mod_mV_per_A;
    // Arduino API uses adc_attenuation_t
    // Where ESP_IDF uses adc_atten_t these cannot be mixed
    adc_attenuation_t _atten = ADC_11db;
};

#endif