#include <stdint.h>
#include <string.h>
#include "app_timer.h"
#include "nrf_drv_clock.h"
#include "nrf.h"
#include "bsp.h"
#include "nrf_delay.h"
#include "app_error.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "nrf_pwr_mgmt.h"
#include "nrf_ble_gatt.h"
#include "nrf_ble_qwr.h"

// SoftDevice and BLE includes
#include "nrf_sdh.h"
#include "nrf_sdh_ble.h"
#include "ble_gap.h"
#include "ble_advdata.h"  // Use ble_advdata_encode()
#include "ble_conn_params.h"

#include "heart_rate.h"   // Your heart rate calculation module.
#include "bpm_service.h"  // Custom BPM BLE service.
#include "max30101.h"     // Sensor driver for MAX30101.
#include "accel.h"
#include "steps.h"
#include "gps_uart.h"
#include "gps_nmea.h"



// Application-specific defines
#define APP_BLE_CONN_CFG_TAG 1
#define DEVICE_NAME          "sthealth"    // Advertised device name.
#define MS_PER_SAMPLE        10            // 100 Hz sampling (10 ms per sample).
#define MIN_CONN_INTERVAL    MSEC_TO_UNITS(100, UNIT_1_25_MS)
#define MAX_CONN_INTERVAL    MSEC_TO_UNITS(100, UNIT_1_25_MS)
#define SLAVE_LATENCY        0
#define CONN_SUP_TIMEOUT     MSEC_TO_UNITS(2000, UNIT_10_MS)


NRF_BLE_QWR_DEF(m_qwr);
NRF_BLE_GATT_DEF(m_gatt);

extern void scan_i2c_bus(void);

static ble_bpm_service_t                 m_bpm_service;                             // Instance of our custom BPM service.
static uint16_t                          m_conn_handle = BLE_CONN_HANDLE_INVALID;
static ble_gap_adv_params_t              m_adv_params;                            // Advertising parameters.
static ble_gap_conn_params_t             m_gap_conn_params;
static ble_gap_conn_sec_mode_t           sec_mode;

// Globals for advertising using the new API:
static uint8_t m_adv_handle = BLE_GAP_ADV_SET_HANDLE_NOT_SET;
static uint8_t m_encoded_advdata[BLE_GAP_ADV_SET_DATA_SIZE_MAX];
static uint8_t m_encoded_scanrspdata[BLE_GAP_ADV_SET_DATA_SIZE_MAX];


static void power_management_init(void)
{
  ret_code_t err_code = nrf_pwr_mgmt_init();
  APP_ERROR_CHECK(err_code);
}

// GAP INITIALIZATION
static void gap_params_init(void)
{
    ret_code_t err_code;
    


    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);
    
    err_code = sd_ble_gap_device_name_set(&sec_mode,(const uint8_t *)DEVICE_NAME, strlen(DEVICE_NAME));
    APP_ERROR_CHECK(err_code);

    memset(&m_gap_conn_params, 0, sizeof(m_gap_conn_params));

    m_gap_conn_params.min_conn_interval = MIN_CONN_INTERVAL;
    m_gap_conn_params.max_conn_interval = MAX_CONN_INTERVAL;
    m_gap_conn_params.slave_latency = SLAVE_LATENCY;
    m_gap_conn_params.conn_sup_timeout = CONN_SUP_TIMEOUT;

    err_code = sd_ble_gap_ppcp_set(&m_gap_conn_params);
    APP_ERROR_CHECK(err_code);
}

static void gatt_init(void)
{
  ret_code_t err_code = nrf_ble_gatt_init(&m_gatt, NULL);
  APP_ERROR_CHECK(err_code);

}

//    type used for the Heart Rate service.
uint8_t m_bpm_uuid_type;

// Vendor-specific base UUID registration.
// Replace the base UUID below with one unique to your application.
static void custom_uuid_init(void)
{
    // Use the standard BLE UUID type since the Heart Rate service uses
    // a 16-bit Bluetooth SIG assigned value.
    m_bpm_uuid_type = BLE_UUID_TYPE_BLE;
}

// BLE event handler – updates connection handle and forwards events.
static void ble_evt_handler(ble_evt_t const * p_ble_evt, void * p_context)
{
    ret_code_t err_code;

    switch(p_ble_evt->header.evt_id)
    {
        case BLE_GAP_EVT_CONNECTED:
            NRF_LOG_INFO("Connected.");
            m_conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
            m_bpm_service.conn_handle = m_conn_handle;
            break;

        case BLE_GAP_EVT_DISCONNECTED:
            NRF_LOG_INFO("Disconnected.");
            m_conn_handle = BLE_CONN_HANDLE_INVALID;
            //m_bpm_service.conn_handle = BLE_CONN_HANDLE_INVALID;
            break;

        case BLE_GAP_EVT_PHY_UPDATE_REQUEST:
        {
            NRF_LOG_INFO("PHY update request.");
            ble_gap_phys_t const phys = {
                .rx_phys = BLE_GAP_PHY_AUTO,
                .tx_phys = BLE_GAP_PHY_AUTO,
            };
            err_code = sd_ble_gap_phy_update(p_ble_evt->evt.gap_evt.conn_handle,
                                             &phys);
            APP_ERROR_CHECK(err_code);
        } break;

        case BLE_GAP_EVT_CONN_PARAM_UPDATE_REQUEST:
        {
            ble_gap_conn_params_t const * params =
                &p_ble_evt->evt.gap_evt.params.conn_param_update_request.conn_params;
            NRF_LOG_INFO("Conn param update request: min %u max %u lat %u sup_to %u",
                         params->min_conn_interval,
                         params->max_conn_interval,
                         params->slave_latency,
                         params->conn_sup_timeout);
            err_code = sd_ble_gap_conn_param_update(p_ble_evt->evt.gap_evt.conn_handle,
                                                    params);
            APP_ERROR_CHECK(err_code);
        } break;


        case BLE_GAP_EVT_CONN_PARAM_UPDATE:
        {
            ble_gap_conn_params_t const * params =
                &p_ble_evt->evt.gap_evt.params.conn_param_update.conn_params;
            NRF_LOG_INFO("Conn params updated: min %u max %u lat %u sup_to %u",
                         params->min_conn_interval,
                         params->max_conn_interval,
                         params->slave_latency,
                         params->conn_sup_timeout);
        } break;

        case BLE_GAP_EVT_SEC_PARAMS_REQUEST:
            NRF_LOG_INFO("BLE_GAP_EVT_SEC_PARAMS_REQUEST");
            err_code = sd_ble_gap_sec_params_reply(p_ble_evt->evt.gap_evt.conn_handle,
                                                   BLE_GAP_SEC_STATUS_PAIRING_NOT_SUPP,
                                                   NULL,
                                                   NULL);
            APP_ERROR_CHECK(err_code);
            break;

        default:
            break;
    }

    // Forward the event to the BPM service so it can react to connection and
    // CCCD changes.
    bpm_service_on_ble_evt(&m_bpm_service, p_ble_evt);
}


// Initialize the BLE stack.
static void ble_stack_init(void)
{
    ret_code_t err_code;
    
    err_code = nrf_sdh_enable_request();
    APP_ERROR_CHECK(err_code);
    
    uint32_t ram_start = 0;
    err_code = nrf_sdh_ble_default_cfg_set(APP_BLE_CONN_CFG_TAG, &ram_start);
    APP_ERROR_CHECK(err_code);
    
    err_code = nrf_sdh_ble_enable(&ram_start);
    APP_ERROR_CHECK(err_code);
    
    NRF_SDH_BLE_OBSERVER(m_ble_observer, 3, ble_evt_handler, NULL);
}



// Initialize advertising data and parameters using the new API.
static void advertising_init(void)
{
    ret_code_t err_code;
    
    // Set advertising parameters.
    memset(&m_adv_params, 0, sizeof(m_adv_params));
    m_adv_params.properties.type = BLE_GAP_ADV_TYPE_CONNECTABLE_SCANNABLE_UNDIRECTED;
    m_adv_params.p_peer_addr     = NULL;
    m_adv_params.filter_policy   = BLE_GAP_ADV_FP_ANY;
    m_adv_params.interval        = MSEC_TO_UNITS(100, UNIT_0_625_MS);
    m_adv_params.duration        = 0; // Advertise indefinitely.
    
    
    // Set up advertising data.
    ble_advdata_t advdata;
    memset(&advdata, 0, sizeof(advdata));
    advdata.name_type = BLE_ADVDATA_FULL_NAME;
    advdata.include_appearance = true;
    advdata.flags = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;
    
    
    /* Create an array with your custom service UUID.
    ble_uuid_t adv_uuid;
    adv_uuid.type = m_bpm_uuid_type;
    adv_uuid.uuid = BLE_UUID_BPM_SERVICE;
    ble_uuid_t adv_uuids[] = { adv_uuid };
    advdata.uuids_complete.uuid_cnt = sizeof(adv_uuids) / sizeof(adv_uuids[0]);
    advdata.uuids_complete.p_uuids  = adv_uuids;
    
    uint16_t advdata_len = sizeof(m_encoded_advdata);
    err_code = ble_advdata_encode(&advdata, m_encoded_advdata, &advdata_len);
    APP_ERROR_CHECK(err_code); 
    
    // No scan response data.
    uint16_t scanrsp_len = 0;
    
    // Configure the advertising data structure.
    ble_gap_adv_data_t adv_data;
    memset(&adv_data, 0, sizeof(adv_data));
    adv_data.adv_data.p_data = m_encoded_advdata;
    adv_data.adv_data.len    = advdata_len;
    adv_data.scan_rsp_data.p_data = NULL;
    adv_data.scan_rsp_data.len = 0;

    */
    
    // Configure the advertising set.
    err_code = sd_ble_gap_adv_set_configure(&m_adv_handle, &adv_data, &m_adv_params);
    APP_ERROR_CHECK(err_code);

    err_code = ble_advertising_init(&m_adv_handle, const ble_advertising_init_t *const p_init)
}

// Handle errors from the connection parameters module.
static void conn_params_error_handler(uint32_t nrf_error)
{
    APP_ERROR_HANDLER(nrf_error);
}

// Initialize the connection parameters module to reply to parameter update
// requests from the central and optionally request updates.
static void conn_params_init(void)
{
    ret_code_t err_code;
    ble_conn_params_init_t cp_init;

    memset(&cp_init, 0, sizeof(cp_init));

    cp_init.p_conn_params                  = &m_gap_conn_params;
    cp_init.first_conn_params_update_delay = APP_TIMER_TICKS(5000);
    cp_init.next_conn_params_update_delay  = APP_TIMER_TICKS(30000);
    cp_init.max_conn_params_update_count   = 3;
    cp_init.start_on_notify_cccd_handle    = BLE_GATT_HANDLE_INVALID;
    cp_init.disconnect_on_fail             = false;
    cp_init.evt_handler                    = NULL;
    cp_init.error_handler                  = conn_params_error_handler;

    err_code = ble_conn_params_init(&cp_init);
    APP_ERROR_CHECK(err_code);
}

// Start advertising.
static void advertising_start(void)
{
    ret_code_t err_code;
    err_code = sd_ble_gap_adv_start(m_adv_handle, APP_BLE_CONN_CFG_TAG);
    APP_ERROR_CHECK(err_code);
    NRF_LOG_INFO("Advertising started.");
}



static void test_hfclk(void)
{
    // Check if HFCLK is running from external crystal
    uint32_t hfclk_is_running = 0;
    uint32_t hfclk_src = 0;
    
    // Wait a bit for clock to stabilize after request
    nrf_delay_ms(10);
    
    // Check HFCLK status
    hfclk_is_running = NRF_CLOCK->HFCLKSTAT & CLOCK_HFCLKSTAT_STATE_Msk;
    hfclk_src = NRF_CLOCK->HFCLKSTAT & CLOCK_HFCLKSTAT_SRC_Msk;
    
    if (hfclk_is_running)
    {
        if (hfclk_src == (CLOCK_HFCLKSTAT_SRC_Xtal << CLOCK_HFCLKSTAT_SRC_Pos))
        {
            NRF_LOG_INFO("HFCLK: Running from EXTERNAL CRYSTAL (Good!)");
            // Flash LED to indicate crystal is working
            bsp_board_led_on(0);
            nrf_delay_ms(500);
            bsp_board_led_off(0);
        }
        else
        {
            NRF_LOG_ERROR("HFCLK: Running from INTERNAL RC (Bad for BLE!)");
            NRF_LOG_ERROR("Check your 32MHz crystal and capacitors!");
            // Rapid LED flash to indicate problem
            for (int i = 0; i < 10; i++)
            {
                bsp_board_led_invert(0);
                nrf_delay_ms(100);
            }
        }
    }
    else
    {
        NRF_LOG_ERROR("HFCLK: NOT RUNNING AT ALL!");
    }
    
}

// Handlers for HFCLK and LFCLK start events
static void hfclk_started_handler(nrf_drv_clock_evt_type_t event)
{
    if (event == NRF_DRV_CLOCK_EVT_HFCLK_STARTED)
    {
        NRF_LOG_INFO("HFCLK started successfully");
    }
}

static void lfclk_started_handler(nrf_drv_clock_evt_type_t event)
{
    if (event == NRF_DRV_CLOCK_EVT_LFCLK_STARTED)
    {
        NRF_LOG_INFO("LFCLK started successfully");
    }
}

// Enhanced clock initialization with error checking
static void clock_init_with_verification(void)
{
    ret_code_t err_code;
    
    // Initialize the clock driver
    err_code = nrf_drv_clock_init();
    if (err_code != NRF_SUCCESS && err_code != NRF_ERROR_MODULE_ALREADY_INITIALIZED)
    {
        NRF_LOG_ERROR("Clock init failed: 0x%02X", err_code);
        APP_ERROR_CHECK(err_code);
    }
    
    // Request both clocks with callbacks to know when they're ready
    static nrf_drv_clock_handler_item_t hfclk_handler_item = {0};
    static nrf_drv_clock_handler_item_t lfclk_handler_item = {0};
    
    // Assign Callbacks
    hfclk_handler_item.event_handler = hfclk_started_handler;
    lfclk_handler_item.event_handler = lfclk_started_handler;
    
    // Request clocks
    nrf_drv_clock_hfclk_request(&hfclk_handler_item);
    nrf_drv_clock_lfclk_request(&lfclk_handler_item);
    
    // Wait for clocks to start
    while (!nrf_drv_clock_hfclk_is_running() || !nrf_drv_clock_lfclk_is_running())
    {
        // Wait with timeout
        static uint32_t timeout_counter = 0;
        nrf_delay_ms(1);
        timeout_counter++;
        
        if (timeout_counter > 1000) // 1 second timeout
        {
            NRF_LOG_ERROR("Clock startup timeout!");
            if (!nrf_drv_clock_hfclk_is_running())
            {
                NRF_LOG_ERROR("HFCLK failed to start - check 32MHz crystal!");
            }
            if (!nrf_drv_clock_lfclk_is_running())
            {
                NRF_LOG_ERROR("LFCLK failed to start - check 32.768kHz crystal!");
            }
            break;
        }
    }
    
    // Run the crystal test
    test_hfclk();
}



int main(void)
{
    ret_code_t err_code;
    ret_code_t err;

    // Initialize the clock driver for EasyDMA peripherals
    clock_init_with_verification();
    
    // Initialize logging.
    err_code = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(err_code);
    NRF_LOG_DEFAULT_BACKENDS_INIT();

    // Initialize board support (PCA10040).
    bsp_board_init(BSP_INIT_LEDS | BSP_INIT_BUTTONS);

    // Initialize app_timer library.
    err_code = app_timer_init();
    APP_ERROR_CHECK(err_code);

    nrf_pwr_mgmt_init();


    //Initialize BLE stack
    ble_stack_init();

    
    // Initialize vendor-specific UUID.
    //custom_uuid_init();
    
   
    gap_params_init();

    gatt_init();

    advertising_init();
   
    
    // Initialize Heart Rate service (BPM).
    bpm_service_init(&m_bpm_service);

    conn_params_init();
    
    // Start advertising.
    advertising_start();


        // Initialize the shared TWI (I2C) interface used by all sensors.
    twi_master_init();

    // Initialize the scanning of addresses available on I2C bus. 
    //scan_i2c_bus();

   


    
   // MPU-6050 init (0x69 if AD0 is high)
    NRF_LOG_DEBUG("Init MPU-6050 at 0x68...");
    bool ok = mpu6050_init();
    if (!ok)
    {
        NRF_LOG_ERROR("MPU-6050 init NACK or fail");
    }
    else
    {
        steps_init();
        NRF_LOG_DEBUG("MPU-6050 ready");
    }



    // Initialize GPS parsing and transport
    gps_nmea_init();
    NRF_LOG_DEBUG("NMEA parser init returned.");
    gps_uart_init();
    NRF_LOG_DEBUG("gps_uart_init() returned: 0x%08X", err);
    

   

    uint32_t red_led = 0, ir_led = 0, green_led=0;

    

    
    // Initialize Algorithms.
    heart_rate_init();
    steps_init();
    
    // Initialize MAX30101 sensor (retry until successful).
    while (max30101_init() == false)
    {
        NRF_LOG_DEBUG("MAX30101 initialization failed, retrying...");
        nrf_delay_ms(1000);
    }
    NRF_LOG_DEBUG("MAX30101 initialized successfully.");
    
    uint16_t previous_bpm = 0;
    
    // Main loop: read sensor data, update BPM, and send BLE notifications.
    while (true)
    {
        

        steps_update();

        // 1) Read and log raw accel data
        {
        int16_t ax, ay, az;
        if (mpu6050_read_accel(&ax, &ay, &az))
          {
            NRF_LOG_DEBUG("MPU6050 RAW → X:%d  Y:%d  Z:%d", ax, ay, az);
          }
        else
          {
            NRF_LOG_WARNING("MPU6050 read error");
          }
        }



        
        // Read sensor data from MAX30101.
        if (max30101_read_fifo(&green_led, &red_led, &ir_led))
        {
            NRF_LOG_DEBUG("MAX30101 Read: Red=%u", red_led);
            NRF_LOG_DEBUG("MAX30101 Read: IR=%u", ir_led);
            NRF_LOG_DEBUG("MAX30101 Read: Green=%u", green_led);
            
            // For heart rate detection in SpO₂ mode, we now use the red LED sample.
            float raw_green = (float)green_led;
    
            uint16_t bpm = heart_rate_update(raw_green);
            
            static uint16_t previous_bpm = 0;
            if (bpm != previous_bpm && bpm != 0)
            {
                NRF_LOG_DEBUG("Heart Rate (Green LED): %u BPM", bpm);
                previous_bpm = bpm;
               
               

            }
            else
            {
                NRF_LOG_DEBUG("BPM not updated. Current BPM: %u, Previous BPM: %u", bpm, previous_bpm);
            }

          



            if ((bpm != previous_bpm) && (bpm != 0))
            {
                NRF_LOG_DEBUG("Heart Rate: %u BPM", bpm);
                previous_bpm = bpm;
                
                err_code = bpm_service_update(&m_bpm_service, bpm);
                if (err_code != NRF_SUCCESS)
                {
                    NRF_LOG_INFO("BPM update over BLE failed. Error: 0x%08X", err_code);
                }
            }


        }
        else
        {
            NRF_LOG_DEBUG("MAX30101 FIFO read failed!");
        }
        
        // Delay to maintain a 100 Hz sampling rate.
        nrf_delay_ms(MS_PER_SAMPLE);
        NRF_LOG_FLUSH();
    }
}



