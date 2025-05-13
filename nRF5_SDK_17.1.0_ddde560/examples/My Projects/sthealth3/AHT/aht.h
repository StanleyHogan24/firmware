#ifndef AHT_H
#define AHT_H

#include <stdbool.h>

/**
 * @brief Initialize the AHT10 sensor.
 *
 * @return true if initialization is successful, false otherwise.
 */
bool aht_init(void);

/**
 * @brief Read the temperature from the AHT10 sensor.
 *
 * @param[out] temperature Pointer to a float variable that receives the temperature in °C.
 * @return true if the reading is successful, false otherwise.
 */
bool aht_read_temperature(float *temperature);

#endif // AHT_H
