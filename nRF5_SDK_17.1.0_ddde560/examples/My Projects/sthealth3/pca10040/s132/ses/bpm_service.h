#ifndef BPM_SERVICE_H__
#define BPM_SERVICE_H__

#include <stdint.h>
#include <stdbool.h>
#include "ble.h"
#include "ble_srv_common.h"

// UUIDs used for the BPM (Heart Rate) service and characteristic.
// These match the standard Heart Rate Service definitions.
#define BLE_UUID_BPM_SERVICE  0x180D  /**< Heart Rate service UUID. */
#define BLE_UUID_BPM_CHAR     0x2A37  /**< Heart Rate Measurement characteristic UUID. */

// Structure to hold the service and its characteristic handles.
typedef struct
{
    uint16_t                    service_handle;
    ble_gatts_char_handles_t    bpm_char_handles;
    uint16_t                    conn_handle;
} ble_bpm_service_t;

/**@brief Function for initializing the custom BPM Service.
 *
 * @param[in,out] p_bpm_service  Service structure.
 */
void bpm_service_init(ble_bpm_service_t * p_bpm_service);

/**@brief Function for updating the BPM characteristic and sending a notification.
 *
 * @param[in,out] p_bpm_service  Service structure.
 * @param[in]     bpm            The new BPM value.
 *
 * @return NRF_SUCCESS on success, otherwise an error code.
 */
uint32_t bpm_service_update(ble_bpm_service_t * p_bpm_service, uint16_t bpm);

#endif // BPM_SERVICE_H__
