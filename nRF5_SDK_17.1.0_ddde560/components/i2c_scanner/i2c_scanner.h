#ifndef I2C_SCANNER_H
#define I2C_SCANNER_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initializes a TWI interface for the I2C scanner.
 */
void i2c_scanner_init(void);

/**
 * @brief Performs an I2C scan from 0x08 to 0x77.
 * Logs found addresses via NRF_LOG_INFO.
 */
void i2c_scanner_scan(void);

#ifdef __cplusplus
}
#endif

#endif // I2C_SCANNER_H
