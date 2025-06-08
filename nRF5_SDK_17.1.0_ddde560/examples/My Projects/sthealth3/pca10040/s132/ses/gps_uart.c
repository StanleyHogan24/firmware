// gps_uart.c
#include "gps_uart.h"
#include "gps_nmea.h"
#include "nrf_log.h"
#include "app_error.h"
#include "nrfx_uart.h"

#define GPS_UART_INSTANCE   0
static const nrfx_uart_t gps_uart = NRFX_UART_INSTANCE(GPS_UART_INSTANCE);
#define GPS_UART_TX_PIN     6   // P0.06: MCU TX -> GPS RX
#define GPS_UART_RX_PIN     8   // P0.08: GPS TX -> MCU RX
#define GPS_UART_BAUDRATE   NRF_UART_BAUDRATE_9600

static uint8_t rx_char;

// UART event handler must match nrfx_uart_event_handler_t signature
static void uart_event_handler(nrfx_uart_event_t const * p_event, void * p_context)
{
    (void)p_context;
    if (p_event->type == NRFX_UART_EVT_RX_DONE)
    {
        // Feed received byte to NMEA parser
        gps_nmea_parse_char(rx_char);
        // Re-arm reception (retry on busy)
        ret_code_t err;
        do {
            err = nrfx_uart_rx(&gps_uart, &rx_char, 1);
        } while (err == NRF_ERROR_BUSY);
        APP_ERROR_CHECK(err);
    }
    else if (p_event->type == NRFX_UART_EVT_ERROR)
    {
        // UART error: log error_mask
        NRF_LOG_ERROR("UART error: mask=0x%08X", p_event->data.error.error_mask);
    }
}


void gps_uart_init(void)
{
    nrfx_uart_config_t config = NRFX_UART_DEFAULT_CONFIG;
    config.pseltxd = GPS_UART_TX_PIN;
    config.pselrxd = GPS_UART_RX_PIN;
    config.baudrate = GPS_UART_BAUDRATE;
    config.hwfc = NRF_UART_HWFC_DISABLED;
    config.parity = NRF_UART_PARITY_EXCLUDED;

    // Initialize UART with interrupt handler
    ret_code_t err = nrfx_uart_init(&gps_uart, &config, uart_event_handler);
    APP_ERROR_CHECK(err);

    // Prime first reception (retry on busy)
    do {
        err = nrfx_uart_rx(&gps_uart, &rx_char, 1);
    } while (err == NRF_ERROR_BUSY);
    APP_ERROR_CHECK(err);
}