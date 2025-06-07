// accel.h

#ifndef ACCEL_H
#define ACCEL_H

#include <stdint.h>
#include <stdbool.h>

// MPU-6050 I²C address (AD0 low). Change to 0x69 if AD0 is pulled high.
#define MPU6050_I2C_ADDRESS      0x68  

// MPU-6050 register map
#define MPU6050_REG_PWR_MGMT_1   0x6B
#define MPU6050_REG_CONFIG       0x1A
#define MPU6050_REG_ACCEL_CONFIG 0x1C
#define MPU6050_REG_ACCEL_XOUT_H 0x3B

/**
 * @brief  Write a single byte to an MPU-6050 register.
 * @return true if ACKed and transfer complete.
 */
bool mpu6050_register_write(uint8_t reg_addr, uint8_t value);

/**
 * @brief  Read one or more bytes starting at an MPU-6050 register.
 * @param  destination  pointer to buffer for the read data
 * @param  length       number of bytes to read
 * @return true if ACKed and transfer complete.
 */
bool mpu6050_register_read(uint8_t reg_addr, uint8_t *destination, uint8_t length);

/**
 * @brief  Wake up the MPU-6050 and configure default settings.
 * @return true on success (all writes ACKed).
 */
bool mpu6050_init(void);

/**
 * @brief  Read raw accelerometer data.
 * @param  ax  pointer to signed 16-bit X reading
 * @param  ay  pointer to signed 16-bit Y reading
 * @param  az  pointer to signed 16-bit Z reading
 * @return true on success.
 */
bool mpu6050_read_accel(int16_t *ax, int16_t *ay, int16_t *az);

#endif // ACCEL_H