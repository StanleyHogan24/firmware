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
    
    // [Rest of your characteristic setup code...]
    ble_gatts_char_md_t char_md = {0};
    char_md.char_props.read   = 1;
    char_md.char_props.notify = 1;
    
    // Add a Client Characteristic Configuration Descriptor (CCCD) so that
    // a peer can enable notifications.
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
    attr_char_value.init_len  = sizeof(uint16_t);
    attr_char_value.max_len   = sizeof(uint16_t);
    
    uint16_t initial_bpm = 0;
    attr_char_value.p_value   = (uint8_t *)&initial_bpm;
    
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

    // Check if the connection handle is valid
    if (p_bpm_service->conn_handle == BLE_CONN_HANDLE_INVALID)
    {
        return NRF_ERROR_INVALID_STATE;
    }

    // Update the BPM value in the GATT table
    memset(&gatts_value, 0, sizeof(gatts_value));
    gatts_value.len     = sizeof(uint16_t);
    gatts_value.offset  = 0;
    gatts_value.p_value = (uint8_t *)&bpm;

    err_code = sd_ble_gatts_value_set(p_bpm_service->conn_handle,
                                      p_bpm_service->bpm_char_handles.value_handle,
                                      &gatts_value);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    // Notify the updated BPM value to the connected client
    ble_gatts_hvx_params_t hvx_params;
    memset(&hvx_params, 0, sizeof(hvx_params));

    hvx_params.handle = p_bpm_service->bpm_char_handles.value_handle;
    hvx_params.type   = BLE_GATT_HVX_NOTIFICATION;
    hvx_params.offset = 0;
    hvx_params.p_len  = &gatts_value.len;
    hvx_params.p_data = gatts_value.p_value;

    err_code = sd_ble_gatts_hvx(p_bpm_service->conn_handle, &hvx_params);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    NRF_LOG_INFO("BPM value updated to %d", bpm);
    return NRF_SUCCESS;
}


