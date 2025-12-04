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
 * Volt battery(7, 50000, 10000, 100, ADC_11db, 12);
 * battery.begin();
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
     * @param ma_samples Number of samples to average over in moving average. Currently has no effect
     * @param atten ADC attenuation setting, default ADC_11db. ADC_11db and ADC_12db are equal. Max input is approx 3160mV
     * @param resolution ADC resolution, default 12 bits
     */
    Volt(int pin, float R1, float R2, int ma_samples = 100,
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
     * @note Should be called before taking any readings.
     */
    void begin();
    /**
     * @brief  Calculates moving average of ADC readings. 
     * Excutes only the sampling and averaging once per call.
     * Uses old samples in buffer to calculate moving average.
     * @return averaged ADC value
     */
    uint moving_avg_ADC();
    /**
     * @brief ADC_to_mV function
     * Converts raw ADC value to millivolts using lookup table and linear interpolation
     * Lookup tables are defined in private section
     * Lookup tables based on measurements at 23,7°C with 12dB attenuation and 12 bit resolution
     * @param adc Raw ADC value
     * @return voltage in mV
     */
    float ADC_to_mV(int adc);
    /**
     * @brief Should not be used instead use ADC_to_mV with moving_avg_ADC
     * gets the voltage from API analogReadMilliVolts()
     * @return voltage in mV
     */
    float read_voltage_mV();
    /**
     * @brief Should not be used instead use ADC_to_mV with moving_avg_ADC
     * gets the voltage from API analogReadMilliVolts()
     * and divides with divFactor based on R1 and R2
     * Such that you get the voltage from the bouy battery
     * @return voltage in mV
     */
    float read_battery_voltage_mV();

private:
    int _pin;
    int _resolution;
    int _ma_samples;
    // Moving average buffer statically set to 100 samples.
    int ma_buffer[100];
    int ma_index;
    long ma_sum;
    bool ma_full;
    // Arduino API uses adc_attenuation_t
    // where ESP_IDF uses adc_atten_t these cannot be mixed
    adc_attenuation_t _atten = ADC_11db;
    float _R1;
    float _R2;
    float _divFactor;

    // Lookup tables for ADC to mV conversion
    // measurements done at 23,7°C with 12dB attenuation and 12 bit resolution
    // with Keysight U1241C True RMS Digital Multimeter
    // made static to avoid multiple copies if multiple Volt objects are created
    // measeurements done with power supply with voltagedivider 50k/10k 
    // from 18400mV to 9000mV
    static const int N = 20;
    const uint16_t adc_table[N] = {
    50, 1894, 1996, 2100, 2202, 2303, 2406, 2511, 2615, 2718,
    2827, 2933, 3044, 3159, 3280, 3410, 3540, 3685, 3835, 3975};
    const uint16_t mv_table[N] = {
    38, 1597, 1681, 1764, 1848, 1932, 2016, 2100, 2184, 2268,
    2354, 2438, 2522, 2606, 2690, 2774, 2857, 2941, 3025, 3093};
};

#endif