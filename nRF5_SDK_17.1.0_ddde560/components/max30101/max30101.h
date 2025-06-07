#ifndef MAX30101_H__
#define MAX30101_H__

#include <stdint.h>
#include <stdbool.h>

// I2C Address
#define MAX30101_I2C_ADDRESS       0x57

// TWI (I2C) Configuration
#define TWI_INSTANCE_ID     0
#define TWI_SCL_PIN         27   // SCL Pin
#define TWI_SDA_PIN         26   // SDA Pin

// Register Map
#define MAX30101_INTR_STATUS_1       0x00
#define MAX30101_INTR_STATUS_2       0x01
#define MAX30101_INTR_ENABLE_1       0x02
#define MAX30101_INTR_ENABLE_2       0x03
#define MAX30101_FIFO_WR_PTR         0x04
#define MAX30101_OVF_COUNTER         0x05
#define MAX30101_FIFO_RD_PTR         0x06
#define MAX30101_FIFO_DATA           0x07
#define MAX30101_FIFO_CONFIG         0x08
#define MAX30101_MODE_CONFIG         0x09
#define MAX30101_SPO2_CONFIG         0x0A

// LED Pulse Amplitude Registers
#define MAX30101_LED1_PA             0x0C  // Red LED
#define MAX30101_LED2_PA             0x0D  // IR LED
#define MAX30101_LED3_PA             0x0E  // Green LED
#define MAX30101_LED4_PA             0x0F  // White/Ambient LED

// Multi-LED Slot Config Registers
#define MAX30101_REG_MULTI_LED_MODE1 0x11  // SLOT1[3:0] | SLOT2[7:4]
#define MAX30101_REG_MULTI_LED_MODE2 0x12  // SLOT3[3:0] | SLOT4[7:4]

// Operating Modes
#define MAX30101_MODE_SHUTDOWN       0x80
#define MAX30101_MODE_RESET          0x40
#define MAX30101_MODE_HEART_RATE     0x02
#define MAX30101_MODE_SPO2           0x03
#define MAX30101_MODE_MULTI_LED      0x07  // Enables LED1, LED2, LED3 slots

// FIFO Configuration
#define MAX30101_FIFO_CFG            0x1F  // no averaging, rollover, threshold=15

// SpO2 Configuration
#define MAX30101_SPO2_CFG            0x47  // 411us PW, 400sps, 4096nA FS

// Default LED Drive Currents (0x00-0xFF)
#define MAX30101_LED1_PA_VAL         0x1  // Red
#define MAX30101_LED2_PA_VAL         0x1  // IR
#define MAX30101_LED3_PA_VAL         0x7F  // Green
#define MAX30101_LED4_PA_VAL         0x20  // Ambient

// LED Slot Assignments
typedef enum {
    SLOT_NONE      = 0x0,
    SLOT_RED_LED   = 0x1,
    SLOT_IR_LED    = 0x2,
    SLOT_GREEN_LED = 0x3,
    SLOT_WHITE_LED = 0x4
} max30101_led_slot_t;

// Function Prototypes
void    twi_master_init(void);
bool    max30101_register_write(uint8_t reg_addr, uint8_t value);
bool    max30101_register_read(uint8_t reg_addr, uint8_t *buffer, uint8_t len);
bool    max30101_init(void);
bool    max30101_reset_fifo(void);
// Read FIFO for three channels: green, red, IR
bool    max30101_read_fifo(uint32_t *green_led, uint32_t *red_led, uint32_t *ir_led);
bool    max30101_set_led_pa(uint8_t led_number, uint8_t pa_value);

#endif // MAX30101_H__
