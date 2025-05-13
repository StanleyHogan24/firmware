 #include <stdint.h>
#include <stdbool.h>
#include "nrf_sdh.h"
#include "nrf_sdh_ble.h"
#include "nrf_ble_gatt.h"
#include "app_error.h"
#include "nrf_log.h"
#include "nrf_twim.h"
#include "ble_advdata.h"
#include "ble_srv_common.h"
#include "ble_advertising.h"
#include "nrf_gpio.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "nrf_delay.h"

// Define BLE connection configuration tag
#define APP_BLE_CONN_CFG_TAG 1
#define DEVICE_NAME "HeartRateMonitor"  // BLE device name

// MAX30101 I2C configuration
#define MAX30101_SCL_PIN 27
#define MAX30101_SDA_PIN 26
#define MAX30101_ADDRESS 0x57  // I2C address of the MAX30101

// TWIM instance
static NRF_TWIM_Type *twim_instance = NRF_TWIM0;

// BLE Heart Rate Service instance
BLE_HRS_DEF(m_hrs);  // Heart Rate Service instance

// Function prototypes
void max30101_init(void);
uint16_t max30101_get_heart_rate(void);
void ble_stack_init(void);
void gatt_init(void);
void advertising_init(void);
void advertising_start(void);
void on_ble_evt(ble_evt_t const *p_ble_evt, void *p_context);
void ble_event_handler_register(void);

// BLE Event Handler
void on_ble_evt(ble_evt_t const *p_ble_evt, void *p_context) {
    switch (p_ble_evt->header.evt_id) {
        case BLE_GAP_EVT_CONNECTED:
            NRF_LOG_INFO("Connected!");
            break;

        case BLE_GAP_EVT_DISCONNECTED:
            NRF_LOG_INFO("Disconnected!");
            break;

        default:
            break;
    }
}

// Register the BLE event handler
void ble_event_handler_register(void) {
    ret_code_t err_code = nrf_sdh_ble_evt_handler_set(on_ble_evt, NULL);
    APP_ERROR_CHECK(err_code);
}

// Initialize the BLE stack
void ble_stack_init(void) {
    ret_code_t err_code;

    // Enable SoftDevice
    err_code = nrf_sdh_enable_request();
    APP_ERROR_CHECK(err_code);

    // Configure the BLE stack using default settings
    uint32_t ram_start = 0;
    err_code = nrf_sdh_ble_default_cfg_set(APP_BLE_CONN_CFG_TAG, &ram_start);
    APP_ERROR_CHECK(err_code);

    // Enable BLE stack
    err_code = nrf_sdh_ble_enable(&ram_start);
    APP_ERROR_CHECK(err_code);
}

// Initialize GATT
void gatt_init(void) {
    ret_code_t err_code;
    static nrf_ble_gatt_t m_gatt;

    err_code = nrf_ble_gatt_init(&m_gatt, NULL);
    APP_ERROR_CHECK(err_code);
}


BLE_ADVERTISING_DEF(m_advertising);  // Advertising module instance

void advertising_init(void) {
    ret_code_t err_code;
    ble_advertising_init_t init;

    memset(&init, 0, sizeof(init));

    // Set up advertising data
    init.advdata.name_type = BLE_ADVDATA_FULL_NAME;
    init.advdata.include_appearance = true;
    init.advdata.flags = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;

    // Set up advertising configuration
    init.config.ble_adv_fast_enabled  = true;
    init.config.ble_adv_fast_interval = 64;     // 40 ms advertising interval
    init.config.ble_adv_fast_timeout  = 18000;  // 18 seconds

    init.evt_handler = NULL;  // Optional: Provide a handler for advertising events

    // Initialize advertising
    err_code = ble_advertising_init(&m_advertising, &init);
    APP_ERROR_CHECK(err_code);

    // Set the advertising configuration tag
    ble_advertising_conn_cfg_tag_set(&m_advertising, APP_BLE_CONN_CFG_TAG);
}



// Start advertising
void advertising_start(void) {
    APP_ERROR_CHECK(sd_ble_gap_adv_start(NULL, APP_BLE_CONN_CFG_TAG));
}

// MAX30101 initialization
void max30101_init(void) {
    nrf_gpio_cfg_output(MAX30101_SCL_PIN);
    nrf_gpio_cfg_output(MAX30101_SDA_PIN);

    nrf_twim_pins_set(twim_instance, MAX30101_SDA_PIN, MAX30101_SCL_PIN);
    nrf_twim_frequency_set(twim_instance, NRF_TWIM_FREQ_400K);
    nrf_twim_enable(twim_instance);

    uint8_t mode_config[2] = {0x09, 0x03};  // Enable SpO2 mode
    nrf_twim_address_set(twim_instance, MAX30101_ADDRESS);
    nrf_twim_tx_buffer_set(twim_instance, mode_config, sizeof(mode_config));
    nrf_twim_task_trigger(twim_instance, NRF_TWIM_TASK_STARTTX);
}

// MAX30101 heart rate retrieval
uint16_t max30101_get_heart_rate(void) {
    uint8_t fifo_data[6];
    nrf_twim_address_set(twim_instance, MAX30101_ADDRESS);
    nrf_twim_rx_buffer_set(twim_instance, fifo_data, sizeof(fifo_data));
    nrf_twim_task_trigger(twim_instance, NRF_TWIM_TASK_STARTRX);

    while (!nrf_twim_event_check(twim_instance, NRF_TWIM_EVENT_STOPPED));
    nrf_twim_event_clear(twim_instance, NRF_TWIM_EVENT_STOPPED);

    uint32_t red_led = (fifo_data[0] << 16) | (fifo_data[1] << 8) | fifo_data[2];
    return (red_led / 1000) % 120 + 40;  // Simulated heart rate
}

// Main function
int main(void) {
    // Initialize logging (optional)
    APP_ERROR_CHECK(NRF_LOG_INIT(NULL));
    NRF_LOG_DEFAULT_BACKENDS_INIT();

    // Initialize BLE stack
    NRF_LOG_INFO("Initializing BLE Stack...");
    ble_stack_init();

    // Initialize GATT
    NRF_LOG_INFO("Initializing GATT...");
    gatt_init();

    // Initialize MAX30101
    NRF_LOG_INFO("Initializing MAX30101...");
    max30101_init();

    // Start advertising
    NRF_LOG_INFO("Starting advertising...");
    advertising_start();

    // Main loop
    while (true) {
        uint16_t heart_rate = max30101_get_heart_rate();
        NRF_LOG_INFO("Heart Rate: %d bpm", heart_rate);
        nrf_delay_ms(1000);  // Wait for 1 second before next reading
        NRF_LOG_FLUSH();
    }
}
