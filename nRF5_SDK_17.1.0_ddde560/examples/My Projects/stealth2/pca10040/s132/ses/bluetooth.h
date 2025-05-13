#ifndef BLUETOOTH_H
#define BLUETOOTH_H

#include <stdint.h>
#include <stdbool.h>
#include "ble.h"
#include "ble_nus.h"

#define DEVICE_NAME "HeartRateMonitor"  // Name of the BLE device
#define APP_BLE_CONN_CFG_TAG 1         // BLE connection config tag
#define APP_BLE_OBSERVER_PRIO 3        // BLE observer priority

// Function prototypes
void bluetooth_init(void);
void bluetooth_send_data(uint8_t *data, uint16_t length);

#endif // BLUETOOTH_H
