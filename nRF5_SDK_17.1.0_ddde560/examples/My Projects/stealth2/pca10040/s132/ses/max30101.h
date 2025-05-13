#ifndef MAX30101_H
#define MAX30101_H

#include "nrfx_twim.h"   // Include the TWI master driver
#include "app_error.h"    // For error handling
#include "nrf_log.h"      // For logging

// Define MAX30101 I2C address and register addresses
#define MAX30101_I2C_ADDRESS      0xAE  // MAX30101 I2C address (shifted by 1 bit)
#define MAX30101_REG_INTR_STATUS  0x00  // Interrupt status register
#define MAX30101_REG_FIFO_DATA    0x03  // FIFO data register
#define MAX30101_REG_MODE_CFG     0x06  // Mode configuration register
#define MAX30101_REG_SPO2_CFG     0x07  // SPO2 configuration register
#define MAX30101_REG_LED1_PA      0x09  // LED1 pulse amplitude register

// Define default configuration values (example)
#define MAX30101_MODE_ACTIVE      0x03  // Active mode for MAX30101
#define MAX30101_SPO2_CFG_VALUE   0x27  // SPO2 config value
#define MAX30101_LED1_PA_VALUE    0x24  // LED pulse amplitude for LED1

// Function prototypes
void max30101_init(nrfx_twim_t * p_twim, uint8_t twim_address);
void max30101_write_register(nrfx_twim_t * p_twim, uint8_t reg_addr, uint8_t value);
void max30101_read_register(nrfx_twim_t * p_twim, uint8_t reg_addr, uint8_t * p_value);
void max30101_configure_led(nrfx_twim_t * p_twim);
void max30101_start_reading(nrfx_twim_t * p_twim);

#endif // MAX30101_H
