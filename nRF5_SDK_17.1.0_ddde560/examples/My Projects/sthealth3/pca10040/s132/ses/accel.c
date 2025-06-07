// accel.c

#include "accel.h"
#include "common_twi.h"   // provides: extern const nrf_drv_twi_t m_twi; extern volatile bool m_xfer_done;
#include "nrf_delay.h"
#include "nrf_log.h"


#define TWI_TIMEOUT_MS  10  // how long to wait for each transfer

static bool wait_for_xfer(void)
{
    uint32_t timeout = TWI_TIMEOUT_MS;
    while (!m_xfer_done && timeout--)
    {
        nrf_delay_ms(1);
    }
    if (!m_xfer_done)
    {
        NRF_LOG_ERROR("TWI transfer timeout");
        return false;
    }
    return true;
}



//------------------------------------------------------------------------------
// Single-byte write
bool mpu6050_register_write(uint8_t reg_addr, uint8_t value)
{
    ret_code_t err;
    uint8_t tx_buf[2] = { reg_addr, value };

    m_xfer_done = false;
    err = nrf_drv_twi_tx(&m_twi, MPU6050_I2C_ADDRESS, tx_buf, sizeof(tx_buf), false);
    if (err != NRF_SUCCESS)
    {
        NRF_LOG_ERROR("TWI TX start failed: 0x%08X", err);
        return false;
    }

    // Now wait for the DONE event, but with timeout
    if (!wait_for_xfer())
        return false;

    return true;
}


//------------------------------------------------------------------------------
// Multi-byte read
bool mpu6050_register_read(uint8_t reg_addr, uint8_t *dst, uint8_t len)
{
    ret_code_t err;

    // Write register address
    m_xfer_done = false;
    err = nrf_drv_twi_tx(&m_twi, MPU6050_I2C_ADDRESS, &reg_addr, 1, true);
    if (err != NRF_SUCCESS)
    {
        NRF_LOG_ERROR("TWI addr write failed: 0x%08X", err);
        return false;
    }
    if (!wait_for_xfer())
        return false;

    // Read data
    m_xfer_done = false;
    err = nrf_drv_twi_rx(&m_twi, MPU6050_I2C_ADDRESS, dst, len);
    if (err != NRF_SUCCESS)
    {
        NRF_LOG_ERROR("TWI RX start failed: 0x%08X", err);
        return false;
    }
    if (!wait_for_xfer())
        return false;

    return true;
}


//------------------------------------------------------------------------------
// Initialization sequence
bool mpu6050_init(void)
{
    NRF_LOG_INFO("MPU6050 init sequence start");
    bool ok = true;

    ok &= mpu6050_register_write(MPU6050_REG_PWR_MGMT_1, 0x00);
    nrf_delay_ms(100);

    ok &= mpu6050_register_write(MPU6050_REG_CONFIG, 0x00);
    ok &= mpu6050_register_write(MPU6050_REG_ACCEL_CONFIG, 0x00);

    if (!ok)
    {
        NRF_LOG_ERROR("MPU6050 init NACK detected");
    }
    else
    {
        NRF_LOG_INFO("MPU6050 init OK");
    }
    return ok;
}

//------------------------------------------------------------------------------
// Read accelerometer FIFO
bool mpu6050_read_accel(int16_t *ax, int16_t *ay, int16_t *az)
{
    uint8_t raw[6];
    if (!mpu6050_register_read(MPU6050_REG_ACCEL_XOUT_H, raw, sizeof(raw))) {
        return false;
    }

    *ax = (int16_t)((raw[0] << 8) | raw[1]);
    *ay = (int16_t)((raw[2] << 8) | raw[3]);
    *az = (int16_t)((raw[4] << 8) | raw[5]);
    return true;
}