// max30101_algorithm.h

#ifndef MAX30101_ALGORITHM_H
#define MAX30101_ALGORITHM_H

#include <stdint.h>
#include "app_error.h"

/**
 * @brief Initialize the MAX30101 algorithm.
 *
 * @return ret_code_t NRF_SUCCESS on success, error code otherwise.
 */
ret_code_t max30101_algorithm_init(void);

/**
 * @brief Calculate heart rate (HR) and blood oxygen saturation (SpO2) from raw sensor data.
 *
 * @param red_led_data Pointer to an array of raw red LED data samples.
 * @param ir_led_data Pointer to an array of raw IR LED data samples.
 * @param data_length Number of samples in the data arrays.
 * @param sampling_rate Sampling rate in Hz.
 * @param hr Pointer to a float where the calculated heart rate will be stored.
 * @param spo2 Pointer to a float where the calculated SpO2 will be stored.
 * @return ret_code_t NRF_SUCCESS on success, error code otherwise.
 */
ret_code_t calculate_hr_and_spo2(uint32_t *red_led_data, uint32_t *ir_led_data, uint32_t data_length, uint32_t sampling_rate, float *hr, float *spo2);

#endif // MAX30101_ALGORITHM_H
