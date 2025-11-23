#include "rtc.h"
#include <twi.h>


#define RTC_ADDRESS 0x68  // I2C slave address of RTC DS1307

//adresy registrov
#define RTC_SEC 0x00    // RTC register address for "seconds"
#define RTC_MIN 0x01    // RTC register address for "minutes"
#define RTC_HOUR 0x02   // RTC register address for "hours"
#define RTC_DAY 0x03
#define RTC_DATE 0x04
#define RTC_MONTH 0x05
#define RTC_YEAR 0x06
#define RTC_CONTROL 0x07

// -- Functions --------------------------------------------
static inline uint8_t decimal_to_bcd(uint8_t value)
{
    return (uint8_t)((value / 10) << 4) | (value % 10);
}

static inline uint8_t bcd_to_decimal(uint8_t bcd)
{
    return (uint8_t)(((bcd >> 4) * 10) + (bcd & 0x0F));
}

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

uint8_t rtc_read_reg(uint8_t reg_addr)
{
    uint8_t data = 0;
    twi_readfrom_mem_into(RTC_ADDRESS, reg_addr, &data, 1);
    return data;
}

uint8_t rtc_write_reg(uint8_t reg_addr, uint8_t data)
{
    return twi_writeto_mem(RTC_ADDRESS, reg_addr, data);
}

uint8_t rtc_set_time(uint8_t hours, uint8_t minutes, uint8_t seconds)
{
    rtc_write_reg(RTC_HOUR, decimal_to_bcd(hours));
    rtc_write_reg(RTC_MIN, decimal_to_bcd(minutes));
    rtc_write_reg(RTC_SEC, decimal_to_bcd(seconds));
    return 0;
}

uint8_t rtc_get_time(uint8_t *hours, uint8_t *minutes, uint8_t *seconds)
{
    uint8_t sec_bcd = rtc_read_reg(RTC_SEC);
    uint8_t min_bcd = rtc_read_reg(RTC_MIN);
    uint8_t hour_bcd = rtc_read_reg(RTC_HOUR);

    *seconds = bcd_to_decimal(sec_bcd); // CH bit je stale nulovy takze netreba maskovat
    *minutes = bcd_to_decimal(min_bcd);
    *hours = bcd_to_decimal(hour_bcd); // netreba maskovat ani 24h mod lebo ten bit 6 je stale nulovy

    return 0;
}

uint8_t rtc_set_date(uint8_t day, uint8_t date, uint8_t month, uint8_t year)
{
    rtc_write_reg(RTC_DAY, decimal_to_bcd(day));
    rtc_write_reg(RTC_DATE, decimal_to_bcd(date));
    rtc_write_reg(RTC_MONTH, decimal_to_bcd(month));
    rtc_write_reg(RTC_YEAR, decimal_to_bcd(year));
    return 0;
}