//BLE PROGRAMMING FOR BASE FILE GIVEN BY SUMAIR'S EMBEDDED ENGINEERING







#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#include "app_timer.h" //uses RTC0 instance and SoftDevice needs to use RTC1

#include "bsp_btn_ble.h" //initializing LED's and Buttons

#include "nrf_pwr_mgmt.h"

//includes for softdevice

#include "nrf_sdh.h"
#include "nrf_sdh_ble.h"
#include "nrf_sdh_soc.h"

#include "nrf_ble_qwr.h"

#include "nrf_ble_gatt.h"

#include "ble_advdata.h"
#include "ble_advertising.h"

#include "ble_conn_params.h"




//constants

#define APP_BLE_CONN_CFG_TAG    1
#define APP_BLE_OBSERVER_PRIO   3
#define DEVICE_NAME             "test BT"
#define MIN_CONN_INTERVAL       MSEC_TO_UNITS(100, UNIT_1_25_MS)
#define MAX_CONN_INTERVAL       MSEC_TO_UNITS(100, UNIT_1_25_MS)
#define SLAVE_LATENCY           0
#define CONN_SUPP_TIMEOUT       MSEC_TO_UNITS(2000, UNIT_10_MS)


#define FIRST_CONN_PARAMS_UPDATE_DELAY  APP_TIMER_TICKS(5000)
#define NEXT_CONN_PARAMS_UPDATE_DELAY   APP_TIMER_TICKS(30000)
#define MAX_CONN_PARAMS_UPDATE_COUNT    3



#define APP_ADV_INTERVAL        300
#define APP_ADV_DURATION        0






NRF_BLE_QWR_DEF(m_qwr);
NRF_BLE_GATT_DEF(m_gatt);
BLE_ADVERTISING_DEF(m_advertising);


//will tell us of connection 
static uint16_t m_conn_handle = BLE_CONN_HANDLE_INVALID;




//1st Step: Initialize the Logger 

static void log_init()
{
  ret_code_t err_code = NRF_LOG_INIT(NULL);
  APP_ERROR_CHECK(err_code);


  NRF_LOG_DEFAULT_BACKENDS_INIT();

}


//2nd Step: Init App Timer 

static void timer_init(void)
{
  ret_code_t err_code = app_timer_init();
  APP_ERROR_CHECK(err_code);

}


//3rd Step: Board Support Package (LED's) 

static void leds_init(void)
{
  ret_code_t err_code = bsp_init(BSP_INIT_LEDS, NULL)
  APP_ERROR_CHECK(err_code);

}


//4th Step: Power Management (sleep when radio is not used)

static void power_management_init(void)
{
  ret_code_t err_code = nrf_pwr_mgmt_init();
  APP_ERROR_CHECK(err_code);
}


// Step 4.1 ; idle state handle 

static void idle_state_handle(void)
{
  if(NRF_LOG_PROCESS() == false)
  {
    nrf_pwr_mgmt_run();

  }

}


//Step 5.1 BLE Event Handler 

static void ble_evt_handler(ble_evt_t const * p_ble_evt, void * p_context)
{

ret_code_t err_code = NRF_SUCCESS;

switch(p_ble_evt->header.evt_id)
{

  case BLE_GAP_EVT_DISCONNECTED:

  NRF_LOG_INFO("Device is Disconnected!!");

  break;

  case BLE_GAP_EVT_CONNECTED:

  NRF_LOG_INFO("Device is Connected!!");

  err_code = bsp_indication_set(BSP_INDICATE_CONNECTED);
  APP_ERROR_CHECK(err_code);

  m_conn_handle = p_ble_evt->evt.gap_evt.conn_handle;

  err_code = nrf_ble_qwr_assign(&m_qwr, m_conn_handle);
  APP_ERROR_CHECK(err_code);

  break;


//a device that we are communicating with may want to update communiocation parameters. setting this up will allow that change to happen automatically
  case BLE_GAP_EVT_PHY_UPDATE_REQUEST:


  NRF_LOG_DEBUG("PHY Update Request.");

  ble_gap_phys_t const phys = 
  {

    .rx_phys = BLE_GAP_PHY_AUTO,
    .tx_phys = BLE_GAP_PHY_AUTO,


  };

  err_code = sd_ble_gap_phy_update(p_ble_evt->evt.gap_evt.conn_handle, &phys);
  APP_ERROR_CHECK(err_code);


  break;




}



}


//Step 5 Enabling Softdevice or Initializing BLE Stack 

static void ble_stack_init()
{
  ret_code_t err_code;
  
  err_code = nrf_sdh_enable_request();
  APP_ERROR_CHECK(err_code);


  uint32_t ram_start = 0;

  err_code = nrf_sdh_ble_default_cfg_set(APP_BLE_CONN_CFG_TAG, &ram_start);
  APP_ERROR_CHECK(err_code);


  err_code = nrf_sdh_ble_enable(&ram_start);
  APP_ERROR_CHECK(err_code);


  NRF_SDH_BLE_OBSERVER(M_BLE_OBSERVER, APP_BLE_OBSERVER_PRIO, ble_evt_handler, NULL);



}



//STEP 6: gap initialization
static void gap_params_init(void)
{

  ret_code_t err_code;

  ble_gap_conn_params_t gap_conn_params;
  ble_gap_conn_sec_mode_t sec_mode;

  BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);

  err_code = sd_ble_gap_device_name_set(&sec_mode, (const uint8_t *)DEVICE_NAME, strlen(DEVICE_NAME));
  APP_ERROR_CHECK(err_code);

  memset(&gap_conn_params, 0, seizeof(gap_conn_params));

  gap_conn_params.min_conn_interval = MIN_CONN_INTERVAL;
  gap_conn_params.max_conn_interval = MAX_CONN_INTERVAL;
  gap_conn_params.slave_latency = SLAVE_LATENCY;
  gap_conn_params.conn_sup_timeout = CONN_SUPP_TIMEOUT;

  err_code = sd_ble_gap_ppcp_set(&gap_conn_params);
  APP_ERROR_CHECK(err_code);


}


//STEP 7: GATT initialization

static void gatt_init(void)
{

  ret_code_t err_code = nrf_ble_gatt_init(&m_gatt, NULL);
  APP_ERROR_CHECK(err_code);


}

//STEP 8.1: create advertisement event handler
static void on_adv_evt(ble_adv_evt_t ble_adv_evt)
{

  ret_code_t err_code;

  switch (ble_adv_evt)
  {
    case BLE_ADV_EVT_FAST:

    NRF_LOG_INFO("Fast advertising...");
    err_code = bsp_indication_set(BSP_INDICATE_ADVERTISING);
    APP_ERROR_CHECK(err_code);
    break;



    case BLE_ADV_EVT_IDLE:
    err_code = bsp_inidication_set(BSP_INDICATE_IDLE);
    APP_ERROR_CHECK(err_code);
    break;

    default:
    break;

}


//STEP 8: advertising init
static void advertising_init(void)
{
  ret_code_t err_code;

  ble_advertising_init_t init;

  memset(&init, 0, sizeof(init));

  init.advdata.name_type = BLE_ADVDATA_FULL_NAME;

  init.advdata.include_appearance = true;

  init.advdata.flags = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;

  init.config.ble_adv_fast_enabled = true;

  init.config.ble_adv_fast_interval = APP_ADV_INTERVAL;
  init.config.ble_adv_fast_timeout = APP_ADV_DURATION;

  init.evt_handler = on_adv_evt;

  err_code = ble_advertising_init(&m_advertising, &init);
  APP_ERROR_CHECK(err_code);

  ble_advertising_conn_cfg_tag_set(&m_advertising, APP_BLE_CONN_CFG_TAG);

}



//Step 9.1: Error Handler for Query handler

static void nrf_qwr_error_handler(uint32_t nrf_error)
{
APP_ERROR_HANDLER(nrf_error);
}


//Step 9: Intialize the Services

static void services_init(void)
{
  ret_code_t err_code;

  nrf_ble_qwr_init_t qwr_init = {0};

  qwr_init.error_handler = nrf_qwr_error_handler;


  err_code = nrf_ble_qwr_init(&m_qwr, &qwr_init);
  APP_ERROR_CHECK(err_code);



}

//Step 10.1: create an event handler for connection parameters update
static void on_conn_params_evt(ble_conn_params_evt_t * p_evt)
{
  ret_code_t err_code;

  if(p_evt->evt_type == BLE_CONN_PARAMS_EVT_FAILED)
  {
      err_code = sd_ble_gap_disconnect(m_conn_handle, BLE_HCI_CONN_INTERVAL_UNACCEPTABLE);
      APP_ERROR_CHECK(err_code);

  }

  if(p_evt->evt_type == BLE_CONN_PARAMS_EVT_SUCCEEDED)
  {


  }


}






//Step 10.2: create an error handler for conn params update
static void conn_params_error_handler(uint32_t nrf_error)
{
  APP_ERROR_HANDLER(nrf_error);

}








//Step 10: Intialize Connection Parameters

static void conn_params_init(void)

{
  ret_code_t err_code;

  ble_conn_params_init_t cp_init;

  memset(&cp_init, 0, sizeof(cp_init));

  cp_init.p_conn_params     = NULL;

  cp_init.first_conn_params_update_delay = FIRST_CONN_PARAMS_UPDATE_DELAY;

  cp_init.next_conn_params_update_delay = NEXT_CONN_PARAMS_UPDATE_DELAY;

  cp_init.max_conn_params_update_count = MAX_CONN_PARAMS_UPDATE_COUNT;

  cp_init.start_on_notify_cccd_handle = BLE_GATT_HANDLE_INVALID;

  cp_init.disconnect_on_fail          = false;

  cp_init.error_handler = conn_params_error_handler;

  cp_init.evt_handler = on_conn_params_evt;

  err_code = ble_conn_params_init(&cp_init);
  APP_ERROR_CHECK(err_code);



}


//step 11: start advertisement
static void advertising_start(void)
{
  ret_code_t err_code = ble_advertising_start(&m_advertising, BLE_ADV_MODE_FAST);
  APP_ERROR_CHECK(err_code);


}







int main(void)

{


  log_init();

  timers_init();

  leds_init();

  power_management_init();

  ble_stack_init();

  gap_params_init();

  gatt_init();

  advertising_init();

  services_init();

  conn_params_init();

  NRF_LOG_INFO("BLE Base application started...");

  advertising_start();




    //Enter Main Loop
    for(;;)
    {
      idle_state_handle();

    }

}




  