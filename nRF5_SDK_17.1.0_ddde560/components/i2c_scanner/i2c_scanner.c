// i2c_scanner.c

#include "i2c_scanner.h"
#include "nrfx_twi.h"
#include "nrf_delay.h"
#include "nrf_log.h"
#include "app_error.h"

// TWI instance 1, separate from main's instance 0
#define SCANNER_TWI_INSTANCE_ID  1
#define SCANNER_SCL_PIN          27  // Example pins—adjust for your board
#define SCANNER_SDA_PIN          26

static const nrfx_twi_t m_twi_scanner = NRFX_TWI_INSTANCE(SCANNER_TWI_INSTANCE_ID);
static volatile bool m_scanner_busy   = false;

// Local TWI handler
static void i2c_scanner_twi_handler(nrfx_twi_evt_t const * p_event, void * p_context)
{
    if (p_event->type == NRFX_TWI_EVT_DONE)
    {
        m_scanner_busy = false;
    }
    else if (p_event->type == NRFX_TWI_EVT_ADDRESS_NACK || 
             p_event->type == NRFX_TWI_EVT_DATA_NACK)
    {
        m_scanner_busy = false;
    }
    else
    {
        m_scanner_busy = false;
    }
}

void i2c_scanner_init(void)
{
    NRF_LOG_INFO("i2c_scanner_init()");

    // Configure this TWI instance
    nrfx_twi_config_t config = {
        .scl                = SCANNER_SCL_PIN,
        .sda                = SCANNER_SDA_PIN,
        .frequency          = NRF_TWI_FREQ_100K,
        .interrupt_priority = NRFX_TWI_DEFAULT_CONFIG_IRQ_PRIORITY,
        .hold_bus_uninit    = false
    };

    ret_code_t err_code = nrfx_twi_init(&m_twi_scanner, &config, i2c_scanner_twi_handler, NULL);
    APP_ERROR_CHECK(err_code);

    nrfx_twi_enable(&m_twi_scanner);
    NRF_LOG_INFO("I2C Scanner TWI enabled on instance 1.");
}

void i2c_scanner_scan(void)
{
    NRF_LOG_INFO("i2c_scanner_scan() -> scanning 0x08..0x77");

    for (uint8_t addr = 0x08; addr <= 0x77; addr++)
    {
        m_scanner_busy = true;

        // Try a zero-length write. If device ACKs, it will return NRF_SUCCESS
        ret_code_t err_code = nrfx_twi_tx(&m_twi_scanner, addr, NULL, 0, false);
        if (err_code == NRF_SUCCESS)
        {
            // Wait until TWI completes
            while (m_scanner_busy)
            {
                __WFE();
            }
            // If no NACK event, we assume device responded
            NRF_LOG_INFO("I2C device found at: 0x%02X", addr);
        }
        else
        {
            // Even if error, wait for event handler to clear busy
            while (m_scanner_busy)
            {
                __WFE();
            }
        }

        nrf_delay_ms(10); // small gap between addresses
    }

    NRF_LOG_INFO("I2C Scan complete.");
}
