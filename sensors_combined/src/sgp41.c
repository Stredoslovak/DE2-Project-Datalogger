/*
 * I2C-Generator: 0.3.0
 * Yaml Version: 0.1.0
 * Template Version: 0.7.0-62-g3d691f9
 */
/*
 * Copyright (c) 2021, Sensirion AG
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * * Neither the name of Sensirion AG nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

// Use project's TWI and UART libraries (no Arduino)
#include <stdint.h>
#include <stdio.h>
#include <util/delay.h>
#include "twi.h"
#include "uart.h"
#include "SensirionI2CSgp41.h"
#include <avr/interrupt.h>
#include "timer.h"
#include "sensirion_gas_index_algorithm.h"

/* Refactored SGP41 wrapper: provide init + single-measure interface so main.c
 * can control timing and UART output. The module keeps internal state for the
 * gas index algorithms between measurements.
 */

// Time in seconds needed for NOx conditioning (do not exceed 10s)
static uint16_t conditioning_s = 10;

static GasIndexAlgorithmParams voc_params;
static GasIndexAlgorithmParams nox_params;
static int initialized = 0;

/* Initialize SGP41 algorithm state and perform optional self-test.
 * Caller must call `twi_init()` before this function.
 * Returns 0 on success, non-zero on error.
 */
int sgp41_init_simple(void)
{
    if (initialized) return 0;

    /* Get serial number (optional, non-fatal) */
    uint16_t serialNumber[3];
    (void)sgp41_getSerialNumber(serialNumber);

    /* Run self-test once (non-fatal) */
    uint16_t testResult = 0;
    (void)sgp41_executeSelfTest(&testResult);

    /* Initialize gas index algorithms for VOC and NOx */
    GasIndexAlgorithm_init(&voc_params, GasIndexAlgorithm_ALGORITHM_TYPE_VOC);
    GasIndexAlgorithm_init(&nox_params, GasIndexAlgorithm_ALGORITHM_TYPE_NOX);

    initialized = 1;
    return 0;
}

/* Perform one measurement and compute VOC/NOx indices. Caller provides pointers
 * to store results. Returns 0 on success or non-zero on error.
 */
int sgp41_measure_once(int32_t *voc_index, int32_t *nox_index)
{
    if (!initialized) return -1;

    uint16_t defaultRh = 0x8000;
    uint16_t defaultT = 0x6666;
    uint16_t srawVoc = 0;
    uint16_t srawNox = 0;

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

    if (err) return (int)err;

    /* Process raw sraw values through gas index algorithms */
    GasIndexAlgorithm_process(&voc_params, (int32_t)srawVoc, voc_index);
    GasIndexAlgorithm_process(&nox_params, (int32_t)srawNox, nox_index);

    return 0;
}