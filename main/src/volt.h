#ifndef VOLT_H
#define VOLT_H

#include <Arduino.h>

/**
 * @class Volt
 * Implements functionality to read voltage from ADC pin
 * and calculating source voltage from voltagedivider circuit
 *
 * Example usage:
 * @code
 * Volt battery(7, 1100, 10000.0, 10000.0, ADC_11db, 12);
 * battery.set_sampling(20, 20);
 * battery.read_battery_voltage_mV();
 * @endcode
 */
class Volt
{
public:
    /**
     * @brief Constructor for the Volt class
     * @param pin GPIOpin connected to voltage source, make sure pin has an ADC_Channel.
     * @param R1  Resistor value in voltagedivider
     * @param R2  The resistor value the voltage is measured across
     */
    Volt(int pin, float R1, float R2,
         adc_attenuation_t atten = ADC_11db,
         int resolution = 12);

    /**
     * @brief begin method for Volt class
     * Configures the ESP32 to recieve voltage on _pin.
     * Executes:
     * @code
     * analogReadResolution(_resolution);
     * analogSetPinAttenuation(_pin, (adc_attenuation_t)_atten);
     * @endcode
     * @note Must be called before taking any readings.
     */
    void begin();
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
     * @endcode
     * @return average ADC value
     */
    int avg_ADC();
    /**
     * @brief read_battery_voltage_mv method for Volt class
     * gets the voltage from API analogReadMilliVolts()
     * @return voltage in mV
     */
    float read_voltage_mV();
    /**
     * @brief read_battery_voltage_mv method for Volt class
     * gets the voltage from API analogReadMilliVolts()
     * and divides with divFactor based on R1 and R2
     * Such that you get the voltage from the bouy battery
     * @return voltage in mV
     */
    float read_battery_voltage_mV();

private:
    int _pin;
    int _resolution;
    // Arduino API uses adc_attenuation_t
    // Where ESP_IDF uses adc_atten_t these cannot be mixed
    adc_attenuation_t _atten = ADC_11db;
    int _samples;
    int _tid_m_samples;
    float _R1;
    float _R2;
    float _divFactor;
};

#endif