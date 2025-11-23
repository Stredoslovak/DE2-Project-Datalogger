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
#include <si4703.h>

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

// volatile uint8_t si4703_regs[2]; // SI4703 registers
 uint16_t si4703_all_regs[16] = {0};
// -- Function definitions ---------------------------------
/*
 * Function: Main function where the program execution begins
 * Purpose:  Call function to test all I2C (TWI) combinations
 *           and send detected devices to UART.
 * Returns:  none
 */

 // PB0 - 8 - SEN
 // PB1 - 9 - RST
volatile uint8_t rds_flag = 0;
volatile uint8_t rds_ready_flag = 0;
uint8_t BLER[4] = {0};
volatile uint8_t interupt_reached = 0;
volatile uint8_t rds_poll = 0;
static void setup_ext_int(void)
{
    // PD2 = INT0
    gpio_mode_input_nopull(&DDRD, 2); // nastav PD2 ako vstup s pull-up (pouzi gpio.h)
    EICRA &= ~((1 << ISC00)); // vycistit bity pre INT0/INT1 v EICRA
    EICRA |= (1 << ISC01);
    // EICRA |= (1 << ISC01); // ISC01 = 1, ISC00 = 0 -> falling edge
    EIMSK |= (1 << INT0); // povolit externy interrupt INT0

    // PCICR |= (1 << PCIE0); // povolit pin change interrupt pre PCINT[7:0] (PORTB)
    // PCMSK0 |= (1 << PCINT2); // povolit pin change interrupt
}

static void rds_check_group_and_dump(uint16_t A, uint16_t B, uint16_t C, uint16_t D)
{
    uint8_t group = (B >> 12) & 0x0F;        // bits 15..12
    uint8_t version = (B >> 11) & 0x01;      // bit 11: 0=A, 1=B

    char line[120];
    // sprintf(line, "RDS group=%u%s (B=0x%04X)\r\n", group, version==0 ? "A" : "B", B);
    // uart_puts(line);

    if (group == 4 && version == 0) {
        /* Decode CT (clock/time) - Group 4A
           Assumptions (standard layout used by many RDS parsers):
            - Block C (16 bits) = Modified Julian Day (MJD)
            - Block D: bits 15..11 = hour (5 bits, 0..23)
                       bits 10..5  = minute (6 bits, 0..59)
                       bits 4..0   = local offset in 0.5 hour units (5-bit signed two's complement)
        */

        uint32_t mjd = (((uint32_t)(B & 0x0003)) << 15) | (((uint32_t)C >> 1) & 0x7FFF);
        uint8_t hour = (uint8_t)(((C & 0x0001) << 4) | ((D >> 12) & 0x0F));
        uint8_t minute = (uint8_t)((D >> 6) & 0x3F);
        int8_t offset_half = (int8_t)(D & 0x3F);
        /* convert 5-bit signed (two's complement) to signed integer */
        if (offset_half & 0x10) offset_half = offset_half - 32;

        /* compute local minutes and day adjustment */
        int utc_minutes = hour * 60 + minute;
        int local_minutes = utc_minutes + (offset_half * 30); // offset in minutes (0.5h = 30min)
        int day_adj = 0;
        while (local_minutes < 0) { local_minutes += 24*60; day_adj -= 1; }
        while (local_minutes >= 24*60) { local_minutes -= 24*60; day_adj += 1; }

        /* Convert MJD -> Gregorian date (algorithm via JDN) */
        int jdn = (int)mjd + 2400001; /* JDN approximation suitable for conversion */
        int l = jdn + 68569;
        int n = (4 * l) / 146097;
        l = l - (146097 * n + 3) / 4;
        int i = (4000 * (l + 1)) / 1461001;
        l = l - (1461 * i) / 4 + 31;
        int j = (80 * l) / 2447;
        int day = l - (2447 * j) / 80;
        l = j / 11;
        int month = j + 2 - 12 * l;
        int year = 100 * (n - 49) + i + l;

        /* apply day adjustment if local time rolled over a day boundary */
        if (day_adj != 0) {
            int jdn_adj = jdn + day_adj;
            l = jdn_adj + 68569;
            n = (4 * l) / 146097;
            l = l - (146097 * n + 3) / 4;
            i = (4000 * (l + 1)) / 1461001;
            l = l - (1461 * i) / 4 + 31;
            j = (80 * l) / 2447;
            day = l - (2447 * j) / 80;
            l = j / 11;
            month = j + 2 - 12 * l;
            year = 100 * (n - 49) + i + l;
        }

        sprintf(line,
            "CT 4A: MJD=%u -> %04d-%02d-%02d UTC %02u:%02u offset=%+d*0.5h local %02d:%02d\r\n",
            mjd, year, month, day, hour, minute, offset_half,
            local_minutes / 60, local_minutes % 60);
        uart_puts(line);
        sprintf(line, "MJD=0x%08X (Hour=0x%04X | Minute=0x%04x | Offset=0x%04X)\r\n", mjd,hour, minute, offset_half);
        uart_puts(line);
        sprintf(line, "MJD=%u (Hour=%d | Minute=%d | Offset=%d)\r\n", (int)mjd,hour, minute, offset_half);
        uart_puts(line);
    } else {
        // sprintf(line, "Not CT (C=0x%04X D=0x%04X)\r\n", C, D);
        // uart_puts(line);
    }
}

int main(void)
{
    // char uart_msg[10];
    // gpio_mode_output(&DDRB, 2); //sync pin

    uart_init(UART_BAUD_SELECT(115200, F_CPU));
    si4703_init_i2c(&DDRB,&PORTB, 1, &DDRB,&PORTB, 0, &DDRC,&PORTC, 4, 5);
    
    // si4703_init_i2c(&DDRB)

    // I2C Scanner
    sei();  // Needed for UART
    uart_puts("Scanning I2C... ");
    twi_init();
    setup_ext_int();
    si4703_readRegs((uint8_t)SI4703_ADDR, si4703_all_regs);
    si4703_setRegs(si4703_all_regs);
    // for (uint8_t sla = 0; sla < 128; sla++) {
    //     if (twi_test_address(sla) == 0) {  // If ACK from Slave
    //         sprintf(uart_msg, "0x%x ", sla);
    //         uart_puts(uart_msg);
            
    //     }
    // }
    uart_puts(" Done\r\n");
    // uart_puts("\r\n");

    twi_writeto_mem(RTC_ADDRESS, RTC_SEC, 0x00);
    twi_writeto_mem(RTC_ADDRESS, RTC_MIN, 0x00);
    twi_writeto_mem(RTC_ADDRESS, RTC_HOUR, 0x00); // 24-hour format
    twi_writeto_mem(RTC_ADDRESS, 0x07, 0b00010011); // output 32khz



    _delay_ms(500);
    if (twi_test_address(RTC_ADDRESS) != 0) {
        uart_puts("[\x1b[31;1mERROR\x1b[0m] I2C device not detected\r\n");
        while (1);  // Cycle here forever
    }
    else
    {
        uart_puts("[\x1b[32;1mRTC_OK\x1b[0m]");
    }
    
    _delay_ms(500);
    tim1_ovf_1sec();
    tim1_ovf_enable();

    {
        // uint16_t si4703_all_regs[16] = {0};
        char uart_line[40];

        // Použi SI4703_ADDR definíciu z hlavičky
        si4703_readRegs((uint8_t)SI4703_ADDR, si4703_all_regs);

        uart_puts("\r\nSI4703 register map:\r\n");
        for (uint8_t a = 0; a < 16; a++) {
            sprintf(uart_line, "0x%02X: 0x%04X\r\n", a, si4703_all_regs[a]);
            uart_puts(uart_line);
        }
    }
    
{
        // uint16_t si4703_all_regs[16] = {0};
        char uart_line[40];

        // Použi SI4703_ADDR definíciu z hlavičky
        // gpio_write_high(&PORTB, 2); // sync high
        si4703_readRegs((uint8_t)SI4703_ADDR, si4703_all_regs);

        uart_puts("\r\nSI4703 register map after write 2:\r\n");
        for (uint8_t a = 0; a < 16; a++) {
            sprintf(uart_line, "0x%02X: 0x%04X\r\n", a, si4703_all_regs[a]);
            uart_puts(uart_line);
        }
    }
    // si4703_tuneTo(876, si4703_all_regs); // Radio Brno nefunguje
    // si4703_tuneTo(883, si4703_all_regs); // Radio Kiss
    // si4703_tuneTo(10, si4703_all_regs); // Radio FM4
    si4703_tuneTo(904, si4703_all_regs); // Vltava
    si4703_setVol(5, si4703_all_regs); // Volume 8

    if (si4703_askForRDS(si4703_all_regs))
    {
        char uart_line[40];
    sprintf(uart_line, "\r\nRDS requested \r\n");
    uart_puts(uart_line);
    rds_flag = 1;
    }
    else
    {
        char uart_line[40];
    sprintf(uart_line, "\r\nRDS not failed");
    uart_puts(uart_line);
    }
    
    

    while (1) {
        if (rds_poll)
        {
           si4703_readRegs((uint8_t)SI4703_ADDR, si4703_all_regs);
        }
        
        if (interupt_reached)
        {
            interupt_reached = 0;
            uart_puts("\r\nrdsInt\r\n");
        }
        
        if (rds_ready_flag)
        {
            
            
            si4703_readRegs((uint8_t)SI4703_ADDR, si4703_all_regs);
            // si4703_clearRDSRequest(si4703_all_regs);

            BLER[0] = ((si4703_all_regs[0x0A] >> 9) & ((1u << 2) - 1));
            BLER[1] = ((si4703_all_regs[0x0B] >> 14) & ((1u << 2) - 1));
            BLER[2] = ((si4703_all_regs[0x0B] >> 12) & ((1u << 2) - 1));
            BLER[3] = ((si4703_all_regs[0x0B] >> 10) & ((1u << 2) - 1));
            // char uart_line[40];

            // uart_puts("\r\nRDS BLOCKS BER: ");
            // for (uint8_t i = 0; i < 4; i++)
            // {
            //     sprintf(uart_line, "%d ", BLER[i]);
            //     uart_puts(uart_line);
            // }
            if (BLER[0] <= 0 && BLER[1] <= 0 && BLER[2] <= 0 && BLER[3] <= 0)
            {
                rds_check_group_and_dump(
                    si4703_all_regs[0x0C],
                    si4703_all_regs[0x0D],
                    si4703_all_regs[0x0E],
                    si4703_all_regs[0x0F]
                );
            }
            rds_ready_flag = 0;
        }
        
        if (flag_update_uart == 60)
        {
            flag_update_uart = 0;
            uart_puts("\r\n Heartbeat\r\n");
            // sprintf(uart_msg, "\r\n RTC TIME hex: 0x%x,0x%x,0x%x", rtc_time[2], rtc_time[1], rtc_time[0]);
            // uart_puts(uart_msg);
            // sprintf(uart_msg, "\r\n RTC TIME dec: %d,%d,%d", rtc_time[2], rtc_time[1], rtc_time[0]);
            // uart_puts(uart_msg);
            // sprintf(uart_msg, "\r\n RTC CAL hex: %d,%d,%d", rtc_calendar[2], rtc_calendar[1], rtc_calendar[0]);
            // uart_puts(uart_msg);
            // sprintf(uart_msg, "\r\n RTC CNTRL hex: %x", rtc_control);
            // uart_puts(uart_msg);

            // sprintf(uart_msg, "\r\n SI4703 REG 0x07: %x,%x", si4703_regs[1], si4703_regs[0]);
            // uart_puts(uart_msg);
             // uint16_t si4703_all_regs[16] = {0};
        char uart_line[40];
        // if (rds_flag)
        // {
        //     // uart_puts("\r\nRDS requested\r\n");
        //     rds_check_group_and_dump(
        //     si4703_all_regs[0x0C],
        //     si4703_all_regs[0x0D],
        //     si4703_all_regs[0x0E],
        //     si4703_all_regs[0x0F]
        // );
        // }
        
        // Použi SI4703_ADDR definíciu z hlavičky
        // gpio_write_high(&PORTB, 2); // sync high
        
        uart_puts("\r\nSI4703 register map after write:\r\n");
        for (uint8_t a = 0; a < 16; a++) {
            sprintf(uart_line, "0x%02X: 0x%04X\r\n", a, si4703_all_regs[a]);
            uart_puts(uart_line);
        }
        // sprintf(uart_line, "\r\n Current frequency: %d.%d MHz\r\n", si4703_getFreq(si4703_all_regs)/10, si4703_getFreq(si4703_all_regs)%10);
        // uart_puts(uart_line);
        // sprintf(uart_line, " Current volume: %d\r\n", si4703_getVolume(si4703_all_regs));
        // uart_puts(uart_line);

        
        }
        
        
    }

    return 0;
}

ISR(TIMER1_OVF_vect)
{
    // twi_readfrom_mem_into(RTC_ADDRESS, RTC_SEC, rtc_time, 3);
    // twi_readfrom_mem_into(RTC_ADDRESS, RTC_CALENDAR, rtc_calendar, 3);
    // twi_readfrom_mem_into(RTC_ADDRESS, 0x07, &rtc_control, 1);
    // twi_readfrom_mem_into(0x10, 0x07, si4703_regs, 2);
    
    // if (rds_flag && (((si4703_all_regs[0x0A] >> 15) & ((1u))) == 1))
    // {
    //     rds_ready_flag = 1;
    // }
    // if (rds_flag)
    // {
    //     rds_poll = 1;
    // }
    
    flag_update_uart += 1;
}   

ISR(INT0_vect)
{
    cli();
    rds_ready_flag = 1;
    // rds_flag = 0;
    // interupt_reached = 1;
    // uart_puts("\r\nRDS INT");
    sei();
}