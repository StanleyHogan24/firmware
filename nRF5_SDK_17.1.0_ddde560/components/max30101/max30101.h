#ifndef MAX30101_H__
#define MAX30101_H__

#include <stdint.h>
#include <stdbool.h>

/*
 * MAX30101 I2C Address and TWI Configuration
 *
 * The MAX30101 typically uses the 7-bit I2C address 0x57.
 * Ensure that this matches your module documentation.
 */
#define MAX30101_I2C_ADDRESS       0x57

// TWI (I2C) Instance and Pin Configuration
#define TWI_INSTANCE_ID     0
#define TWI_SCL_M           27   // I2C SCL Pin (set according to your board layout)
#define TWI_SDA_M           26   // I2C SDA Pin (set according to your board layout)

/*
 * MAX30101 Register Addresses
 *
 * The register map is defined in the datasheet.
 */
#define MAX30101_INTR_STATUS_1     0x00  // Interrupt Status Register 1
#define MAX30101_INTR_STATUS_2     0x01  // Interrupt Status Register 2
#define MAX30101_INTR_ENABLE_1     0x02  // Interrupt Enable Register 1
#define MAX30101_INTR_ENABLE_2     0x03  // Interrupt Enable Register 2
#define MAX30101_FIFO_WR_PTR       0x04  // FIFO Write Pointer Register
#define MAX30101_OVF_COUNTER       0x05  // Overflow Counter Register
#define MAX30101_FIFO_RD_PTR       0x06  // FIFO Read Pointer Register
#define MAX30101_FIFO_DATA         0x07  // FIFO Data Register (used to read sensor samples)
#define MAX30101_FIFO_CONFIG       0x08  // FIFO Configuration Register
#define MAX30101_MODE_CONFIG       0x09  // Mode Configuration Register
#define MAX30101_SPO2_CONFIG       0x0A  // SpO2 Configuration Register
#define MAX30101_LED1_PA           0x0C  // LED1 Pulse Amplitude Register (Red LED)
#define MAX30101_LED2_PA           0x0D  // LED2 Pulse Amplitude Register (IR LED)
#define MAX30101_LED3_PA           0x0E  // LED3 Pulse Amplitude Register (Green LED)

/*
 * MAX30101 Configuration Values
 *
 * These hex values are written to the sensor's registers to configure its operation.
 * Please consult the datasheet for details on the bit fields.
 *
 * 1. Reset Command (Register 0x09 - Mode Configuration):
 *    - Bit 6 (RESET): Writing 0x40 sets bit 6 to 1 to initiate a software reset.
 *    - After reset, all registers are set to their default power-on values.
 */
#define MAX30101_RESET             0x40

/*
 * 2. Mode Setting (Register 0x09 - Mode Configuration):
 *    - Bits [2:0] (MODE): Setting these bits to 0x03 configures the sensor in SpO2 mode.
 *      In SpO2 mode, the sensor uses the red and IR LED channels to measure oxygen saturation.
 *    - Note: Bit 7 is SHDN (shutdown) and should be 0 for normal operation.
 *    - 0x07 (111) configures the sensor for Multi-LED Mode. This will allow us to use the Green LED. 
 */
#define MAX30101_MODE_SPO2         0x03

/*
 * 3. FIFO Configuration (Register 0x08 - FIFO Configuration):
 *    The 8-bit value 0x1F (binary 0001 1111) sets the following:
 *      - Bits [7:5] (SMP_AVE): 000 -> No sample averaging is performed.
 *      - Bit 4 (FIFO_ROLLOVER_EN): 1 -> Enables FIFO rollover when full.
 *      - Bits [3:0] (FIFO_A_FULL): 1111 -> Almost-Full threshold is set to 15 samples.
 *    This configuration ensures continuous data streaming with minimal delay.
 */
#define MAX30101_FIFO_CFG          0x1F

/*
 * 4. SpO2 Configuration (Register 0x0A - SpO2 Configuration):
 *
 *    The 8-bit value 0x2F (binary 0010 1111) sets:
 *      - Bits [0, 1] (LED_PW): 11 -> LED Pulse Width = 411 µs.
 *             (Mapping: 00=69µs, 01=118µs, 10=215µs, 11=411µs)
 *      - Bits [2, 3, 4] (SPO2_SR): 011 -> SpO2 Sample Rate = 400 samples per second.
 *             (Mapping: 000=50 sps, 001=100 sps, 010=200 sps, 011=400 sps,
 *                       100=800 sps, 101=1000 sps, 110=1600 sps, 111=3200 sps)
 *      - Bits [5, 6] (SPO2_ADC_RGE): 01 -> ADC Full-Scale Range = 4096 nA (4.096 µA).
 *
 *    Note: The chosen sample rate of 3200 sps and full-scale range might be higher than needed for
 *          certain applications (like heart rate monitoring at ~100 sps). Adjust this value as needed.
 */
#define MAX30101_SPO2_CFG          0x47

/*
 * 5. LED Pulse Amplitude Settings (Registers 0x0C, 0x0D, 0x0E):
 *    - These registers control the drive current for each LED channel.
 *    - The value range is 0x00 to 0xFF. The current provided to each LED is proportional to the register value.
 *    - The example value 0x03 is used here to set a low LED current.
 *      You may need to adjust these values based on your application requirements and ambient conditions.
 */
#define MAX30101_LED1_PA_VAL       0x3F  // Red LED drive current
#define MAX30101_LED2_PA_VAL       0x3F  // IR LED drive current
#define MAX30101_LED3_PA_VAL       0x3F  // Green LED drive current

/*
 * Function Prototypes for MAX30101 Sensor Interaction
 */

/*
 * @brief Initialize the TWI (I2C) master interface.
 *
 * Configures the I2C peripheral with the pin and instance settings defined above.
 */
void twi_master_init(void);

/*
 * @brief Write a value to a MAX30101 register.
 *
 * @param register_address The register address to write to.
 * @param value The value to be written.
 * @return true if the write operation succeeds, false otherwise.
 */
bool max30101_register_write(uint8_t register_address, uint8_t value);

/*
 * @brief Read one or more bytes from a MAX30101 register.
 *
 * @param register_address The register address to read from.
 * @param destination Pointer to the buffer where the read data will be stored.
 * @param number_of_bytes Number of bytes to read.
 * @return true if the read operation succeeds, false otherwise.
 */
bool max30101_register_read(uint8_t register_address, uint8_t *destination, uint8_t number_of_bytes);

/*
 * @brief Initialize the MAX30101 sensor.
 *
 * This function performs the following operations:
 *   - Issues a software reset by writing MAX30101_RESET (0x40) to the Mode Configuration register (0x09).
 *   - Configures the sensor mode to SpO2 by writing MAX30101_MODE_SPO2 (0x03) to register 0x09.
 *   - Configures the FIFO with MAX30101_FIFO_CFG (0x1F) to set no averaging, enable FIFO rollover,
 *     and set the almost-full threshold to 15 samples.
 *   - Configures the SpO2 settings (LED pulse width, sample rate, ADC range) by writing MAX30101_SPO2_CFG (0x2F) to register 0x0A.
 *   - Sets the LED drive currents using the LED Pulse Amplitude registers.
 *
 * @return true if the sensor is successfully initialized, false otherwise.
 */
bool max30101_init(void);

/*
 * @brief Read data from the MAX30101 FIFO.
 *
 * Retrieves the latest sensor readings from the FIFO buffer. The FIFO may contain data for multiple LED channels.
 *
 * @param green_led Pointer to store the green LED sample (if available).
 * @param red_led Pointer to store the red LED sample.
 * @param ir_led Pointer to store the IR LED sample.
 * @return true if the FIFO read operation is successful, false otherwise.
 */
bool max30101_read_fifo(uint32_t *red_led, uint32_t *ir_led);

bool max30101_reset_fifo(void);

/*
 * @brief Set the LED pulse amplitude for a specific LED.
 *
 * Adjusts the LED drive current by writing a new amplitude value to the corresponding LED register.
 *
 * @param led_number The LED channel number (1 for red, 2 for IR, 3 for green).
 * @param pa_value The pulse amplitude value to be set (0x00 to 0xFF).
 * @return true if the operation is successful, false otherwise.
 */
bool max30101_set_led_pa(uint8_t led_number, uint8_t pa_value);

#endif // MAX30101_H__
