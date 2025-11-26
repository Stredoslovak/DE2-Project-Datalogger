#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <stdlib.h>
#include <util/delay.h>
#include "uart.h"
#include "twi.h"
#include "timer.h"
#include "bme280.h"
#include "SensirionI2CSgp41.h"
#include <math.h>
#include <string.h>

/* Prototypes of helper functions implemented in src/bme.c and src/sgp41.c */
int bme_init_simple(struct bme280_dev *dev, uint8_t *dev_addr_ptr);
int bme_read_once(struct bme280_dev *dev, int32_t *t100, uint32_t *press_pa, uint32_t *hum_x1024);
int sgp41_init_simple(void);
int sgp41_measure_once(int32_t *voc_index, int32_t *nox_index);

volatile uint8_t measurement_flag = 0;

ISR(TIMER1_OVF_vect) {
    measurement_flag = 1;
}

int main(void)
{
    /* Initialize UART and TWI */
    uart_init(UART_BAUD_SELECT(9600, F_CPU));
    twi_init();

    /* Configure Timer1 to overflow every 1s and enable interrupt */
    tim1_stop();
    tim1_ovf_1sec();
    tim1_ovf_enable();
    sei();

    uart_puts("Combined sensors: T, P, H, Alt, VOC_idx, NOX_idx\r\n");

    /* Initialize BME280 */
    struct bme280_dev bme_dev;
    uint8_t bme_addr = BME280_I2C_ADDR_PRIM;
    if (bme_init_simple(&bme_dev, &bme_addr) != BME280_OK) {
        uart_puts("BME init failed\r\n");
        while (1);
    }

    /* Initialize SGP41 */
    if (sgp41_init_simple() != 0) {
        uart_puts("SGP41 init warning\r\n");
        /* non-fatal; continue */
    }

    /* Main measurement loop: every 1 second, measure both sensors and print a single line */
    while (1) {
        if (!measurement_flag) continue;
        measurement_flag = 0;

        /* Read BME280 */
        int32_t t100 = 0;
        uint32_t press_pa = 0;
        uint32_t hum_x1024 = 0;
        char outbuf[128];

        if (bme_read_once(&bme_dev, &t100, &press_pa, &hum_x1024) == 0) {
            int32_t temp_int = t100 / 100;
            int32_t temp_frac = (t100 >= 0) ? (t100 % 100) : ((-t100) % 100);

            uint32_t press_hpa_int = press_pa / 100;
            uint32_t press_hpa_frac = press_pa % 100;

            uint32_t hum_percent_x100 = (hum_x1024 * 100 + 512) / 1024;
            uint32_t hum_int = hum_percent_x100 / 100;
            uint32_t hum_frac = hum_percent_x100 % 100;

            double P = (double)press_pa;
            const double P0 = 101325.0;
            double alt = 44330.0 * (1.0 - pow(P / P0, 0.19029495718363465));
            int32_t alt_int = (int32_t)alt;
            int32_t alt_frac = (int32_t)(fabs(alt - (double)alt_int) * 100.0 + 0.5);

            
            snprintf(outbuf, sizeof(outbuf), "T=%ld.%02ld C, P=%lu.%02luhPa, H=%lu.%02lu%%, Alt=%ld.%02ldm,",
                     (long)temp_int, (long)temp_frac,
                     (unsigned long)press_hpa_int, (unsigned long)press_hpa_frac,
                     (unsigned long)hum_int, (unsigned long)hum_frac,
                     (long)alt_int, (long)alt_frac);
        } else {
            snprintf(outbuf, sizeof(outbuf), "BME_ERR,",
                     0);
        }

        /* Read SGP41 */
        int32_t voc_idx = 0;
        int32_t nox_idx = 0;
        if (sgp41_measure_once(&voc_idx, &nox_idx) == 0) {
            char tail[64];
            snprintf(tail, sizeof(tail), " VOC_idx=%ld, NOX_idx=%ld\r\n", (long)voc_idx, (long)nox_idx);
            strncat(outbuf, tail, sizeof(outbuf) - strlen(outbuf) - 1);
        } else {
            strncat(outbuf, "SGP_ERR\r\n", sizeof(outbuf) - strlen(outbuf) - 1);
        }

        uart_puts(outbuf);
    }

    return 0;
}
