// steps.c

#include "accel.h"
#include "nrf_log.h"
#include <math.h>
#include "steps.h"

#define ACCEL_SCALE         16384.0f   // LSB per g for ±2 g
#define STEP_THRESH_HIGH    1.2f       // g’s – tune experimentally
#define STEP_THRESH_LOW     0.9f       // g’s – hysteresis

static uint32_t m_step_count = 0;
static bool     m_step_flag  = false;

/** @brief  Call this once after accel init if you need any state reset. */
void steps_init(void)
{
    m_step_count = 0;
    m_step_flag  = false;
}

/** @brief  Call this at your sample rate (e.g. 50–100 Hz). */
void steps_update(void)
{
    int16_t ax, ay, az;
    if (!mpu6050_read_accel(&ax, &ay, &az)) {
        NRF_LOG_WARNING("Accel read failed");
        return;
    }

    // Convert to g’s and compute vector magnitude
    float fx = ax / ACCEL_SCALE;
    float fy = ay / ACCEL_SCALE;
    float fz = az / ACCEL_SCALE;
    float mag = sqrtf(fx*fx + fy*fy + fz*fz);

    // Simple peak detector with hysteresis
    if (mag > STEP_THRESH_HIGH && !m_step_flag) {
        m_step_flag = true;
        m_step_count++;
        NRF_LOG_INFO("Steps: %d", m_step_count);
    }
    else if (mag < STEP_THRESH_LOW) {
        m_step_flag = false;
    }
}

/** @brief  Return total steps counted so far. */
uint32_t steps_get_count(void)
{
    return m_step_count;
}