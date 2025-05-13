#include <stdbool.h>
#include "nrf_drv_twi.h"
#include "app_error.h"
#include "common_twi.h"

// TWI instance definition and related variables
volatile bool m_xfer_done = false;
const nrf_drv_twi_t m_twi = NRF_DRV_TWI_INSTANCE(TWI_INSTANCE_ID);

// TWI event handler function
void twi_handler(nrf_drv_twi_evt_t const * p_event, void * p_context)
{
    if (p_event->type == NRF_DRV_TWI_EVT_DONE)
    {
        m_xfer_done = true;
    }
    // ... handle other events if necessary
}

bool twi_master_transfer(uint8_t address, uint8_t *p_data, uint8_t length, bool stop)
{
    // For debugging/demo purposes, simply return true.
    // You might want to print the transfer details:
    // printf("TWI Transfer to address 0x%02X: ", address);
    // for (uint8_t i = 0; i < length; i++) {
    //     printf("0x%02X ", p_data[i]);
    // }
    // printf(" Stop: %d\n", stop);

    return true;
}



// (Rest of your MPU6050 driver code)
