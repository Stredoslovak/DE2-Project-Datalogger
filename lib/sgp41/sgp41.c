/**
 * @file sgp41.c
 * @brief SGP41 Gas Sensor Driver Implementation
 */

#include "sgp41.h"
#include <twi.h>
#include <util/delay.h>

// -- CRC-8 Configuration ----------------------------------
#define CRC8_POLYNOMIAL 0x31u
#define CRC8_INIT       0xFF

// -- Public Functions -------------------------------------

uint8_t sgp41_calculate_crc(const uint8_t *data, uint8_t len)
{
    uint8_t crc = CRC8_INIT;
    
    for (uint8_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint8_t bit = 0; bit < 8; bit++) {
            if (crc & 0x80) {
                crc = (crc << 1) ^ CRC8_POLYNOMIAL;
            } else {
                crc = (crc << 1);
            }
        }
    }
    
    return crc;
}

uint8_t sgp41_init(void)
{
    return twi_test_address(SGP41_I2C_ADDRESS);
}

uint8_t sgp41_self_test(uint16_t *result)
{
    uint8_t data[3];
    uint8_t crc_calc;
    
    // Send self-test command
    twi_start();
    if (twi_write((SGP41_I2C_ADDRESS << 1) | TWI_WRITE) != 0) {
        twi_stop();
        return 1;
    }
    twi_write(SGP41_CMD_SELF_TEST_H);
    twi_write(SGP41_CMD_SELF_TEST_L);
    twi_stop();
    
    // Wait for self-test to complete
    _delay_ms(SGP41_SELFTEST_DELAY_MS);
    
    // Read result (2 bytes data + 1 byte CRC)
    twi_start();
    if (twi_write((SGP41_I2C_ADDRESS << 1) | TWI_READ) != 0) {
        twi_stop();
        return 1;
    }
    
    data[0] = twi_read(TWI_ACK);  // MSB
    data[1] = twi_read(TWI_ACK);  // LSB
    data[2] = twi_read(TWI_NACK); // CRC
    twi_stop();
    
    // Validate CRC
    crc_calc = sgp41_calculate_crc(data, 2);
    if (crc_calc != data[2]) {
        return 2; // CRC error
    }
    
    *result = ((uint16_t)data[0] << 8) | data[1];
    return 0;
}

uint8_t sgp41_get_serial_number(uint16_t serial[3])
{
    uint8_t data[9];
    uint8_t crc_calc;
    
    // Send get serial number command
    twi_start();
    if (twi_write((SGP41_I2C_ADDRESS << 1) | TWI_WRITE) != 0) {
        twi_stop();
        return 1;
    }
    twi_write(SGP41_CMD_GET_SERIAL_H);
    twi_write(SGP41_CMD_GET_SERIAL_L);
    twi_stop();
    
    _delay_ms(1);
    
    // Read 3 words (each 2 bytes + 1 CRC = 9 bytes total)
    twi_start();
    if (twi_write((SGP41_I2C_ADDRESS << 1) | TWI_READ) != 0) {
        twi_stop();
        return 1;
    }
    
    for (uint8_t i = 0; i < 9; i++) {
        if (i == 8) {
            data[i] = twi_read(TWI_NACK);
        } else {
            data[i] = twi_read(TWI_ACK);
        }
    }
    twi_stop();
    
    // Validate CRCs and extract serial number
    for (uint8_t i = 0; i < 3; i++) {
        crc_calc = sgp41_calculate_crc(&data[i * 3], 2);
        if (crc_calc != data[i * 3 + 2]) {
            return 2; // CRC error
        }
        serial[i] = ((uint16_t)data[i * 3] << 8) | data[i * 3 + 1];
    }
    
    return 0;
}

uint8_t sgp41_execute_conditioning(uint16_t rh_comp, uint16_t t_comp, 
                                    uint16_t *sraw_voc)
{
    uint8_t data[3];
    uint8_t params[6];
    uint8_t crc_calc;
    
    // Prepare RH parameter with CRC
    params[0] = (uint8_t)(rh_comp >> 8);
    params[1] = (uint8_t)(rh_comp & 0xFF);
    params[2] = sgp41_calculate_crc(params, 2);
    
    // Prepare temperature parameter with CRC
    params[3] = (uint8_t)(t_comp >> 8);
    params[4] = (uint8_t)(t_comp & 0xFF);
    params[5] = sgp41_calculate_crc(&params[3], 2);
    
    // Send conditioning command with parameters
    twi_start();
    if (twi_write((SGP41_I2C_ADDRESS << 1) | TWI_WRITE) != 0) {
        twi_stop();
        return 1;
    }
    
    twi_write(SGP41_CMD_CONDITIONING_H);
    twi_write(SGP41_CMD_CONDITIONING_L);
    
    for (uint8_t i = 0; i < 6; i++) {
        twi_write(params[i]);
    }
    twi_stop();
    
    // Wait for measurement
    _delay_ms(SGP41_CONDITIONING_DELAY_MS);
    
    // Read VOC result (2 bytes + 1 CRC)
    twi_start();
    if (twi_write((SGP41_I2C_ADDRESS << 1) | TWI_READ) != 0) {
        twi_stop();
        return 1;
    }
    
    data[0] = twi_read(TWI_ACK);  // MSB
    data[1] = twi_read(TWI_ACK);  // LSB
    data[2] = twi_read(TWI_NACK); // CRC
    twi_stop();
    
    // Validate CRC
    crc_calc = sgp41_calculate_crc(data, 2);
    if (crc_calc != data[2]) {
        return 2;
    }
    
    *sraw_voc = ((uint16_t)data[0] << 8) | data[1];
    return 0;
}

uint8_t sgp41_measure_raw_signals(uint16_t rh_comp, uint16_t t_comp,
                                   uint16_t *sraw_voc, uint16_t *sraw_nox)
{
    uint8_t data[6];
    uint8_t params[6];
    uint8_t crc_calc;
    
    // Prepare RH parameter with CRC
    params[0] = (uint8_t)(rh_comp >> 8);
    params[1] = (uint8_t)(rh_comp & 0xFF);
    params[2] = sgp41_calculate_crc(params, 2);
    
    // Prepare temperature parameter with CRC
    params[3] = (uint8_t)(t_comp >> 8);
    params[4] = (uint8_t)(t_comp & 0xFF);
    params[5] = sgp41_calculate_crc(&params[3], 2);
    
    // Send measure command with parameters
    twi_start();
    if (twi_write((SGP41_I2C_ADDRESS << 1) | TWI_WRITE) != 0) {
        twi_stop();
        return 1;
    }
    
    twi_write(SGP41_CMD_MEASURE_RAW_H);
    twi_write(SGP41_CMD_MEASURE_RAW_L);
    
    for (uint8_t i = 0; i < 6; i++) {
        twi_write(params[i]);
    }
    twi_stop();
    
    // Wait for measurement
    _delay_ms(SGP41_MEASURE_DELAY_MS);
    
    // Read VOC and NOx results (2x(2 bytes + 1 CRC) = 6 bytes)
    twi_start();
    if (twi_write((SGP41_I2C_ADDRESS << 1) | TWI_READ) != 0) {
        twi_stop();
        return 1;
    }
    
    for (uint8_t i = 0; i < 6; i++) {
        if (i == 5) {
            data[i] = twi_read(TWI_NACK);
        } else {
            data[i] = twi_read(TWI_ACK);
        }
    }
    twi_stop();
    
    // Validate VOC CRC
    crc_calc = sgp41_calculate_crc(data, 2);
    if (crc_calc != data[2]) {
        return 2; // VOC CRC error
    }
    
    // Validate NOx CRC
    crc_calc = sgp41_calculate_crc(&data[3], 2);
    if (crc_calc != data[5]) {
        return 2; // NOx CRC error
    }
    
    *sraw_voc = ((uint16_t)data[0] << 8) | data[1];
    *sraw_nox = ((uint16_t)data[3] << 8) | data[4];
    
    return 0;
}

