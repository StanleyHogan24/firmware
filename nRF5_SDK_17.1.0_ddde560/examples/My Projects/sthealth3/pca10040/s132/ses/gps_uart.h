// gps_uart.h
#ifndef GPS_UART_H_
#define GPS_UART_H_

#include <stdint.h>
#include "nrfx_uart.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the UART peripheral for GPS communication using nrfx_uart.
 *
 * Configures pins P0.06 (TX) and P0.08 (RX) at 9600 baud with interrupt-driven RX.
 */
void gps_uart_init(void);

#ifdef __cplusplus
}
#endif

#endif // GPS_UART_H_