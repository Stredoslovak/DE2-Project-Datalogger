// this code was created with help from GithubCopilot suggestions


// -- Includes ---------------------------------------------
#include <avr/io.h>         // AVR device-specific IO definitions
#include <avr/interrupt.h>  // Interrupts standard C library for AVR-GCC
#include "timer.h"          // Timer library for AVR-GCC
#include <uart.h>           // Peter Fleury's UART library
#include <stdlib.h>         // C library. Needed for number conversions
#include "SPI_routines.h"
#include "sd_routines.h"
#include "FAT32.h"
#include "gpio.h"
#include "rtc.h"
#include <twi.h>
#include "bme280.h"
#include "SensirionI2CSgp41.h"
#include <math.h>
#include <string.h>
#include <util/delay.h>
#include <stdio.h>


#define LOG_TIME_INTERVAL_SEC 5
#define UART_ON
// #define UART_DEBUG
#define SD_write
// #define UPDATE_RTC_TIME_COMPILE
#define DAY_NUMBER 2 // 1=Sunday ... 7=Saturday



#define ACTIVITY_LED_PORT   PORTC
#define L_ACT    2
#define STATUS_LED_PORT     PORTC
#define L_STATUS      1
#define ERROR_LED_PORT      PORTC
#define L_ERROR       0





volatile uint8_t printRTC = 0;
volatile uint8_t counterTim1 = 0;


uint8_t SD_OK, FS_OK, BM_OK, SGP_OK, RTC_OK = 0;


int bme_init_simple(struct bme280_dev *dev, uint8_t *dev_addr_ptr);
int bme_read_once(struct bme280_dev *dev, int32_t *t100, uint32_t *press_pa, uint32_t *hum_x1024);
int sgp41_init_simple(void);
int sgp41_measure_once(int32_t *voc_index, int32_t *nox_index);


volatile uint8_t measurement_flag = 0;


/**
 * @brief  Convert month ASCII abbreviation sum to month number.
 * @param  sum Sum of ASCII values of 3-letter month abbreviation.
 * @return Month number (1-12) or 0 on error.
 */
uint8_t get_month_from_ascii_sum(uint16_t sum) {
    switch(sum) {
        case 268: return 12; // Dec
        case 269: return 2;  // Feb
        case 281: return 1;  // Jan
        case 285: return 8;  // Aug
        case 288: return 3;  // Mar
        case 291: return 4;  // Apr
        case 294: return 10; // Oct
        case 295: return 5;  // May
        case 296: return 9;  // Sep
        case 299: return 7;  // Jul
        case 301: return 6;  // Jun
        case 307: return 11; // Nov
        default: return 0;   // Error 
    }
}


/**
 * @brief  Configure external interrupt INT0 on falling edge (PD2).
 * @return none
 */
static void setup_ext_int(void)
{
    // PD2 = INT0
    gpio_mode_input_nopull(&DDRD, 2);
    EICRA &= ~((1 << ISC00));
    EICRA |= (1 << ISC01);
    EIMSK |= (1 << INT0);
}


/**
 * @brief  Select interrupt source based on RTC availability.
 *         Uses external interrupt if RTC found, Timer1 otherwise.
 * @return none
 */
void set_interrupt_source(void)
{
    if (RTC_OK == 0)
    {
        tim1_ovf_disable();
        rtc_write_reg(0x0e, 0x00);
        setup_ext_int();
    }
    else
    {   
        EIMSK &= ~(1 << INT0);
        tim1_ovf_enable();
    }
}


/**
 * @brief  Main program entry point.
 *         Initializes sensors, SD card, RTC, and starts data logging.
 * @return Program exit code (never returns in normal operation).
 */
int main(void)
{
    tim1_ovf_1sec();
    uint8_t hour, minute, second; 
    uint8_t day, date, month, year;
    
    // Initialize UART
    uart_init(UART_BAUD_SELECT(115200, F_CPU));

    //Init I2C
    twi_init();

    // Configure SPI pins for ATmega328P
    gpio_mode_output(&DDRB, PB3);  // MOSI
    gpio_mode_output(&DDRB, PB5);  // SCK
    gpio_mode_output(&DDRD, PD4);  // SS (Chip Select)
    gpio_mode_input_nopull(&DDRB, PB4);  // MISO as input
    
    // Set SS high initially (SD card deselected)
    gpio_write_high(&PORTD, PIND4);

    gpio_mode_output(&DDRC, 0); //led red   ERROR
    gpio_mode_output(&DDRC, 1); //led green STATUS
    gpio_mode_output(&DDRC, 2); //led yellow ACT
    gpio_write_high(&PORTC, 0);
    gpio_write_high(&PORTC, 1);
    gpio_write_high(&PORTC, 2);
    
    // Enable global Interrupts
    sei();
    
    gpio_write_low(&ACTIVITY_LED_PORT, L_ACT);
    gpio_write_low(&ERROR_LED_PORT, L_ERROR);

    #ifdef UART_DEBUG
        uart_puts_P("\r\n=== SD Card FAT32 Test ===\r\n");
    #endif

    // Initialize SPI
    #ifdef SD_write
    spi_init();
    #ifdef UART_DEBUG
    uart_puts_P("SPI initialized\r\n");
    #endif
    
    #ifdef UART_DEBUG
    uart_puts_P("Initializing SD card...\r\n");
    #endif
    
    if(SD_init())
    {
        #ifdef UART_DEBUG
            uart_puts_P("SD Card Initialization Error!\r\n");
            uart_puts_P("System continuing without SD card...\r\n");
        #endif
        SD_OK = 1;
    }
    #ifdef UART_DEBUG
        uart_puts_P("SD card initialized successfully!\r\n");
        uart_puts_P("Reading boot sector...\r\n");
    #endif
    
    if(getBootSectorData())
    {
        #ifdef UART_DEBUG
            uart_puts_P("FAT32 initialization failed!\r\n");
            uart_puts_P("System continuing without SD card...\r\n");
        #endif
        FS_OK = 1;
    }
    else
    {
        #ifdef UART_DEBUG
            uart_puts_P("FAT32 initialized successfully!\r\n");
        #endif
        FS_OK = 0;
    }
    #endif

     /* Initialize BME280 */
    struct bme280_dev bme_dev;
    uint8_t bme_addr = BME280_I2C_ADDR_PRIM;
    if (bme_init_simple(&bme_dev, &bme_addr) != BME280_OK) {
        
        #ifdef UART_DEBUG
            uart_puts_P("BME init failed\r\n");
        #endif
        BM_OK = 1;
    }
    _delay_ms(100);

    /* Initialize SGP41 */
    if (sgp41_init_simple() != 0) 
    {
        #ifdef UART_DEBUG
            uart_puts_P("SGP41 init failed\r\n");
        #endif
        SGP_OK = 1;
    }
    
    if (twi_test_address(RTC_ADDRESS))
    {
        #ifdef UART_DEBUG
            uart_puts_P("RTC not found!\r\n");
            uart_puts_P("System using Compile time\r\n");
        #endif
        RTC_OK = 1;

        // Set compile time to local variables (RTC not available)
        int hour_comp, minute_comp, second_comp;
        sscanf(__TIME__, "%d:%d:%d", &hour_comp, &minute_comp, &second_comp);
        hour = (uint8_t)hour_comp;
        minute = (uint8_t)minute_comp;
        second = (uint8_t)second_comp;

        int year_comp, month_comp, day_comp;
        char month_str[4];
        sscanf(__DATE__, "%s %d %d", month_str, &day_comp, &year_comp);

        uint16_t month_sum = month_str[0] + month_str[1] + month_str[2];
        month_comp = get_month_from_ascii_sum(month_sum);
        
        date = (uint8_t)day_comp;
        month = (uint8_t)month_comp;
        year = (uint8_t)(year_comp % 100);
    }
    else
    {
        #ifdef UART_DEBUG
            uart_puts_P("RTC found!\r\n");
        #endif
        #ifdef UPDATE_RTC_TIME_COMPILE
            int hour_comp, minute_comp, second_comp;
            sscanf(__TIME__, "%d:%d:%d", &hour_comp, &minute_comp, &second_comp);
            hour = (uint8_t)hour_comp;
            minute = (uint8_t)minute_comp;
            second = (uint8_t)second_comp;

            int year_comp, month_comp, day_comp;
            char month_str[4];
            sscanf(__DATE__, "%s %d %d", month_str, &day_comp, &year_comp);

            uint16_t month_sum = month_str[0] + month_str[1] + month_str[2];
            month_comp = get_month_from_ascii_sum(month_sum);
            
            date = (uint8_t)day_comp;
            month = (uint8_t)month_comp;
            year = (uint8_t)(year_comp % 100);
            rtc_set_time(hour, minute, second);
            rtc_set_date(DAY_NUMBER, day, month, (year % 100));
        #endif
        rtc_write_reg(0x0e, 0x00);
    }
    
    if (RTC_OK & SGP_OK & SD_OK & FS_OK & BM_OK)
    {
        #ifdef UART_ON
            uart_puts_P("No sensors and SD card available, system halted!\r\n");
        #endif
        gpio_write_low(&ERROR_LED_PORT, L_ERROR);
        gpio_write_low(&ACTIVITY_LED_PORT, L_ACT);
        while(1);
    }
    else if (RTC_OK | SGP_OK | BM_OK | SD_OK | FS_OK)
    {
        #ifdef UART_ON
            uart_puts_P("System initialized with warnings!\r\n");
        #endif
        gpio_write_low(&ERROR_LED_PORT, L_ERROR);
    }
    else
    {
        #ifdef UART_ON
            uart_puts_P("System initialized successfully!\r\n");
        #endif
        gpio_write_high(&ERROR_LED_PORT, L_ERROR);
        gpio_write_high(&STATUS_LED_PORT, L_ACT);
        gpio_write_low(&STATUS_LED_PORT, L_STATUS);
    }
    
    set_interrupt_source();
    char buffer[50];
    // Main loop
    while (1)
    {   
        #ifdef UART_ON
        if(printRTC == 1)
        {
            printRTC = 0;
            if (!(twi_test_address(RTC_ADDRESS)))
            {
                rtc_get_time(&hour, &minute, &second);
                sprintf(buffer, "Time RTC: %02d:%02d:%02d\r\n", hour, minute, second);
                uart_puts(buffer);
                rtc_get_date(&date, &month, &year);
                sprintf(buffer, "Date RTC: %02d/%02d/20%02d\r\n", date, month, year);
                uart_puts(buffer);
                RTC_OK = 0;
            }
            else
            {
                uart_puts_P("RTC not found, using Compile time\r\n");
                gpio_write_low(&ERROR_LED_PORT, L_ERROR);
            }
        }
        #endif
        
        if(RTC_OK | SGP_OK | BM_OK | SD_OK | FS_OK)
        {
            gpio_write_low(&ERROR_LED_PORT, L_ERROR);
        }
        else
        {
            gpio_write_high(&ERROR_LED_PORT, L_ERROR);
        }

        if (measurement_flag)
        {
            measurement_flag = 0;

            char sdString[100];
            memset(sdString, 0, sizeof(sdString)); 
            
            int32_t t100 = 0;
            uint32_t press_pa = 0;
            uint32_t hum_x1024 = 0;
            
            if(twi_test_address(RTC_ADDRESS))
            {
                RTC_OK = 1;
                set_interrupt_source();
            }
            else
            {
                RTC_OK = 0;
                rtc_get_time(&hour, &minute, &second);
                rtc_get_date(&date, &month, &year);
                set_interrupt_source();
            }
            
            snprintf(sdString, sizeof(sdString), "%02d:%02d:%02d,%02d/%02d/20%02d,",
                     hour, minute, second, date, month, year);
            
            if (bme_read_once(&bme_dev, &t100, &press_pa, &hum_x1024) == 0) {
                int32_t temp_int = t100 / 100;
                int32_t temp_frac = (t100 >= 0) ? (t100 % 100) : ((-t100) % 100);
                
                uint32_t press_hpa_int = press_pa / 100;
                uint32_t press_hpa_frac = press_pa % 100;
                
                uint32_t hum_percent_x100 = (hum_x1024 * 100 + 512) / 1024;
                uint32_t hum_int = hum_percent_x100 / 100;
                uint32_t hum_frac = hum_percent_x100 % 100;
                
                double P = (double)press_pa;
                double alt = 44330.0 * (1.0 - pow(P / 101325.0, 0.19029495718363465));
                int32_t alt_int = (int32_t)alt;
                int32_t alt_frac = (int32_t)(fabs(alt - (double)alt_int) * 100.0 + 0.5);
                
                char temp_buf[64];
                snprintf(temp_buf, sizeof(temp_buf), "%02ld.%02ld,%03lu.%02lu,%02lu.%02lu,%03ld.%02ld,",
                         (long)temp_int, (long)temp_frac,
                         (unsigned long)press_hpa_int, (unsigned long)press_hpa_frac,
                         (unsigned long)hum_int, (unsigned long)hum_frac,
                         (long)alt_int, (long)alt_frac);
                strncat(sdString, temp_buf, sizeof(sdString) - strlen(sdString) - 1);
                BM_OK = 0;
            } else {
                gpio_write_low(&ERROR_LED_PORT, L_ERROR);
                strncat(sdString, "ERR,ERR,ERR,ERR,", sizeof(sdString) - strlen(sdString) - 1);
                BM_OK = 1;
            }
            
            /* Read SGP41 */
            int32_t voc_idx = 0;
            int32_t nox_idx = 0;
            if (sgp41_measure_once(&voc_idx, &nox_idx) == 0) {
                char temp_buf[32];
                snprintf(temp_buf, sizeof(temp_buf), "%ld,%ld\n", (long)voc_idx, (long)nox_idx);
                strncat(sdString, temp_buf, sizeof(sdString) - strlen(sdString) - 1);
                SGP_OK = 0;
            } else {
                strncat(sdString, "ERR,ERR\n", sizeof(sdString) - strlen(sdString) - 1);
                SGP_OK = 1;
            }
            
            uint8_t write_error = 0;

            /* Write to SD card */
            #ifdef SD_write
            gpio_write_low(&ACTIVITY_LED_PORT, L_ACT);
            if (SD_OK == 0 && FS_OK == 0)
            {
                memset(dataString, 0, MAX_STRING_SIZE);
                strncpy((char*)dataString, sdString, MAX_STRING_SIZE - 1);
                
                unsigned char fileName[12];
                strcpy((char*)fileName, "data1.csv");

                write_error = writeFile(fileName);
                 if (write_error) {
                    uart_puts_P("SD write error!\r\n");
                    gpio_write_low(&ERROR_LED_PORT, L_ERROR);
                }
                gpio_write_high(&ACTIVITY_LED_PORT, L_ACT);
            }
            #endif
            
            #ifdef UART_ON
                uart_puts(sdString);
            #endif
        }
    }
    return 0;
}


/**
 * @brief  Timer1 overflow interrupt handler (1-second timer).
 *         Increments counter and triggers measurement when interval elapsed.
 */
ISR(TIMER1_OVF_vect)
{   
    counterTim1++;
    if (counterTim1 >= LOG_TIME_INTERVAL_SEC) 
    {
        counterTim1 = 0;    
        measurement_flag = 1;
    }
}


/**
 * @brief  External interrupt INT0 handler (RTC square-wave output).
 *         Increments counter and triggers measurement when interval elapsed.
 */
ISR(INT0_vect)
{
    counterTim1++;
    if (counterTim1 >= LOG_TIME_INTERVAL_SEC)
    {
        counterTim1 = 0;    
        measurement_flag = 1;
    }
}
