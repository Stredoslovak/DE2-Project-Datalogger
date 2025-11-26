// Program to interface with SGP41 sensor
#include <stdint.h>
#include <stdio.h>
#include <util/delay.h>
#include "twi.h"
#include "uart.h"
#include "SensirionI2CSgp41.h"
#include <avr/interrupt.h>
#include "timer.h"
#include "sensirion_gas_index_algorithm.h"

// Time in seconds needed for NOx conditioning (do not exceed 10s)
static uint16_t conditioning_s = 10;

volatile uint8_t measurement_flag = 0;

ISR(TIMER1_OVF_vect) {
    measurement_flag = 1;
}

int main(void) {
    // Initialize UART and TWI
    // Use a conservative baudrate for reliable terminal output
    uart_init(UART_BAUD_SELECT(9600, F_CPU));
    twi_init();

    // Configure Timer1 to overflow every 1s and enable interrupt
    tim1_stop();
    tim1_ovf_1sec();
    tim1_ovf_enable();
    sei();

    // Print header
    uart_puts("VOC_idx: <index> NOX_idx: <index>\r\n");

    // Get serial number (optional)
    uint16_t serialNumber[3];
    if (sgp41_getSerialNumber(serialNumber) == 0) {
        char buf[64];
        snprintf(buf, sizeof(buf), "SN: %04X %04X %04X\r\n",
                 serialNumber[0], serialNumber[1], serialNumber[2]);
        uart_puts(buf);
    }

    // Run self-test once
    uint16_t testResult;
    if (sgp41_executeSelfTest(&testResult) == 0) {
        char buf[64];
        snprintf(buf, sizeof(buf), "SelfTest: 0x%04X\r\n", testResult);
        uart_puts(buf);
    } else {
        uart_puts("SelfTest: error\r\n");
    }

    uint16_t defaultRh = 0x8000;
    uint16_t defaultT = 0x6666;
    uint16_t srawVoc = 0;
    uint16_t srawNox = 0;

    /* Initialize gas index algorithms for VOC and NOx */
    GasIndexAlgorithmParams voc_params;
    GasIndexAlgorithmParams nox_params;
    GasIndexAlgorithm_init(&voc_params, GasIndexAlgorithm_ALGORITHM_TYPE_VOC);
    GasIndexAlgorithm_init(&nox_params, GasIndexAlgorithm_ALGORITHM_TYPE_NOX);
    int32_t voc_index = 0;
    int32_t nox_index = 0;

    unsigned long seconds = 0;

    // Main loop: wait for timer interrupt to set measurement_flag
    while (1) {
        if (!measurement_flag) continue;
        measurement_flag = 0;
        seconds++;

        uint16_t err;
        if (conditioning_s > 0) {
            /* run conditioning command to keep sensor heater conditioned */
            (void)sgp41_executeConditioning(defaultRh, defaultT, &srawVoc);
            /* always perform a measurement to obtain both VOC and NOx */
            err = sgp41_measureRawSignals(defaultRh, defaultT, &srawVoc, &srawNox);
            conditioning_s--;
        } else {
            err = sgp41_measureRawSignals(defaultRh, defaultT, &srawVoc, &srawNox);
        }

        /* Process raw sraw values through gas index algorithms */
        GasIndexAlgorithm_process(&voc_params, (int32_t)srawVoc, &voc_index);
        GasIndexAlgorithm_process(&nox_params, (int32_t)srawNox, &nox_index);

        char outbuf[96];
        if (err) {
            snprintf(outbuf, sizeof(outbuf), "ERR: %u\r\n", (unsigned)err);
        } else {
            /* Print only VOC and NOX gas indices */
            snprintf(outbuf, sizeof(outbuf), "VOC_idx=%ld NOX_idx=%ld\r\n",
                     (long)voc_index, (long)nox_index);
        }
        uart_puts(outbuf);
        // Ensure we never condition longer than 10 seconds as per datasheet note
        // Do NOT turn the heater off automatically here — turning the heater
        // off disables VOC measurements on SGP41. Keep the heater enabled
        // for continuous VOC+NOx readings. If you want to turn it off,
        // call `sgp41_turnHeaterOff()` manually.
    }

    return 0;
}