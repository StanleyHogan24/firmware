#ifndef TWI_INTERFACE_H__
#define TWI_INTERFACE_H__

#include <stdbool.h>
#include "nrf_drv_twi.h"

// TWI instance definition and related variables
#define TWI_INSTANCE_ID     0
extern volatile bool m_xfer_done;
extern const nrf_drv_twi_t m_twi;

void twi_handler(nrf_drv_twi_evt_t const * p_event, void * p_context);
void twi_master_init(void);

bool twi_master_transfer(uint8_t address, uint8_t *p_data, uint8_t length, bool stop);

#endif // TWI_INTERFACE_H__
