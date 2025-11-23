#ifndef RTC_H
    #define RTC_H
#endif  
// #include <avr/io.h>
#include <stdint.h>

/* I2C address of DS1307 RTC */
#define RTC_ADDRESS 0x68

/* RTC register addresses (DS1307) */
#define RTC_SEC     0x00  /* Seconds (and CH bit) */
#define RTC_MIN     0x01  /* Minutes */
#define RTC_HOUR    0x02  /* Hours (12/24 bit) */
#define RTC_DAY     0x03  /* Day of week */
#define RTC_DATE    0x04  /* Date (day of month) */
#define RTC_MONTH   0x05  /* Month */
#define RTC_YEAR    0x06  /* Year (00..99) */
#define RTC_CONTROL 0x07  /* Control register (square-wave output) */

/*
 * rtc_setup
 *  - Initialize RTC device state: start oscillator (clear CH bit),
 *    select 24-hour mode (clear 12/24 bit) and clear control register.
 *  - No parameters, returns void.
 */
void rtc_setup(void);

/*
 * rtc_read_reg
 *  - Read single RTC register over I2C.
 *  - reg_addr : register address (use RTC_* macros)
 *  - returns  : byte read from device
 */
uint8_t rtc_read_reg(uint8_t reg_addr);


/*
 * rtc_write_reg
 *  - Write single RTC register over I2C.
 *  - reg_addr : register address (use RTC_* macros)
 *  - data     : byte to write
 *  - returns  : result code from underlying TWI write (0 = success typically)
 */
uint8_t rtc_write_reg(uint8_t reg_addr, uint8_t data);

/*
 * rtc_set_time
 *  - Set RTC time (hours, minutes, seconds) in decimal (0..23, 0..59, 0..59).
 *  - Values are converted to BCD before writing.
 *  - returns  : 0 on success (function currently always returns 0)
 */
uint8_t rtc_set_time(uint8_t hours, uint8_t minutes, uint8_t seconds);

/*
 * rtc_get_time
 *  - Read current time from RTC and convert BCD -> decimal.
 *  - hours, minutes, seconds : pointers to store values (non-NULL)
 *  - returns : 0 on success
 */
uint8_t rtc_get_time(uint8_t *hours, uint8_t *minutes, uint8_t *seconds);

/*
 * rtc_set_date
 *  - Set RTC calendar date (day-of-week, date, month, year) in decimal.
 *  - day   : 1..7 (day of week), date: 1..31, month: 1..12, year: 0..99
 *  - Values are converted to BCD before writing.
 *  - returns : 0 on success
 */
uint8_t rtc_set_date(uint8_t day, uint8_t date, uint8_t month, uint8_t year);
