#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <math.h>
#include <util/delay.h>
#include "twi.h"
#include "timer.h"
#include "uart.h"
#include "bme280.h"

volatile uint8_t tick = 0;

/* Manufacturer integer compensation functions (from Bosch Sensortec). Returns:
 *  - Temperature: int32 in 0.01 degC
 *  - Pressure: uint32 in Q24.8 (Pa * 256)
 *  - Humidity: uint32 in Q22.10 (percent * 1024)
 *  - Amplitude: int32 
 */

static int32_t BME280_compensate_T_int32(int32_t adc_T, struct bme280_calib_data *calib)
{
    int32_t var1, var2, T;
    var1 = ((((adc_T >> 3) - ((int32_t)calib->dig_t1 << 1))) * ((int32_t)calib->dig_t2)) >> 11;
    var2 = (((((adc_T >> 4) - ((int32_t)calib->dig_t1)) * ((adc_T >> 4) - ((int32_t)calib->dig_t1))) >> 12) *
            ((int32_t)calib->dig_t3)) >> 14;
    calib->t_fine = var1 + var2;
    T = (calib->t_fine * 5 + 128) >> 8;
    return T;
}

static uint32_t BME280_compensate_P_int64(int32_t adc_P, struct bme280_calib_data *calib)
{
    int64_t var1, var2, p;
    var1 = ((int64_t)calib->t_fine) - 128000;
    var2 = var1 * var1 * (int64_t)calib->dig_p6;
    var2 = var2 + ((var1 * (int64_t)calib->dig_p5) << 17);
    var2 = var2 + (((int64_t)calib->dig_p4) << 35);
    var1 = ((var1 * var1 * (int64_t)calib->dig_p3) >> 8) + ((var1 * (int64_t)calib->dig_p2) << 12);
    var1 = (((((int64_t)1) << 47) + var1) * ((int64_t)calib->dig_p1)) >> 33;
    if (var1 == 0)
    {
        return 0; /* avoid exception caused by division by zero */
    }
    p = 1048576 - adc_P;
    p = (((p << 31) - var2) * 3125) / var1;
    var1 = (((int64_t)calib->dig_p9) * (p >> 13) * (p >> 13)) >> 25;
    var2 = (((int64_t)calib->dig_p8) * p) >> 19;
    p = ((p + var1 + var2) >> 8) + (((int64_t)calib->dig_p7) << 4);
    return (uint32_t)p;
}

static uint32_t bme280_compensate_H_int32(int32_t adc_H, struct bme280_calib_data *calib)
{
    int32_t v_x1_u32r;
    v_x1_u32r = (calib->t_fine - ((int32_t)76800));
    v_x1_u32r = (((((adc_H << 14) - (((int32_t)calib->dig_h4) << 20) - (((int32_t)calib->dig_h5) * v_x1_u32r)) + ((int32_t)16384)) >> 15) *
                 (((((((v_x1_u32r * ((int32_t)calib->dig_h6)) >> 10) * (((v_x1_u32r * ((int32_t)calib->dig_h3)) >> 11) + ((int32_t)32768))) >> 10) + ((int32_t)2097152)) * ((int32_t)calib->dig_h2) + 8192) >> 14));
    v_x1_u32r = (v_x1_u32r - (((((v_x1_u32r >> 15) * (v_x1_u32r >> 15)) >> 7) * ((int32_t)calib->dig_h1)) >> 4));
    v_x1_u32r = (v_x1_u32r < 0 ? 0 : v_x1_u32r);
    v_x1_u32r = (v_x1_u32r > 419430400 ? 419430400 : v_x1_u32r);
    return (uint32_t)(v_x1_u32r >> 12);
}

// Adapter callbacks for Bosch driver (use dev->intf_ptr as pointer to 7-bit I2C address)
static BME280_INTF_RET_TYPE user_i2c_read(uint8_t reg_addr, uint8_t *reg_data, uint32_t len, void *intf_ptr)
{
    uint8_t dev_addr = *((uint8_t *)intf_ptr);
    if (len == 0 || reg_data == NULL)
        return BME280_E_INVALID_LEN;

    twi_readfrom_mem_into(dev_addr, reg_addr, (volatile uint8_t *)reg_data, (uint8_t)len);
    return BME280_INTF_RET_SUCCESS;
}

static BME280_INTF_RET_TYPE user_i2c_write(uint8_t reg_addr, const uint8_t *reg_data, uint32_t len, void *intf_ptr)
{
    uint8_t dev_addr = *((uint8_t *)intf_ptr);
    twi_start();
    if (twi_write((dev_addr << 1) | TWI_WRITE) != 0) { twi_stop(); return BME280_E_COMM_FAIL; }
    if (twi_write(reg_addr) != 0) { twi_stop(); return BME280_E_COMM_FAIL; }
    for (uint32_t i = 0; i < len; i++)
    {
        if (twi_write(reg_data[i]) != 0) { twi_stop(); return BME280_E_COMM_FAIL; }
    }
    twi_stop();
    return BME280_INTF_RET_SUCCESS;
}

static void user_delay_us(uint32_t period, void *intf_ptr)
{
    (void)intf_ptr;
    while (period >= 1000)
    {
        _delay_ms(1);
        period -= 1000;
    }
    while (period--)
        _delay_us(1);
}

ISR(TIMER1_OVF_vect)
{
    tick = 1;
}

int main(void)
{
    twi_init();
    uart_init(UART_BAUD_SELECT(9600, F_CPU));

    tim1_ovf_262ms();     // ~262 ms timer
    tim1_ovf_enable();    // enable interrupt
    sei();

    uart_puts("BME280 (driver) INIT...\r\n");

    struct bme280_dev dev;
    uint8_t dev_addr = BME280_I2C_ADDR_PRIM; // default 0x76

    dev.intf = BME280_I2C_INTF;
    dev.intf_ptr = &dev_addr;
    dev.read = user_i2c_read;
    dev.write = user_i2c_write;
    dev.delay_us = user_delay_us;

    int8_t rslt = bme280_init(&dev);
    if (rslt != BME280_OK)
    {
        uart_puts("BME280 init failed\r\n");
        while (1);
    }
    uart_puts("BME280 detected OK\r\n");

    /* Configure sensor: oversampling x1 for T/P/H */
    struct bme280_settings settings;
    settings.osr_h = BME280_OVERSAMPLING_1X;
    settings.osr_p = BME280_OVERSAMPLING_1X;
    settings.osr_t = BME280_OVERSAMPLING_1X;
    settings.filter = BME280_FILTER_COEFF_OFF;
    settings.standby_time = BME280_STANDBY_TIME_125_MS;

    rslt = bme280_set_sensor_settings(BME280_SEL_ALL_SETTINGS, &settings, &dev);
    if (rslt != BME280_OK)
    {
        uart_puts("BME280 set settings failed\r\n");
    }

    /* Main loop: trigger forced measurement on each tick and read compensated data */
    while (1)
    {
        if (tick)
        {
            tick = 0;

            /* Trigger one-shot measurement (forced) */
            bme280_set_sensor_mode(BME280_POWERMODE_FORCED, &dev);

            /* Wait required measurement time */
            uint32_t meas_delay_us = 0;
            bme280_cal_meas_delay(&meas_delay_us, &settings);
            user_delay_us(meas_delay_us, dev.intf_ptr);

            /* Read raw measurement registers (8 bytes: press(3), temp(3), hum(2)) */
            uint8_t data[8];
            rslt = bme280_get_regs(BME280_REG_DATA, data, BME280_LEN_P_T_H_DATA, &dev);
            if (rslt == BME280_OK)
            {
                uint32_t adc_P = ((uint32_t)data[0] << 12) | ((uint32_t)data[1] << 4) | ((uint32_t)data[2] >> 4);
                uint32_t adc_T = ((uint32_t)data[3] << 12) | ((uint32_t)data[4] << 4) | ((uint32_t)data[5] >> 4);
                uint32_t adc_H = ((uint32_t)data[6] << 8) | (uint32_t)data[7];

                /* Compensate using manufacturer's integer routines */
                int32_t t100 = BME280_compensate_T_int32((int32_t)adc_T, &dev.calib_data); /* 0.01 degC */
                uint32_t p_q24_8 = BME280_compensate_P_int64((int32_t)adc_P, &dev.calib_data); /* Q24.8 */
                uint32_t hum_x1024 = bme280_compensate_H_int32((int32_t)adc_H, &dev.calib_data); /* Q22.10 */

                /* Convert pressure Q24.8 -> Pa (divide by 256) */
                uint32_t press_pa = (uint32_t)((uint64_t)p_q24_8 / 256ULL);

                char msg[96];
                /* Temperature: t100 is in 0.01 degC */
                int32_t temp_int = t100 / 100; /* integer degrees C */
                int32_t temp_frac = (t100 >= 0) ? (t100 % 100) : ((-t100) % 100); /* centi-degree remainder */

                /* Pressure: press_pa is Pa -> convert to hPa (Pa/100) with 2 decimals */
                uint32_t press_hpa_int = press_pa / 100; /* hPa integer part */
                uint32_t press_hpa_frac = press_pa % 100; /* fractional 1/100 hPa */

                /* Humidity: hum_x1024 is percent * 1024. Convert to percent*100 with rounding */
                uint32_t hum_percent_x100 = (hum_x1024 * 100 + 512) / 1024; /* add 512 for rounding */
                uint32_t hum_int = hum_percent_x100 / 100;
                uint32_t hum_frac = hum_percent_x100 % 100;

                /* Altitude: use standard formula
                 * altitude (m) = 44330 * (1 - (P / P0)^(1/5.255))
                 * where P0 = 101325 Pa (sea level)
                 */
                double P = (double)press_pa;
                const double P0 = 101325.0;
                double alt = 44330.0 * (1.0 - pow(P / P0, 0.19029495718363465));
                int32_t alt_int = (int32_t)alt;
                int32_t alt_frac = (int32_t)(fabs(alt - (double)alt_int) * 100.0 + 0.5);

                sprintf(msg, "T=%ld.%02ld C  P=%lu.%02luhPa  H=%lu.%02lu %%  Alt=%ld.%02ld m\r\n",
                    (long)temp_int, (long)temp_frac,
                    (unsigned long)press_hpa_int, (unsigned long)press_hpa_frac,
                    (unsigned long)hum_int, (unsigned long)hum_frac,
                    (long)alt_int, (long)alt_frac);
                uart_puts(msg);
            }
            else
            {
                uart_puts("BME280 read failed\r\n");
            }
        }
    }

    return 0;
}