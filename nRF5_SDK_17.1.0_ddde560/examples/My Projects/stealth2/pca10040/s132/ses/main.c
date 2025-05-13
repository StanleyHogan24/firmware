#include "nrfx_twim.h"
#include "max30101.h"
#include "app_error.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "bsp.h"
#include "app_button.h"



// Define the TWIM instance (using TWIM0 in this case)
static nrfx_twim_t m_twim = NRFX_TWIM_INSTANCE(0);

// Define I2C pins for your hardware (adjust if needed)
#define SDA_PIN 26
#define SCL_PIN 27

// Function to initialize logging
static void log_init(void)
{
    ret_code_t err_code = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(err_code);
    NRF_LOG_DEFAULT_BACKENDS_INIT();
}

// Function to initialize the TWIM (I2C) peripheral
static void twim_init(void)
{
    nrfx_twim_config_t twim_config = NRFX_TWIM_DEFAULT_CONFIG;
    
    // Set the frequency field directly (100kHz, 250kHz, 400kHz, etc.)
    twim_config.frequency = NRF_TWIM_FREQ_400K;  // Set to 400kHz for faster communication

    twim_config.scl = SCL_PIN;  // Set SCL pin
    twim_config.sda = SDA_PIN;  // Set SDA pin

    ret_code_t err_code = nrfx_twim_init(&m_twim, &twim_config, NULL, NULL);
    APP_ERROR_CHECK(err_code);

    nrfx_twim_enable(&m_twim);
}

// Function to initialize the MAX30101 sensor
static void max30101_sensor_init(void)
{
    // Initialize the MAX30101 sensor
    max30101_init(&m_twim, MAX30101_I2C_ADDRESS);
}

// Function to read sensor data and process it
static void read_max30101_data(void)
{
    // Start reading data from MAX30101
    max30101_start_reading(&m_twim);
}

int main(void)
{

    
    // Initialize logging system
    log_init();

    // Initialize the TWIM (I2C) interface
    twim_init();

    // Initialize the MAX30101 sensor
    max30101_sensor_init();



    // Main loop
    while (1)
    {
        // Read and process MAX30101 data
        read_max30101_data();

        // Log data (example, customize based on your data processing)
        NRF_LOG_INFO("MAX30101 data reading...");

        // Process logging if necessary
        NRF_LOG_FLUSH();

        // Enter low-power mode when idle
        __WFE();  // Wait for event (low power)
    }
}



