#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "nrf_drv_twi.h"
#include "nrf_delay.h"
#include "max30101.h"
#include "common_twi.h"
#include "nrf_log.h"

//---------------------------------------------------------------------------
// TWI instance and transfer completion flag provided by common_twi module
extern const nrf_drv_twi_t m_twi;
extern volatile bool m_xfer_done;
extern void twi_handler(nrf_drv_twi_evt_t const * p_event, void * p_context);

//---------------------------------------------------------------------------
// TWI Master Initialization
void twi_master_init(void)
{
    ret_code_t err_code;
    const nrf_drv_twi_config_t twi_config = {
        .scl                = TWI_SCL_PIN,
        .sda                = TWI_SDA_PIN,
        .frequency          = NRF_DRV_TWI_FREQ_400K,
        .interrupt_priority = APP_IRQ_PRIORITY_HIGH,
        .clear_bus_init     = true
    };

    err_code = nrf_drv_twi_init(&m_twi, &twi_config, twi_handler, NULL);
    APP_ERROR_CHECK(err_code);
    nrf_drv_twi_enable(&m_twi);
}

//---------------------------------------------------------------------------
// MAX30101 Register Write
bool max30101_register_write(uint8_t register_address, uint8_t value)
{
    ret_code_t err_code;
    uint8_t tx_buf[2] = { register_address, value };

    m_xfer_done = false;
    err_code = nrf_drv_twi_tx(&m_twi, MAX30101_I2C_ADDRESS, tx_buf, sizeof(tx_buf), false);
    while (!m_xfer_done) {}

    return (err_code == NRF_SUCCESS);
}

//---------------------------------------------------------------------------
// MAX30101 Register Read
bool max30101_register_read(uint8_t register_address, uint8_t *destination, uint8_t number_of_bytes)
{
    ret_code_t err_code;

    m_xfer_done = false;
    err_code = nrf_drv_twi_tx(&m_twi, MAX30101_I2C_ADDRESS, &register_address, 1, true);
    while (!m_xfer_done) {}
    if (err_code != NRF_SUCCESS) return false;

    m_xfer_done = false;
    err_code = nrf_drv_twi_rx(&m_twi, MAX30101_I2C_ADDRESS, destination, number_of_bytes);
    while (!m_xfer_done) {}

    return (err_code == NRF_SUCCESS);
}

//---------------------------------------------------------------------------
// MAX30101 Initialization with multi-LED support
bool max30101_init(void)
{
    bool success = true;

    // Software reset and delay
    success &= max30101_register_write(MAX30101_MODE_CONFIG, MAX30101_MODE_RESET);
    nrf_delay_ms(100);

    // Reset FIFO pointers
    success &= max30101_register_write(MAX30101_FIFO_WR_PTR, 0x00);
    success &= max30101_register_write(MAX30101_OVF_COUNTER, 0x00);
    success &= max30101_register_write(MAX30101_FIFO_RD_PTR, 0x00);

    // Configure FIFO: no averaging, rollover enabled, almost-full threshold
    success &= max30101_register_write(MAX30101_FIFO_CONFIG, MAX30101_FIFO_CFG);

    // Enable multi-LED mode
    success &= max30101_register_write(MAX30101_MODE_CONFIG, MAX30101_MODE_MULTI_LED);

    // Configure SpO2 settings: LED pulse width, sample rate, ADC range
    success &= max30101_register_write(MAX30101_SPO2_CONFIG, MAX30101_SPO2_CFG);

    // Set LED drive currents: Red, IR, Green, Ambient
    success &= max30101_register_write(MAX30101_LED1_PA, MAX30101_LED1_PA_VAL);
    success &= max30101_register_write(MAX30101_LED2_PA, MAX30101_LED2_PA_VAL);
    success &= max30101_register_write(MAX30101_LED3_PA, MAX30101_LED3_PA_VAL);
    success &= max30101_register_write(MAX30101_LED4_PA, MAX30101_LED4_PA_VAL);

    // Configure LED slot assignments: RED->slot1, IR->slot2, GREEN->slot3, NONE->slot4
    uint8_t slot_cfg1 = (SLOT_RED_LED   << 0) | (SLOT_IR_LED    << 4);
    uint8_t slot_cfg2 = (SLOT_GREEN_LED << 0) | (SLOT_NONE      << 4);
    success &= max30101_register_write(MAX30101_REG_MULTI_LED_MODE1, slot_cfg1);
    success &= max30101_register_write(MAX30101_REG_MULTI_LED_MODE2, slot_cfg2);

    return success;
}

//---------------------------------------------------------------------------
// Read FIFO for three LED channels: Red, IR, Green
bool max30101_read_fifo(uint32_t *green_led, uint32_t *red_led, uint32_t *ir_led)
{
    // 3 bytes per slot x 3 active slots = 9 bytes
    uint8_t fifo_data[9];
    if (!max30101_register_read(MAX30101_FIFO_DATA, fifo_data, sizeof(fifo_data))) {
        return false;
    }

    // Combine bytes: slot1=RED, slot2=IR, slot3=GREEN
    *red_led   = ((uint32_t)fifo_data[0] << 16) | ((uint32_t)fifo_data[1] << 8)  | fifo_data[2];
    *ir_led    = ((uint32_t)fifo_data[3] << 16) | ((uint32_t)fifo_data[4] << 8)  | fifo_data[5];
    *green_led = ((uint32_t)fifo_data[6] << 16) | ((uint32_t)fifo_data[7] << 8)  | fifo_data[8];

    return true;
}

//---------------------------------------------------------------------------
// Reset FIFO pointers and overflow counter
bool max30101_reset_fifo(void)
{
    bool success = true;
    success &= max30101_register_write(MAX30101_FIFO_WR_PTR, 0x00);
    success &= max30101_register_write(MAX30101_OVF_COUNTER, 0x00);
    success &= max30101_register_write(MAX30101_FIFO_RD_PTR, 0x00);
    return success;
}

//---------------------------------------------------------------------------
// Dynamically set LED pulse amplitudes for any channel
bool max30101_set_led_pa(uint8_t led_number, uint8_t pa_value)
{
    if (led_number < 1 || led_number > 4) {
        NRF_LOG_ERROR("Invalid LED number %d", led_number);
        return false;
    }

    uint8_t reg_addr;
    switch (led_number) {
        case 1: reg_addr = MAX30101_LED1_PA; break;
        case 2: reg_addr = MAX30101_LED2_PA; break;
        case 3: reg_addr = MAX30101_LED3_PA; break;
        case 4: reg_addr = MAX30101_LED4_PA; break;
        default: return false;
    }

    return max30101_register_write(reg_addr, pa_value);
}
