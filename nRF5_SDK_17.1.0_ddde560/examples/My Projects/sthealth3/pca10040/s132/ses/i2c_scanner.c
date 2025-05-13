// i2c_scanner.c
#include "i2c_scanner.h"
#include "nrfx_twi.h"
#include "nrf_delay.h"
#include "nrf_log.h"
#include "app_error.h"

// Example TWI instance for the scanner
static const nrfx_twi_t m_twi_scanner = NRFX_TWI_INSTANCE(1); 
// or 0 if it does not conflict with your main TWI

static volatile bool m_scanner_busy = false;

// Optional local handler unique to this file
static void i2c_scanner_twi_handler(nrfx_twi_evt_t const * p_event, void * p_context)
{
    if (p_event->type == NRFX_TWI_EVT_DONE)
    {
        NRF_LOG_DEBUG("Scanner: TWI done");
        m_scanner_busy = false;
    }
    else if (p_event->type == NRFX_TWI_EVT_ADDRESS_NACK || p_event->type == NRFX_TWI_EVT_DATA_NACK)
    {
        NRF_LOG_DEBUG("Scanner: NACK");
        m_scanner_busy = false;
    }
}

// --------------------------------------------------------------------
// 1) i2c_scanner_init
// --------------------------------------------------------------------
void i2c_scanner_init(void)
{
    nrfx_twi_config_t const config = {
        .scl                = 27,  // or your SCL pin
        .sda                = 26,  // or your SDA pin
        .frequency          = NRF_TWI_FREQ_100K,
        .interrupt_priority = NRFX_TWI_DEFAULT_CONFIG_IRQ_PRIORITY,
        .hold_bus_uninit    = false
    };

    ret_code_t err_code = nrfx_twi_init(&m_twi_scanner, &config, i2c_scanner_twi_handler, NULL);
    APP_ERROR_CHECK(err_code);

    nrfx_twi_enable(&m_twi_scanner);

    NRF_LOG_INFO("I2C Scanner initialized (separate).");
}

// --------------------------------------------------------------------
// 2) i2c_scanner_scan
// --------------------------------------------------------------------
void i2c_scanner_scan(void)
{
    NRF_LOG_INFO("I2C scanner: scanning...");

    for (uint8_t addr = 0x08; addr <= 0x77; addr++)
    {
        m_scanner_busy = true;
        // Try a zero-length write. If device ACKs, we consider it found
        ret_code_t err_code = nrfx_twi_tx(&m_twi_scanner, addr, NULL, 0, false);
        if (err_code == NRF_SUCCESS)
        {
            // Wait until TWI completes
            while (m_scanner_busy)
            {
                __WFE();
            }
            // If no NACK event fired, assume it's valid
            NRF_LOG_INFO("Scanner: Found device at 0x%02X", addr);
        }
        else
        {
            // If it's BUSY or something else, we still wait for event
            while (m_scanner_busy)
            {
                __WFE();
            }
        }

        nrf_delay_ms(5);
    }

    NRF_LOG_INFO("I2C scanner: done.");
}
