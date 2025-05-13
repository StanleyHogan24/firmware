#include "bluetooth.h"
#include "nrf_sdh.h"
#include "nrf_sdh_ble.h"
#include "ble_nus.h"
#include "app_error.h"
#include "nrf_log.h"
#include "nrf_ble_gatt.h"
#include "nrf_ble_qwr.h"
#include "ble_srv_common.h"
#include "ble_advertising.h"



// Define the BLE connection configuration tag
#define APP_BLE_CONN_CFG_TAG 1

// Nordic UART Service instance
BLE_NUS_DEF(m_nus, NRF_SDH_BLE_TOTAL_LINK_COUNT);

// BLE event handler
static void ble_evt_handler(ble_evt_t const *p_ble_evt, void *p_context) {
    switch (p_ble_evt->header.evt_id) {
        case BLE_GAP_EVT_CONNECTED:
            NRF_LOG_INFO("Connected to device");
            break;

        case BLE_GAP_EVT_DISCONNECTED:
            NRF_LOG_INFO("Disconnected from device");
            break;

        default:
            break;
    }
}

// Initialize the BLE stack
static void ble_stack_init(void) {
    ret_code_t err_code;

    // Enable SoftDevice
    err_code = nrf_sdh_enable_request();
    APP_ERROR_CHECK(err_code);

    // Configure BLE stack using default settings
    uint32_t ram_start = 0;
    err_code = nrf_sdh_ble_default_cfg_set(APP_BLE_CONN_CFG_TAG, &ram_start);
    APP_ERROR_CHECK(err_code);

    // Enable BLE stack
    err_code = nrf_sdh_ble_enable(&ram_start);
    APP_ERROR_CHECK(err_code);
}


static void gap_params_init(void) {
    ret_code_t err_code;
    ble_gap_conn_params_t gap_conn_params;
    ble_gap_conn_sec_mode_t sec_mode;

    // Set the security mode to open
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);

    // Set the device name
    err_code = sd_ble_gap_device_name_set(&sec_mode, (const uint8_t *)"stHealth_Tracker", strlen("stHealth_Tracker"));
    APP_ERROR_CHECK(err_code);

    // Configure GAP connection parameters
    memset(&gap_conn_params, 0, sizeof(gap_conn_params));
    gap_conn_params.min_conn_interval = MSEC_TO_UNITS(100, UNIT_1_25_MS);  // 100 ms
    gap_conn_params.max_conn_interval = MSEC_TO_UNITS(200, UNIT_1_25_MS);  // 200 ms
    gap_conn_params.slave_latency     = 0;
    gap_conn_params.conn_sup_timeout  = MSEC_TO_UNITS(4000, UNIT_10_MS);   // 4 seconds

    err_code = sd_ble_gap_ppcp_set(&gap_conn_params);
    APP_ERROR_CHECK(err_code);
}

// Advertising module instance
BLE_ADVERTISING_DEF(m_advertising); 

static void advertising_init(void) {
    ret_code_t err_code;
    ble_advertising_init_t init;

    memset(&init, 0, sizeof(init));

    init.advdata.name_type          = BLE_ADVDATA_FULL_NAME;
    init.advdata.include_appearance = false;
    init.advdata.flags              = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;

    init.config.ble_adv_fast_enabled  = true;
    init.config.ble_adv_fast_interval = MSEC_TO_UNITS(40, UNIT_0_625_MS);  // 40 ms
    init.config.ble_adv_fast_timeout  = 0;  // Never stop advertising

    err_code = ble_advertising_init(&m_advertising, &init);
    APP_ERROR_CHECK(err_code);

    ble_advertising_conn_cfg_tag_set(&m_advertising, APP_BLE_CONN_CFG_TAG);
}

void advertising_start(void) {
    ret_code_t err_code = ble_advertising_start(&m_advertising, BLE_ADV_MODE_FAST);
    APP_ERROR_CHECK(err_code);
    NRF_LOG_INFO("Advertising started.");
}

// Initialize Bluetooth (Public API for initializing Bluetooth)
void bluetooth_init(void) {
    ble_stack_init(); // Initialize the BLE stack
    gap_params_init();
    advertising_init();
    advertising_start();
    NRF_LOG_INFO("Bluetooth initialized.");

}

    // Additional Bluetooth setup steps can go here, e.g., initializing services
    // For example, you can call functions to initialize the Nordic UART Service (NUS):
    // nus_init(); or other service initialization code if needed.


