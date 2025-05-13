#include "max30101.h"

// Function to initialize the MAX30101 sensor
void max30101_init(nrfx_twim_t * p_twim, uint8_t twim_address)
{
    // Reset and configure the MAX30101 to default settings
    max30101_write_register(p_twim, MAX30101_REG_MODE_CFG, MAX30101_MODE_ACTIVE);
    max30101_write_register(p_twim, MAX30101_REG_SPO2_CFG, MAX30101_SPO2_CFG_VALUE);
    max30101_write_register(p_twim, MAX30101_REG_LED1_PA, MAX30101_LED1_PA_VALUE);

    // Configure additional registers if needed (e.g., LED configuration, sample rate, etc.)
    max30101_configure_led(p_twim);
}

// Function to write to a MAX30101 register
void max30101_write_register(nrfx_twim_t * p_twim, uint8_t reg_addr, uint8_t value)
{
    ret_code_t err_code;
    uint8_t data[2] = { reg_addr, value }; // Register address + value to write

    err_code = nrfx_twim_tx(p_twim, MAX30101_I2C_ADDRESS, data, sizeof(data), false);
    if (err_code != NRF_SUCCESS)
    {
        NRF_LOG_ERROR("Error writing to MAX30101 register: %d", err_code);
        APP_ERROR_CHECK(err_code); // Handle error properly
    }
}

// Function to read from a MAX30101 register
void max30101_read_register(nrfx_twim_t * p_twim, uint8_t reg_addr, uint8_t * p_value)
{
    ret_code_t err_code;

    // Send the register address to read from
    err_code = nrfx_twim_tx(p_twim, MAX30101_I2C_ADDRESS, &reg_addr, 1, true);
    if (err_code != NRF_SUCCESS)
    {
        NRF_LOG_ERROR("Error sending register address: %d", err_code);
        APP_ERROR_CHECK(err_code);
    }

    // Read the value from the register
    err_code = nrfx_twim_rx(p_twim, MAX30101_I2C_ADDRESS, p_value, 1);
    if (err_code != NRF_SUCCESS)
    {
        NRF_LOG_ERROR("Error reading from MAX30101 register: %d", err_code);
        APP_ERROR_CHECK(err_code);
    }
}

// Function to configure the LED settings for the MAX30101 sensor
void max30101_configure_led(nrfx_twim_t * p_twim)
{
    // Example: Configure LED1 and LED2 pulse amplitude, depending on your application
    // We assume LED1 is configured in `max30101_init`
}

// Function to start reading sensor data from MAX30101
void max30101_start_reading(nrfx_twim_t * p_twim)
{
    uint8_t data[6]; // FIFO data for example
    ret_code_t err_code;

    // Read data from the FIFO register (example)
    err_code = nrfx_twim_rx(p_twim, MAX30101_I2C_ADDRESS, data, sizeof(data));
    if (err_code != NRF_SUCCESS)
    {
        NRF_LOG_ERROR("Error reading data from MAX30101: %d", err_code);
        APP_ERROR_CHECK(err_code);
    }

    // Process the sensor data (heart rate, SpO2, etc.)
    NRF_LOG_INFO("Read sensor data: %x, %x, %x", data[0], data[1], data[2]);
}
