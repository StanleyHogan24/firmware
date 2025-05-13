#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "nrf_drv_twi.h"
#include "max30101.h"  // Your header file with definitions for MAX30101 registers and constants
#include "common_twi.h"
#include "nrf_log.h"



//*****************************************************************************
// TWI Master Initialization (Used by all I2C devices)
//*****************************************************************************
void twi_master_init(void)
{
    ret_code_t err_code;

    // Configure the settings for TWI communication  
    const nrf_drv_twi_config_t twi_config = {
       .scl                = TWI_SCL_M,               // Define in your board configuration
       .sda                = TWI_SDA_M,               // Define in your board configuration
       .frequency          = NRF_DRV_TWI_FREQ_400K,   // 400KHz communication speed
       .interrupt_priority = APP_IRQ_PRIORITY_HIGH,   // Adjust based on your application
       .clear_bus_init     = true                    // Do not clear bus automatically
    };

    err_code = nrf_drv_twi_init(&m_twi, &twi_config, twi_handler, NULL);
    APP_ERROR_CHECK(err_code);
    nrf_drv_twi_enable(&m_twi);
}

//*****************************************************************************
// MAX30101 Register Write
// Writes a single byte to the specified register address
//*****************************************************************************
bool max30101_register_write(uint8_t register_address, uint8_t value)
{
    ret_code_t err_code;
    // Transmit buffer: register address + data byte
    uint8_t tx_buf[2];
    tx_buf[0] = register_address;
    tx_buf[1] = value;

    m_xfer_done = false;    
    err_code = nrf_drv_twi_tx(&m_twi, MAX30101_I2C_ADDRESS, tx_buf, sizeof(tx_buf), false);
    
    // Wait until transfer completes
    while (m_xfer_done == false) { /* Optionally add timeout */ }

    if (NRF_SUCCESS != err_code)
    {
        return false;
    }
    
    return true;
}

//*****************************************************************************
// MAX30101 Register Read
// Reads number_of_bytes from the specified register into destination buffer
//*****************************************************************************
bool max30101_register_read(uint8_t register_address, uint8_t * destination, uint8_t number_of_bytes)
{
    ret_code_t err_code;

    m_xfer_done = false;
    // First, send the register address (without releasing the bus)
    err_code = nrf_drv_twi_tx(&m_twi, MAX30101_I2C_ADDRESS, &register_address, 1, true);
    while (m_xfer_done == false) { /* Optionally add timeout */ }
    
    if (NRF_SUCCESS != err_code)
    {
        return false;
    }
    
    m_xfer_done = false;
    // Then read the data from that register
    err_code = nrf_drv_twi_rx(&m_twi, MAX30101_I2C_ADDRESS, destination, number_of_bytes);
    while (m_xfer_done == false) { /* Optionally add timeout */ }
    
    if (NRF_SUCCESS != err_code)
    {
        return false;
    }

    return true;
}

//*****************************************************************************
// MAX30101 Initialization
// Configures the sensor registers for SpO2/Heart Rate mode.
//*****************************************************************************
bool max30101_init(void)
{
    bool transfer_succeeded = true;

    // Reset the sensor (if available), clear FIFO pointers etc.
    transfer_succeeded &= max30101_register_write(MAX30101_MODE_CONFIG, MAX30101_RESET);
    // A delay may be needed here after reset (insert delay_ms() call if required)


    // Clear FIFO pointers: write to FIFO_WR_PTR, OVF_COUNTER, and FIFO_RD_PTR if needed
    transfer_succeeded &= max30101_register_write(MAX30101_FIFO_WR_PTR, 0x00);
    transfer_succeeded &= max30101_register_write(MAX30101_OVF_COUNTER, 0x00);
    transfer_succeeded &= max30101_register_write(MAX30101_FIFO_RD_PTR, 0x00);

    // Configure the FIFO: sample averaging, rollover, etc.
    transfer_succeeded &= max30101_register_write(MAX30101_FIFO_CONFIG, MAX30101_FIFO_CFG);

    // Configure the mode: Set to SpO2 mode (or other mode depending on your application)
    transfer_succeeded &= max30101_register_write(MAX30101_MODE_CONFIG, MAX30101_MODE_SPO2);

    // Configure SPO2 settings: ADC range, sample rate, LED pulse width, etc.
    transfer_succeeded &= max30101_register_write(MAX30101_SPO2_CONFIG, MAX30101_SPO2_CFG);

    // Set LED pulse amplitudes
    transfer_succeeded &= max30101_register_write(MAX30101_LED1_PA, MAX30101_LED1_PA_VAL);
    transfer_succeeded &= max30101_register_write(MAX30101_LED2_PA, MAX30101_LED2_PA_VAL);
    // (If your design uses a third LED channel, configure it similarly)

    return transfer_succeeded;
}

//*****************************************************************************
// MAX30101 FIFO Data Read
// Reads one sample (for RED and IR channels) from the FIFO.
// The MAX30101 returns 3 bytes per LED channel. The sample is 18 bits,
// so the upper bits of the 3-byte data are used.
//*****************************************************************************
bool max30101_read_fifo(uint32_t *red_led, uint32_t *ir_led)
{
    // Assume that the FIFO data is organized so that the sample for each LED is returned sequentially.
    // Adjust the reading routine based on your datasheet.
    uint8_t fifo_data[6]; // 2 channels x 3 bytes each (if each sample is 18- or 24-bit)
    
    if(!max30101_register_read(MAX30101_FIFO_DATA, fifo_data, sizeof(fifo_data)))
    {
        return false;
    }
    
    // Example: combine 3 bytes per channel (this depends on your sensor’s data format)
    *red_led = ((uint32_t)fifo_data[0] << 16) | ((uint32_t)fifo_data[1] << 8) | fifo_data[2];
    *ir_led   = ((uint32_t)fifo_data[3] << 16) | ((uint32_t)fifo_data[4] << 8) | fifo_data[5];
    
    return true;
}

/**
 * @brief Reset the FIFO pointer and overflow counter.
 *
 * This function clears the FIFO write pointer, overflow counter, and FIFO read pointer registers,
 * effectively resetting the FIFO state.
 *
 * @return true if the registers were successfully reset; false otherwise.
 */
bool max30101_reset_fifo(void)
{
    bool success = true;
    success &= max30101_register_write(MAX30101_FIFO_WR_PTR, 0x00);
    success &= max30101_register_write(MAX30101_OVF_COUNTER, 0x00);
    success &= max30101_register_write(MAX30101_FIFO_RD_PTR, 0x00);
    return success;
}

bool max30101_set_led_pa(uint8_t led_number, uint8_t pa_value)
{
    // Validate LED number and PA value
    if ((led_number != 1 && led_number != 2) || pa_value > 0x1F)
    {
        NRF_LOG_ERROR("Invalid LED number or PA value.");
        return false;
    }

    uint8_t reg_addr;
    if (led_number == 1)
    {
        reg_addr = MAX30101_LED1_PA;
    }
    else // led_number == 2
    {
        reg_addr = MAX30101_LED2_PA;
    }

    uint8_t data[2];
    data[0] = reg_addr;
    data[1] = pa_value & 0x1F;  // Ensure only lower 5 bits are used

    // Perform I²C write: Register address followed by PA value
    if (!twi_master_transfer(MAX30101_I2C_ADDRESS, data, sizeof(data), true))
    {
        NRF_LOG_ERROR("Failed to set LED%d PA value.", led_number);
        return false;
    }

    NRF_LOG_INFO("LED%d PA set to %d (0x%02X).", led_number, pa_value, pa_value);
    return true;
}
