#include <stdbool.h>
#include <stdint.h>
#include "nrf.h"
#include "nrf_drv_clock.h"
#include "nrf_sdh.h"
#include "nrf_sdh_ble.h"
#include "nrf_pwr_mgmt.h"
#include "app_timer.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "nrfx_twim.h"

// BLE Configuration
#define APP_BLE_CONN_CFG_TAG 1
#define APP_BLE_OBSERVER_PRIO 3

// TWIM Configuration
#define TWIM_INSTANCE_ID 0
#define I2C_SCL_PIN 27
#define I2C_SDA_PIN 26
#define MAX30101_ADDRESS 0x57  // I2C Address of MAX30101

static const nrfx_twim_t m_twim = NRFX_TWIM_INSTANCE(TWIM_INSTANCE_ID);

// Function Prototypes
void twim_init(void);
void max30101_init(void);
void max30101_read_fifo(uint8_t *data_buffer);

// BLE Event Handler
void ble_evt_handler(ble_evt_t const *p_ble_evt, void *p_context) {
    switch (p_ble_evt->header.evt_id) {
        case BLE_GAP_EVT_CONNECTED:
            NRF_LOG_INFO("Connected.");
            break;

        case BLE_GAP_EVT_DISCONNECTED:
            NRF_LOG_INFO("Disconnected.");
            break;

        default:
            break;
    }
}

// BLE Observer
NRF_SDH_BLE_OBSERVER(my_ble_observer, APP_BLE_OBSERVER_PRIO, ble_evt_handler, NULL);

// TWIM Event Handler
void twim_event_handler(nrfx_twim_evt_t const *p_event, void *p_context) {
    if (p_event->type == NRFX_TWIM_EVT_DONE) {
        NRF_LOG_INFO("TWIM Transfer Complete.");
    }
}

// TWIM Initialization
void twim_init(void) {
    ret_code_t err_code;

    const nrfx_twim_config_t twim_config = {
        .scl                = I2C_SCL_PIN,
        .sda                = I2C_SDA_PIN,
        .frequency          = NRF_TWIM_FREQ_100K,
        .interrupt_priority = APP_IRQ_PRIORITY_LOWEST,
        .hold_bus_uninit    = false
    };

    err_code = nrfx_twim_init(&m_twim, &twim_config, twim_event_handler, NULL);
    APP_ERROR_CHECK(err_code);
    nrfx_twim_enable(&m_twim);

    NRF_LOG_INFO("TWIM Initialized.");
}

// MAX30101 Initialization
void max30101_init(void) {
    ret_code_t err_code;
    uint8_t data[2];

    // Reset the sensor
    data[0] = 0x09;  // MODE_CONFIG register
    data[1] = 0x40;  // Reset command
    err_code = nrfx_twim_tx(&m_twim, MAX30101_ADDRESS, data, 2, false);
    APP_ERROR_CHECK(err_code);
    NRF_LOG_INFO("MAX30101 Reset.");

    nrf_delay_ms(10);  // Wait for reset to complete

    // Set SpO2 configuration
    data[0] = 0x0A;  // SPO2_CONFIG register
    data[1] = 0x03;  // Sample rate = 100Hz, LED pulse width = 1600us
    err_code = nrfx_twim_tx(&m_twim, MAX30101_ADDRESS, data, 2, false);
    APP_ERROR_CHECK(err_code);
    NRF_LOG_INFO("MAX30101 Initialized.");
}

// Read FIFO Data from MAX30101
void max30101_read_fifo(uint8_t *data_buffer) {
    ret_code_t err_code;

    uint8_t reg_address = 0x07;  // FIFO_DATA register
    err_code = nrfx_twim_tx(&m_twim, MAX30101_ADDRESS, &reg_address, 1, true);
    APP_ERROR_CHECK(err_code);

    err_code = nrfx_twim_rx(&m_twim, MAX30101_ADDRESS, data_buffer, 6);
    APP_ERROR_CHECK(err_code);
}

void log_init(void) {
    ret_code_t err_code = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(err_code);
    NRF_LOG_DEFAULT_BACKENDS_INIT();
}

void timers_init(void) {
    ret_code_t err_code = app_timer_init();
    APP_ERROR_CHECK(err_code);
}

void power_management_init(void) {
    ret_code_t err_code = nrf_pwr_mgmt_init();
    APP_ERROR_CHECK(err_code);
}

int main(void) {
    log_init();
    timers_init();
    power_management_init();
    twim_init();
    max30101_init();

    uint8_t fifo_data[6];
    NRF_LOG_INFO("MAX30101 Example Started.");

    while (true) {
        // Read FIFO Data
        max30101_read_fifo(fifo_data);
        uint32_t red_led = (fifo_data[0] << 16) | (fifo_data[1] << 8) | fifo_data[2];
        uint32_t ir_led  = (fifo_data[3] << 16) | (fifo_data[4] << 8) | fifo_data[5];

        NRF_LOG_INFO("RED LED: %d, IR LED: %d", red_led, ir_led);

        nrf_delay_ms(100);  // Sample every 100ms

        if (NRF_LOG_PROCESS() == false) {
            nrf_pwr_mgmt_run();
        }
    }
}
