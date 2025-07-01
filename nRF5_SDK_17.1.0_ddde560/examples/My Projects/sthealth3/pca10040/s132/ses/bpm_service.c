#include "bpm_service.h"
#include "nrf_log.h"
#include "app_error.h"
#include <string.h>
#include "ble_srv_common.h"
#include "nrf_gpio.h"
#include "boards.h"
#include "nrf_log.h"

// UUID type used for the Heart Rate service. This is set in main.c and will be BLE_UUID_TYPE_BLE for standard 16-bit UUIDs.
extern uint8_t m_bpm_uuid_type;

void bpm_service_init(ble_bpm_service_t * p_bpm_service)
{
    ret_code_t err_code;
    ble_uuid_t service_uuid;
    
    service_uuid.type = m_bpm_uuid_type;
    service_uuid.uuid = BLE_UUID_BPM_SERVICE;
    
    err_code = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY,
                                        &service_uuid,
                                        &p_bpm_service->service_handle);
    APP_ERROR_CHECK(err_code);
    
    // Characteristic metadata
    ble_gatts_char_md_t char_md = {0};
    char_md.char_props.read   = 1;
    char_md.char_props.notify = 1;
    
    // Add a Client Characteristic Configuration Descriptor (CCCD)
    ble_gatts_attr_md_t cccd_md = {0};
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.write_perm);
    cccd_md.vloc = BLE_GATTS_VLOC_STACK;

    char_md.p_cccd_md = &cccd_md;
    
    ble_uuid_t char_uuid;
    char_uuid.type = m_bpm_uuid_type;
    char_uuid.uuid = BLE_UUID_BPM_CHAR;
    
    ble_gatts_attr_md_t attr_md = {0};
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_NO_ACCESS(&attr_md.write_perm);
    attr_md.vloc = BLE_GATTS_VLOC_STACK;
    
    ble_gatts_attr_t attr_char_value = {0};
    attr_char_value.p_uuid    = &char_uuid;
    attr_char_value.p_attr_md = &attr_md;
    // Heart Rate Measurement can be up to 20 bytes with RR intervals
    attr_char_value.init_len  = 2;  // Minimum: flags + 8-bit HR value
    attr_char_value.max_len   = 20; // Maximum with multiple RR intervals
    
    uint8_t initial_hrm[2] = {0x00, 0x00}; // Flags=0, HR=0
    attr_char_value.p_value   = initial_hrm;
    
    err_code = sd_ble_gatts_characteristic_add(p_bpm_service->service_handle,
                                               &char_md,
                                               &attr_char_value,
                                               &p_bpm_service->bpm_char_handles);
    APP_ERROR_CHECK(err_code);
    
    p_bpm_service->conn_handle = BLE_CONN_HANDLE_INVALID;
    NRF_LOG_INFO("BPM Service initialized.");
}

void bpm_service_on_ble_evt(ble_bpm_service_t * p_bpm_service,
                            ble_evt_t const   * p_ble_evt)
{
    if (p_bpm_service == NULL || p_ble_evt == NULL)
    {
        return;
    }

    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GAP_EVT_CONNECTED:
            p_bpm_service->conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
            break;

        case BLE_GAP_EVT_DISCONNECTED:
            p_bpm_service->conn_handle = BLE_CONN_HANDLE_INVALID;
            break;

        case BLE_GATTS_EVT_WRITE:
        {
            ble_gatts_evt_write_t const * p_evt_write =
                &p_ble_evt->evt.gatts_evt.params.write;
            if (p_evt_write->handle == p_bpm_service->bpm_char_handles.cccd_handle &&
                p_evt_write->len    == 2)
            {
                bool notif_enabled = ble_srv_is_notification_enabled(p_evt_write->data);
                NRF_LOG_INFO("BPM notifications %s",
                             notif_enabled ? "enabled" : "disabled");
            }
        } break;

        default:
            break;
    }
}

uint32_t bpm_service_update(ble_bpm_service_t * p_bpm_service, uint16_t bpm)
{
    ret_code_t err_code;
    ble_gatts_value_t gatts_value;
    
    // Heart Rate Measurement data structure
    uint8_t hrm_data[3];  // Max we need for basic measurement
    uint8_t data_len = 0;
    
    // Check if the connection handle is valid
    if (p_bpm_service->conn_handle == BLE_CONN_HANDLE_INVALID)
    {
        return NRF_ERROR_INVALID_STATE;
    }
    
    // Build the Heart Rate Measurement according to spec
    if (bpm > 255)
    {
        // 16-bit format
        hrm_data[0] = 0x01;  // Flags: bit 0 set = 16-bit Heart Rate
        hrm_data[1] = (uint8_t)(bpm & 0xFF);
        hrm_data[2] = (uint8_t)(bpm >> 8);
        data_len = 3;
    }
    else
    {
        // 8-bit format
        hrm_data[0] = 0x00;  // Flags: bit 0 clear = 8-bit Heart Rate
        hrm_data[1] = (uint8_t)bpm;
        data_len = 2;
    }
    
    // Update the value in the GATT table
    memset(&gatts_value, 0, sizeof(gatts_value));
    gatts_value.len     = data_len;
    gatts_value.offset  = 0;
    gatts_value.p_value = hrm_data;

    err_code = sd_ble_gatts_value_set(p_bpm_service->conn_handle,
                                      p_bpm_service->bpm_char_handles.value_handle,
                                      &gatts_value);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    // Check if notifications are enabled
    uint8_t cccd_value[2] = {0};
    ble_gatts_value_t cccd_gatts_value = {
        .len = sizeof(cccd_value),
        .offset = 0,
        .p_value = cccd_value
    };
    
    err_code = sd_ble_gatts_value_get(p_bpm_service->conn_handle,
                                      p_bpm_service->bpm_char_handles.cccd_handle,
                                      &cccd_gatts_value);
    
    bool notif_enabled = (err_code == NRF_SUCCESS) && 
                         ble_srv_is_notification_enabled(cccd_value);
    
    if (notif_enabled)
    {
        // Send notification
        ble_gatts_hvx_params_t hvx_params;
        memset(&hvx_params, 0, sizeof(hvx_params));

        hvx_params.handle = p_bpm_service->bpm_char_handles.value_handle;
        hvx_params.type   = BLE_GATT_HVX_NOTIFICATION;
        hvx_params.offset = 0;
        hvx_params.p_len  = &data_len;
        hvx_params.p_data = hrm_data;

        err_code = sd_ble_gatts_hvx(p_bpm_service->conn_handle, &hvx_params);
        
        // Handle notification queue full (not critical)
        if (err_code == NRF_ERROR_RESOURCES)
        {
            NRF_LOG_WARNING("BPM notification queue full");
            return NRF_SUCCESS;
        }
        else if (err_code == NRF_ERROR_INVALID_STATE)
        {
            // Notifications not enabled or other state issue
            NRF_LOG_WARNING("Cannot send BPM notification - invalid state");
            return NRF_SUCCESS;
        }
        else if (err_code != NRF_SUCCESS)
        {
            return err_code;
        }
    }

    NRF_LOG_INFO("BPM value updated to %d", bpm);
    return NRF_SUCCESS;
}


