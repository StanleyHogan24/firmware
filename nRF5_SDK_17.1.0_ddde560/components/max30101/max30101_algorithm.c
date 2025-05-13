// max30101_algorithm.c

#include "max30101_algorithm.h"
#include "nrf_log.h"
#include <math.h>
#include <string.h>

/**
 * @brief Initialize the MAX30101 algorithm.
 *
 * @return ret_code_t NRF_SUCCESS on success, error code otherwise.
 */
ret_code_t max30101_algorithm_init(void)
{
    // Initialize any algorithm-specific variables or state here
    NRF_LOG_INFO("MAX30101 Algorithm initialized.");
    return NRF_SUCCESS;
}

/**
 * @brief Calculate heart rate and SpO2 from raw sensor data.
 *
 * @param red_led_data Pointer to an array of raw red LED data samples.
 * @param ir_led_data Pointer to an array of raw IR LED data samples.
 * @param data_length Number of samples in the data arrays.
 * @param sampling_rate Sampling rate in Hz.
 * @param hr Pointer to a float where the calculated heart rate will be stored.
 * @param spo2 Pointer to a float where the calculated SpO2 will be stored.
 * @return ret_code_t NRF_SUCCESS on success, error code otherwise.
 */
ret_code_t calculate_hr_and_spo2(uint32_t *red_led_data, uint32_t *ir_led_data, uint32_t data_length, uint32_t sampling_rate, float *hr, float *spo2)
{
    if (data_length == 0 || red_led_data == NULL || ir_led_data == NULL || hr == NULL || spo2 == NULL)
    {
        return NRF_ERROR_NULL;
    }

    // Remove DC component (mean value) from the data
    uint32_t red_dc = 0;
    uint32_t ir_dc = 0;
    for (uint32_t i = 0; i < data_length; i++)
    {
        red_dc += red_led_data[i];
        ir_dc += ir_led_data[i];
    }
    red_dc /= data_length;
    ir_dc /= data_length;

    // Calculate AC component (detrended data) using stack allocation
    int32_t red_ac[data_length];
    int32_t ir_ac[data_length];
    for (uint32_t i = 0; i < data_length; i++)
    {
        red_ac[i] = red_led_data[i] - red_dc;
        ir_ac[i] = ir_led_data[i] - ir_dc;
    }

    // Find peaks in the IR data to calculate heart rate
    uint32_t peak_count = 0;
    uint32_t last_peak = 0;
    uint32_t min_peak_distance = sampling_rate / 2; // Minimum 0.5 seconds between peaks

    for (uint32_t i = 1; i < data_length - 1; i++)
    {
        // Simple peak detection
        if (ir_ac[i] > ir_ac[i - 1] && ir_ac[i] > ir_ac[i + 1] && ir_ac[i] > (int32_t)(0.5 * ir_dc)) // Threshold to avoid noise
        {
            if ((i - last_peak) > min_peak_distance)
            {
                peak_count++;
                last_peak = i;
                if (peak_count >= 10) // Limit peak count to prevent overflow
                {
                    break;
                }
            }
        }
    }

    if (peak_count == 0)
    {
        *hr = 0.0f;
    }
    else
    {
        // Calculate heart rate (bpm)
        float duration_in_seconds = (float)data_length / sampling_rate;
        *hr = (float)(peak_count / duration_in_seconds);
    }

    // Calculate SpO2
    // AC component (peak-to-peak amplitude)
    int32_t red_ac_max = red_ac[0];
    int32_t red_ac_min = red_ac[0];
    int32_t ir_ac_max = ir_ac[0];
    int32_t ir_ac_min = ir_ac[0];
    for (uint32_t i = 1; i < data_length; i++)
    {
        if (red_ac[i] > red_ac_max)
            red_ac_max = red_ac[i];
        if (red_ac[i] < red_ac_min)
            red_ac_min = red_ac[i];
        if (ir_ac[i] > ir_ac_max)
            ir_ac_max = ir_ac[i];
        if (ir_ac[i] < ir_ac_min)
            ir_ac_min = ir_ac[i];
    }
    int32_t red_ac_pp = red_ac_max - red_ac_min;
    int32_t ir_ac_pp = ir_ac_max - ir_ac_min;

    if (ir_ac_pp == 0)
    {
        *spo2 = 0.0f; // Prevent division by zero
    }
    else
    {
        // Calculate ratio
        float ratio = ((float)red_ac_pp / red_dc) / ((float)ir_ac_pp / ir_dc);

        // Empirical formula to calculate SpO2
        *spo2 = 104.0f - 17.0f * ratio;

        // Clamp SpO2 to physiological range
        if (*spo2 > 100.0f)
            *spo2 = 100.0f;
        if (*spo2 < 70.0f)
            *spo2 = 70.0f;
    }

    return NRF_SUCCESS;
}
