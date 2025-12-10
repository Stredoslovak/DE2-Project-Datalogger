#include "rtc.h"
#include <twi.h>

// this library was created with help from GithubCopilot suggestions

#define RTC_ADDRESS 0x68  // I2C slave address of RTC DS3231


//adresy registrov
#define RTC_SEC 0x00    // RTC register address for "seconds"
#define RTC_MIN 0x01    // RTC register address for "minutes"
#define RTC_HOUR 0x02   // RTC register address for "hours"
#define RTC_DAY 0x03
#define RTC_DATE 0x04
#define RTC_MONTH 0x05
#define RTC_YEAR 0x06
// #define RTC_CONTROL 0x07


// -- Functions --------------------------------------------

/**
 * @brief  Convert decimal value to Binary-Coded Decimal (BCD).
 * @param  value Decimal value (0-99).
 * @return BCD encoded value.
 */
static inline uint8_t decimal_to_bcd(uint8_t value)
{
    return (uint8_t)((value / 10) << 4) | (value % 10);
}


/**
 * @brief  Convert Binary-Coded Decimal (BCD) to decimal value.
 * @param  bcd BCD encoded value.
 * @return Decimal value.
 */
static inline uint8_t bcd_to_decimal(uint8_t bcd)
{
    return (uint8_t)(((bcd >> 4) * 10) + (bcd & 0x0F));
}


/**
 * @brief  Initialize RTC oscillator and set 24-hour mode (unused).
 * @return none
 */
void rtc_setup(void)
{   
    //start oscilator and select 24-hour mode
    uint8_t sec_reg = rtc_read_reg(RTC_SEC);
    uint8_t hour_reg = rtc_read_reg(RTC_HOUR);
    hour_reg &= ~(1 << 6); // Clear 12/24 bit to select 24-hour mode
    sec_reg &= ~(1 << 7); // Clear CH (Clock Halt) bit to start the oscillator
    rtc_write_reg(RTC_SEC, sec_reg);
    rtc_write_reg(RTC_HOUR, hour_reg);
    rtc_write_reg(RTC_CONTROL, 0x00); // set the control register to disable oscillator output
}


/**
 * @brief  Read one RTC register via I2C.
 * @param  reg_addr RTC register address.
 * @return Register value.
 */
uint8_t rtc_read_reg(uint8_t reg_addr)
{
    uint8_t data = 0;
    twi_readfrom_mem_into(RTC_ADDRESS, reg_addr, &data, 1);
    return data;
}


/**
 * @brief  Write one RTC register via I2C.
 * @param  reg_addr RTC register address.
 * @param  data     Value to write.
 * @return I2C status (0 = success).
 */
uint8_t rtc_write_reg(uint8_t reg_addr, uint8_t data)
{
    return twi_writeto_mem(RTC_ADDRESS, reg_addr, data);
}


/**
 * @brief  Set current time in RTC.
 * @param  hours   Hours (0-23).
 * @param  minutes Minutes (0-59).
 * @param  seconds Seconds (0-59).
 * @return 0 on success.
 */
uint8_t rtc_set_time(uint8_t hours, uint8_t minutes, uint8_t seconds)
{
    rtc_write_reg(RTC_HOUR, decimal_to_bcd(hours));
    rtc_write_reg(RTC_MIN, decimal_to_bcd(minutes));
    rtc_write_reg(RTC_SEC, decimal_to_bcd(seconds));
    return 0;
}


/**
 * @brief  Read current time from RTC.
 * @param  hours   Output for hours.
 * @param  minutes Output for minutes.
 * @param  seconds Output for seconds.
 * @return 0 on success.
 */
uint8_t rtc_get_time(uint8_t *hours, uint8_t *minutes, uint8_t *seconds)
{
    uint8_t sec_bcd = rtc_read_reg(RTC_SEC);
    uint8_t min_bcd = rtc_read_reg(RTC_MIN);
    uint8_t hour_bcd = rtc_read_reg(RTC_HOUR);


    *seconds = bcd_to_decimal(sec_bcd);
    *minutes = bcd_to_decimal(min_bcd);
    *hours = bcd_to_decimal(hour_bcd);


    return 0;
}


/**
 * @brief  Read current date from RTC.
 * @param  date  Output for day of month (1-31).
 * @param  month Output for month (1-12).
 * @param  year  Output for year (0-99).
 * @return 0 on success.
 */
uint8_t rtc_get_date(uint8_t *date, uint8_t *month, uint8_t *year)
{
    uint8_t date_bcd = rtc_read_reg(RTC_DATE);
    uint8_t month_bcd = rtc_read_reg(RTC_MONTH);
    uint8_t year_bcd = rtc_read_reg(RTC_YEAR);


    *date = bcd_to_decimal(date_bcd);
    *month = bcd_to_decimal(month_bcd);
    *year = bcd_to_decimal(year_bcd);


    return 0;
}


/**
 * @brief  Read day of week from RTC.
 * @param  day Output for day of week (1-7, Sunday=1).
 * @return 0 on success.
 */
uint8_t rtc_get_day(uint8_t *day)
{
    uint8_t day_bcd = rtc_read_reg(RTC_DAY);
    *day = bcd_to_decimal(day_bcd);
    return 0;
}


/**
 * @brief  Set date and day of week in RTC.
 * @param  day   Day of week (1-7, Sunday=1).
 * @param  date  Day of month (1-31).
 * @param  month Month (1-12).
 * @param  year  Year (0-99).
 * @return 0 on success.
 */
uint8_t rtc_set_date(uint8_t day, uint8_t date, uint8_t month, uint8_t year)
{
    rtc_write_reg(RTC_DAY, decimal_to_bcd(day));
    rtc_write_reg(RTC_DATE, decimal_to_bcd(date));
    rtc_write_reg(RTC_MONTH, decimal_to_bcd(month));
    rtc_write_reg(RTC_YEAR, decimal_to_bcd(year));
    return 0;
}


/**
 * @brief  Read all RTC time/date registers (7 bytes) at once.
 * @return 0 on success.
 */
uint8_t RTC_read(void)
{
    twi_readfrom_mem_into(RTC_ADDRESS, RTC_SEC, &rtc_register[0], 7);
    return 0;
}


/**
 * @brief  Get current date/time and convert to FAT32 format.
 * @return 0 on success, 1 on error.
 */
unsigned char getDateTime_FAT(void)
{
    uint8_t date, month, year;
    uint8_t hours, minutes, seconds;
    uint8_t error;


    error = rtc_get_date(&date, &month, &year);
    if(error) return 1;
    
    error = rtc_get_time(&hours, &minutes, &seconds);
    if(error) return 1;


    // Calculate year for FAT (years since 1980)
    unsigned int yr = year + 2000 - 1980;
    dateFAT = yr;


    // Add month
    dateFAT = (dateFAT << 4) | month;


    // Add date
    dateFAT = (dateFAT << 5) | date;


    // Add hours
    timeFAT = hours;


    // Add minutes
    timeFAT = (timeFAT << 6) | minutes;


    // Add seconds (FAT32 uses 2-second resolution)
    timeFAT = (timeFAT << 5) | (seconds / 2);


    return 0;
}
