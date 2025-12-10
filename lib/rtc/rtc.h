#ifndef RTC_H
#define RTC_H

// this library was created with help from GithubCopilot suggestions


// #include <avr/io.h>
#include <stdint.h>


/** @brief I2C address of DS1307/DS3231 RTC */
#define RTC_ADDRESS 0x68


/**
 * @defgroup RTCRegisters RTC Register Addresses
 * @{
 */
#define RTC_SEC     0x00  /* Seconds (and CH bit) */
#define RTC_MIN     0x01  /* Minutes */
#define RTC_HOUR    0x02  /* Hours (12/24 bit) */
#define RTC_DAY     0x03  /* Day of week */
#define RTC_DATE    0x04  /* Date (day of month) */
#define RTC_MONTH   0x05  /* Month */
#define RTC_YEAR    0x06  /* Year (00..99) */
#define RTC_CONTROL 0x07  /* Control register (square-wave output) */
/** @} */


/**
 * @defgroup RTCArrayIndices Indices for rtc_register array
 * @{
 */
#define     SECONDS         rtc_register[0]
#define     MINUTES         rtc_register[1]
#define     HOURS           rtc_register[2]
#define     DAY             rtc_register[3]
#define     DATE            rtc_register[4]
#define     MONTH           rtc_register[5]
#define     YEAR            rtc_register[6]
/** @} */


/** @brief Buffer for reading all RTC registers. */
unsigned char rtc_register[7];

/** @brief Global variables to store time in FAT32 format. */
unsigned int dateFAT, timeFAT;


/**
 * @brief  Initialize RTC device state.
 *         Starts oscillator (clears CH bit), selects 24-hour mode, and clears control register.
 */
void rtc_setup(void);


/**
 * @brief  Read single RTC register via I2C.
 * @param  reg_addr Register address (use RTC_* macros).
 * @return Byte read from device.
 */
uint8_t rtc_read_reg(uint8_t reg_addr);


/**
 * @brief  Write single RTC register via I2C.
 * @param  reg_addr Register address (use RTC_* macros).
 * @param  data     Byte to write.
 * @return Result code from TWI write (0 = success).
 */
uint8_t rtc_write_reg(uint8_t reg_addr, uint8_t data);


/**
 * @brief  Set RTC time (decimal values).
 * @param  hours   Hours (0-23).
 * @param  minutes Minutes (0-59).
 * @param  seconds Seconds (0-59).
 * @return 0 on success.
 */
uint8_t rtc_set_time(uint8_t hours, uint8_t minutes, uint8_t seconds);


/**
 * @brief  Read current time from RTC.
 * @param  hours   Pointer to store hours.
 * @param  minutes Pointer to store minutes.
 * @param  seconds Pointer to store seconds.
 * @return 0 on success.
 */
uint8_t rtc_get_time(uint8_t *hours, uint8_t *minutes, uint8_t *seconds);


/**
 * @brief  Set RTC calendar date (decimal values).
 * @param  day   Day of week (1-7).
 * @param  date  Day of month (1-31).
 * @param  month Month (1-12).
 * @param  year  Year (0-99).
 * @return 0 on success.
 */
uint8_t rtc_set_date(uint8_t day, uint8_t date, uint8_t month, uint8_t year);


/**
 * @brief  Read current date from RTC.
 * @param  date  Pointer to store day of month.
 * @param  month Pointer to store month.
 * @param  year  Pointer to store year.
 * @return 0 on success.
 */
uint8_t rtc_get_date(uint8_t *date, uint8_t *month, uint8_t *year);


/**
 * @brief  Read day of week from RTC.
 * @param  day Pointer to store day of week.
 * @return 0 on success.
 */
uint8_t rtc_get_day(uint8_t *day);


/**
 * @brief  Read RTC and convert current time/date to FAT32 format.
 *         Updates global variables dateFAT and timeFAT.
 * @return 0 on success, 1 on error.
 */
unsigned char getDateTime_FAT(void);


#endif /* RTC_H */
