#ifndef HEART_RATE_H__
#define HEART_RATE_H__

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Initialize the heart rate processing module.
 *
 * This function sets up internal variables, buffers, and state (such as counters,
 * filter state, and moving average buffers) needed for the heart rate algorithm.
 */
void heart_rate_init(void);

/**
 * @brief Update the heart rate calculation with a new sensor sample.
 *
 * Processes a new sample (for example, the raw red LED value) and updates the internal
 * heart rate measurement. The algorithm uses filtering, dynamic thresholding, and trough
 * detection (in this version) to compute beats per minute (BPM).
 *
 * @param newSample The latest sensor sample (as a float).
 * @return The current calculated heart rate in BPM.
 */
uint16_t heart_rate_update(float newSample);

/**
 * @brief Retrieve the most recent heart rate measurement.
 *
 * @return The current BPM value.
 */
uint16_t heart_rate_get_bpm(void);

/**
 * @brief (Optional) Apply a bandpass filter to a raw sensor sample.
 *
 * This function can be used to further process the sensor signal.
 * Depending on your configuration, the heart_rate_update() function might already
 * incorporate filtering internally. This is provided for external filtering if needed.
 *
 * @param input The raw sensor sample.
 * @return The filtered sensor sample.
 */

#endif // HEART_RATE_H__
