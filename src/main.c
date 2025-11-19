/*
 * The I2C (TWI) bus scanner tests all addresses and detects devices
 * that are connected to the SDA and SCL signals.
 * (c) 2023-2025 Tomas Fryza, MIT license
 *
 * Developed using PlatformIO and Atmel AVR platform.
 * Tested on Arduino Uno board and ATmega328P, 16 MHz.
 */

// -- Includes ---------------------------------------------
#include <avr/io.h>         // AVR device-specific IO definitions
#include <avr/interrupt.h>  // Interrupts standard C library for AVR-GCC
#include <twi.h>            // I2C/TWI library for AVR-GCC
#include <uart.h>           // Peter Fleury's UART library
#include <stdio.h>          // C library. Needed for `sprintf`
#include "timer.h"
#include "gpio.h"
#include <util/delay.h>

//-- Definitions -------------------------------------------
#define RTC_ADDRESS 0x68  // I2C slave address of RTC DS1307
#define FLASH_ADDRESS 0x50 // I2C slave address of 24LC256 EEPROM
#define RTC_SEC 0x00    // RTC register address for "seconds"
#define RTC_MIN 0x01    // RTC register address for "minutes"
#define RTC_HOUR 0x02   // RTC register address for "hours"
#define RTC_CALENDAR 0x03 // RTC register address for "day of the week"

volatile uint8_t flag_update_uart = 0;
volatile uint8_t rtc_time[3];
volatile uint8_t rtc_calendar[3];
volatile uint8_t rtc_control;
volatile uint8_t startRTC = 0; // Start the RTC oscillator

// -- Function definitions ---------------------------------
/*
 * Function: Main function where the program execution begins
 * Purpose:  Call function to test all I2C (TWI) combinations
 *           and send detected devices to UART.
 * Returns:  none
 */

 // PB1 - 9 - SEN
 // PB0 - 8 - RST
int main(void)
{
    char uart_msg[10];

    // Initialize USART to asynchronous, 8-N-1, 115200 Bd
    // NOTE: Add `monitor_speed = 115200` to `platformio.ini`
    uart_init(UART_BAUD_SELECT(115200, F_CPU));

    sei();  // Needed for UART

    gpio_mode_output(&DDRB, 1); // SEN pin
    gpio_mode_output(&DDRB, 0); // RST pin
    gpio_mode_output(&DDRC, 4); // SDIO or SDA pin

    gpio_write_low(&PORTC, 4); // SDA low
    gpio_write_low(&PORTB, 1); // RST low
    gpio_write_high(&PORTB, 0); // SEN high
    _delay_ms(1);
    gpio_write_high(&PORTB, 1); // RST high
    _delay_ms(1);
    // I2C Scanner
    uart_puts("Scanning I2C... ");
    twi_init();

    for (uint8_t sla = 0; sla < 128; sla++) {
        if (twi_test_address(sla) == 0) {  // If ACK from Slave
            sprintf(uart_msg, "0x%x ", sla);
            uart_puts(uart_msg);
            
        }
    }
    uart_puts(" Done\r\n");
    // uart_puts("\r\n");

    twi_writeto_mem(RTC_ADDRESS, RTC_SEC, 0x00);
    twi_writeto_mem(RTC_ADDRESS, RTC_MIN, 0x00);
    twi_writeto_mem(RTC_ADDRESS, RTC_HOUR, 0x00); // 24-hour format
    twi_writeto_mem(RTC_ADDRESS, 0x07, 0b00010011); // output 32khz
    if (twi_test_address(RTC_ADDRESS) != 0) {
        uart_puts("[\x1b[31;1mERROR\x1b[0m] I2C device not detected\r\n");
        while (1);  // Cycle here forever
    }
    else
    {
        uart_puts("[\x1b[32;1mRTC_OK\x1b[0m]");
    }
    

    // Set Timer 1
    // WRITE YOUR CODE HERE
    tim1_ovf_1sec();
    tim1_ovf_enable();


    // Update "minutes" and "hours" in RTC
    // WRITE YOUR CODE HERE
    
    // Test OLED display
    // WRITE YOUR CODE HERE

    while (1) {
        if (flag_update_uart)
        {
            flag_update_uart = 0;
            sprintf(uart_msg, "\r\n RTC TIME hex: 0x%x,0x%x,0x%x", rtc_time[2], rtc_time[1], rtc_time[0]);
            uart_puts(uart_msg);
            sprintf(uart_msg, "\r\n RTC TIME dec: %d,%d,%d", rtc_time[2], rtc_time[1], rtc_time[0]);
            uart_puts(uart_msg);
            sprintf(uart_msg, "\r\n RTC CAL hex: %d,%d,%d", rtc_calendar[2], rtc_calendar[1], rtc_calendar[0]);
            uart_puts(uart_msg);
            sprintf(uart_msg, "\r\n RTC CNTRL hex: %x", rtc_control);
            uart_puts(uart_msg);
        }
    }

    return 0;
}

ISR(TIMER1_OVF_vect)
{
    twi_readfrom_mem_into(RTC_ADDRESS, RTC_SEC, rtc_time, 3);
    twi_readfrom_mem_into(RTC_ADDRESS, RTC_CALENDAR, rtc_calendar, 3);
    twi_readfrom_mem_into(RTC_ADDRESS, 0x07, &rtc_control, 1);
    flag_update_uart = 1;
}   