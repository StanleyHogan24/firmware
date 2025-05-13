#include "aht.h"
#include "nrf_delay.h"      // For nrf_delay_ms()
#include "common_twi.h"     // Shared TWI interface
#include <stdbool.h>
#include <stdint.h>

#define AHT_I2C_ADDRESS 0x38    // 7-bit I2C address for AHT10

// Timeout/delay values (in milliseconds)
#define AHT_INIT_DELAY_MS      20   // Delay after initialization command
#define AHT_MEASUREMENT_DELAY  80   // Measurement delay (datasheet recommends at least 80ms)

/**
 * @brief Initialize the AHT10 sensor.
 *
 * This function sends the initialization command sequence (0xE1, 0x08, 0x00)
 * required to set up the sensor.
 *
 * @return true if the command was sent successfully, false otherwise.
 */
bool aht_init(void)
{
    // The initialization command for AHT10 is 0xE1 followed by 0x08 and 0x00.
    uint8_t init_cmd[3] = {0xE1, 0x08, 0x00};

    // Send the initialization command over TWI/I²C.
    if (!twi_master_transfer(AHT_I2C_ADDRESS, init_cmd, sizeof(init_cmd), true))
    {
        return false;
    }

    nrf_delay_ms(AHT_INIT_DELAY_MS);
    return true;
}

/**
 * @brief Read the temperature from the AHT10 sensor and output the result in °F.
 *
 * This function triggers a measurement by sending the command (0xAC, 0x33, 0x00),
 * waits for the measurement to complete, reads 6 bytes from the sensor,
 * and then extracts and converts the temperature value.
 *
 * The conversion process is as follows:
 *   1. Convert raw temperature to Celsius:
 *      Temperature (°C) = (temp_raw / (2^20 - 1)) * 200 - 50
 *   2. Convert Celsius to Fahrenheit:
 *      Temperature (°F) = Temperature (°C) * 9/5 + 32
 *
 * @param[out] temperature Pointer to a float variable that will hold the temperature in °F.
 * @return true if the measurement was successfully read and converted, false otherwise.
 */
bool aht_read_temperature(float *temperature)
{
    if (temperature == NULL)
    {
        return false;
    }
    
    // The measurement command: 0xAC, 0x33, 0x00
    uint8_t meas_cmd[3] = {0xAC, 0x33, 0x00};

    // Send the measurement command.
    if (!twi_master_transfer(AHT_I2C_ADDRESS, meas_cmd, sizeof(meas_cmd), true))
    {
        return false;
    }

    // Wait for the measurement to complete.
    nrf_delay_ms(AHT_MEASUREMENT_DELAY);

    // Read 6 bytes of measurement data.
    uint8_t data[6] = {0};
    if (!twi_master_transfer(AHT_I2C_ADDRESS, data, sizeof(data), false))
    {
        return false;
    }

    // The sensor returns 6 bytes:
    // data[0] : status
    // data[1..2] : 20-bit humidity data (not used here)
    // data[3..5] : 20-bit temperature data:
    //   - Bits 19:16 are in the lower nibble of data[3] (data[3] & 0x0F)
    //   - Bits 15:8 are in data[4]
    //   - Bits 7:0 are in data[5]
    uint32_t temp_raw = (((uint32_t)data[3] & 0x0F) << 16) | (((uint32_t)data[4]) << 8) | data[5];

    // Convert the raw temperature to Celsius:
    // Temperature (°C) = (temp_raw / (2^20 - 1)) * 200 - 50
    float temp_c = ((float)temp_raw * 200.0f / 1048575.0f) - 50.0f;

    // Convert Celsius to Fahrenheit: Temperature (°F) = Temperature (°C) * 9/5 + 32
    *temperature = (temp_c * 9.0f / 5.0f) + 32.0f;

    return true;
}
