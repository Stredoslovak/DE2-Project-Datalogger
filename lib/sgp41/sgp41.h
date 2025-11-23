#ifndef SGP41_H
#define SGP41_H

/**
 * @file sgp41.h
 * @brief SGP41 VOC and NOx Gas Sensor Driver
 * 
 * Driver for Sensirion SGP41 using pure AVR-GCC (no Arduino libraries)
 * Measures VOC (Volatile Organic Compounds) and NOx (Nitrogen Oxides)
 * 
 * I2C Address: 0x59 (fixed, 7-bit)
 * 
 * @copyright (c) 2025 MIT License
 */

#include <avr/io.h>
#include <stdint.h>

// -- SGP41 I2C Address ------------------------------------
#define SGP41_I2C_ADDRESS           0x59

// -- SGP41 Commands (16-bit, MSB first) -------------------
#define SGP41_CMD_MEASURE_RAW_H     0x26
#define SGP41_CMD_MEASURE_RAW_L     0x19
#define SGP41_CMD_CONDITIONING_H    0x26
#define SGP41_CMD_CONDITIONING_L    0x12
#define SGP41_CMD_GET_SERIAL_H      0x36
#define SGP41_CMD_GET_SERIAL_L      0x82
#define SGP41_CMD_SELF_TEST_H       0x28
#define SGP41_CMD_SELF_TEST_L       0x0E

// -- Timing (milliseconds) --------------------------------
#define SGP41_MEASURE_DELAY_MS      50
#define SGP41_CONDITIONING_DELAY_MS 50
#define SGP41_SELFTEST_DELAY_MS     250

// -- Default compensation values --------------------------
#define SGP41_DEFAULT_RH            0x8000  // 50% RH
#define SGP41_DEFAULT_T             0x6666  // 25°C

// -- Self-test pass value ---------------------------------
#define SGP41_SELFTEST_PASS         0xD400

// -- Function Prototypes ----------------------------------

/**
 * @brief Calculate CRC-8 checksum for SGP41 data validation
 * @param data Pointer to data bytes
 * @param len Number of bytes
 * @return CRC-8 checksum value
 */
uint8_t sgp41_calculate_crc(const uint8_t *data, uint8_t len);

/**
 * @brief Initialize and test SGP41 presence on I2C bus
 * @return 0 if device found, error code otherwise
 */
uint8_t sgp41_init(void);

/**
 * @brief Execute self-test
 * @param result Pointer to store test result (should be 0xD400 for pass)
 * @return 0 on success, error code otherwise
 */
uint8_t sgp41_self_test(uint16_t *result);

/**
 * @brief Get 48-bit serial number
 * @param serial Pointer to array of 3 x uint16_t to store serial number
 * @return 0 on success, error code otherwise
 */
uint8_t sgp41_get_serial_number(uint16_t serial[3]);

/**
 * @brief Execute conditioning (MUST run for 10 seconds at startup)
 * 
 * This command must be called for 10 seconds before first measurement.
 * During conditioning, only VOC raw signal is returned, NOx stays at 0.
 * 
 * @param rh_comp Relative humidity compensation value (0x8000 = 50% RH)
 * @param t_comp Temperature compensation value (0x6666 = 25°C)
 * @param sraw_voc Pointer to store VOC raw signal (0-65535)
 * @return 0 on success, error code otherwise
 */
uint8_t sgp41_execute_conditioning(uint16_t rh_comp, uint16_t t_comp, 
                                    uint16_t *sraw_voc);

/**
 * @brief Measure raw VOC and NOx signals
 * 
 * Call this function every 1 second for continuous monitoring.
 * First 10 seconds must use sgp41_execute_conditioning() instead.
 * 
 * @param rh_comp Relative humidity compensation (0x8000 = 50%)
 * @param t_comp Temperature compensation (0x6666 = 25°C)
 * @param sraw_voc Pointer to store VOC raw signal (0-65535 ticks)
 * @param sraw_nox Pointer to store NOx raw signal (0-65535 ticks)
 * @return 0 on success, error code otherwise
 */
uint8_t sgp41_measure_raw_signals(uint16_t rh_comp, uint16_t t_comp,
                                   uint16_t *sraw_voc, uint16_t *sraw_nox);

#endif // SGP41_H
