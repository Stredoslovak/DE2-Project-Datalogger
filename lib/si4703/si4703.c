#include "si4703.h"
#include <avr/io.h>
#include "gpio.h"
#include <util/delay.h>
#include <twi.h>

const registers_t si4703_reg_map = {
    .DEVICEID   = 0x00,
    .CHIPID     = 0x01,
    .POWERCFG   = 0x02,
    .CHANNEL    = 0x03,
    .SYSCONFIG1 = 0x04,
    .SYSCONFIG2 = 0x05,
    .STATUSRSSI = 0x0A,
    .READCHAN   = 0x0B,
    .RDSA       = 0x0C,
    .RDSB       = 0x0D,
    .RDSC       = 0x0E,
    .RDSD       = 0x0F
};


void si4703_init_i2c(volatile uint8_t *RST_dataReg,volatile uint8_t *RST_portReg, uint8_t RST_pin,
                     volatile uint8_t *SEN_dataReg,volatile uint8_t *SEN_portReg, uint8_t SEN_pin,
                     volatile uint8_t *SDA_dataReg,volatile uint8_t *SDA_portReg, uint8_t SDA_pin,
                     uint8_t delay)
{
    gpio_mode_output(RST_dataReg, RST_pin); // RST pin
    gpio_mode_output(SDA_dataReg, SDA_pin); // SDIO / SDA pin
    gpio_mode_output(SEN_dataReg, SEN_pin); // SEN pin

    gpio_write_low(SDA_portReg, SDA_pin); // SDA low
    gpio_write_low(RST_portReg, RST_pin); // RST low
    gpio_write_high(SEN_portReg, SEN_pin); // SEN high
    _delay_ms(delay);
    gpio_write_high(RST_portReg, RST_pin); // RST high
    _delay_ms(delay);

    // gpio_mode_input_nopull(SEN_dataReg, SEN_pin); // SDIO / SDA pin to input
}

void si4703_writeRegs(uint8_t addr, uint16_t regs[16])
{
    uint8_t upperByte, lowerByte;
    twi_start();
    twi_write((addr<<1) | TWI_WRITE);
    for (uint16_t registerPos = 0x02; registerPos < 0x08; registerPos++)
    {
        upperByte = (uint8_t)(regs[registerPos] >> 8);
        lowerByte = (uint8_t)(regs[registerPos] & 0x00FF);
        // twi_writeto_mem_16b(addr, (uint8_t)registerPos, (uint8_t)(regs[registerPos] >> 8), (uint8_t)(regs[registerPos] & 0x00FF));
        twi_write(upperByte);   // Upper byte
        twi_write(lowerByte); // Lower byte
    }
    twi_stop();
}
// void si4703_readRegs(uint8_t addr, uint16_t regs[16])
// {
//     for (int i = 0x0A; ; i++) //pociatok
//     {
//         if(i == 0x10) i = 0; //loop do 0x00
//         twi_readfrom_mem_into(addr, i, regs[i],1);
//         regs[i] <<= 8;
//         twi_readfrom_mem_into(addr, i, regs[i],1);
//         if(i == 0x09) break; //koniec
//     }
// }
// void si4703_readRegs(uint8_t addr, uint16_t regs[16])
// {
//     /* Prečítaj všetky 16 registrov SI4703. SI4703 často vracia registrovú mapu
//        v poradí 0x0A..0x0F,0x00..0x09 – preto cyklus rovnaký ako v pôvodnom kóde. */
//     uint16_t reg = 0x0A;
//     while (1) {
//         if (reg == 0x10) reg = 0; /* preskoč za 0x0F späť na 0x00 */

//         uint8_t hi = 0, lo = 0;
//         /* Čítaj vysoký a nízky bajt do medzipamäte */
//         twi_readfrom_mem_into(addr, (uint8_t)reg, &hi, 1);
//         twi_readfrom_mem_into(addr, (uint8_t)reg, &lo, 1);
//         regs[reg] = ((uint16_t)hi << 8) | (uint16_t)lo;

//         if (reg == 0x09) break; /* koniec po 0x09 */
//         reg++;
//     }

//     /* Príklad použitia mapy registrov: */
//     /* uint16_t powercfg_addr = si4703_reg_map.POWERCFG;
//        twi_readfrom_mem_into(addr, (uint8_t)powercfg_addr, &hi, 1); */
// }
void si4703_readRegs(uint8_t addr, uint16_t regs[16])
{
    /* Prečítaj všetky 16 registrov SI4703. SI4703 často vracia registrovú mapu
       v poradí 0x0A..0x0F,0x00..0x09 – preto cyklus rovnaký ako v pôvodnom kóde. */
    uint16_t reg = 0x0A;
    twi_start();
    twi_write((addr<<1) | TWI_READ); // inicializacia Citania
    for (int r = 0; r < 16; ++r) { // prelezie vsetkych 16 2bytoych registrov
        if (reg == 0x10) reg = 0; /* wrap after 0x0F -> 0x00 */

        /* For each register read two bytes: hi then lo.
           Send ACK after every byte except the final byte of the 32-byte transfer. */
        uint8_t highByte = twi_read(TWI_ACK); /* bytes 1..31 will be ACKed where appropriate */
        uint8_t lowByte;
        if (r == 15) {
            /* last byte of the entire transfer (32nd byte) -> NACK */
            lowByte = twi_read(TWI_NACK);
        } else {
            lowByte = twi_read(TWI_ACK);
        }

        regs[reg] = ((uint16_t)highByte << 8) | (uint16_t)lowByte; //prepis nacitanych registrov do formy

        if (reg == 0x09) break; /* done after 0x09 (redundant because loop runs 16 times) */
        ++reg;
    }
    twi_stop();

    /* Príklad použitia mapy registrov: */
    /* uint16_t powercfg_addr = si4703_reg_map.POWERCFG;
       twi_readfrom_mem_into(addr, (uint8_t)powercfg_addr, &hi, 1); */
}
void si4703_setRegs(uint16_t regs[16])
{   
    si4703_readRegs(0x10, regs); // Read current register values
    regs[0x07] = 0x8100;
    regs[0x04] = 0x2000;
    si4703_writeRegs(0x10, regs); // Update
    _delay_ms(650);
    si4703_readRegs(0x10, regs); // Read current register values
    regs[si4703_reg_map.POWERCFG] = 0x4001; // Enable the IC, disable mute
    // regs[si4703_reg_map.SYSCONFIG1] |= (1<< RDS); // Enable RDS
    regs[si4703_reg_map.SYSCONFIG1] |= (1<< DE); // 50kHz Europe setup
    regs[si4703_reg_map.SYSCONFIG2] |= (1<< SPACE0); // 100kHz channel spacing for Europe
    regs[si4703_reg_map.SYSCONFIG2] &= 0xFFF0; // clear volume bits
    regs[si4703_reg_map.SYSCONFIG2] |= 0x0001; // Set volume to lowest
    si4703_writeRegs(0x10, regs); // Update
    _delay_ms(110);

}

void si4703_tuneTo(uint16_t channel, uint16_t regs[16])
{
    uint16_t channelfreq = 10 * channel;
    channelfreq -= 8750; // Convert MHz to channel number
    channelfreq /= 10;

    regs[si4703_reg_map.CHANNEL] &= 0xFE00; // Clear out the channel bits
    regs[si4703_reg_map.CHANNEL] |= (channelfreq & 0x01FF); // Mask in the new channel
    regs[si4703_reg_map.CHANNEL] |= (1 << TUNE); // Set the TUNE bit to start tuning
    si4703_writeRegs(0x10, regs);
    _delay_ms(100); // Wait 60ms - you can use or skip this delay
    // while (gpio_read(&PORTB, 0) == 1) // Wait for interrupt indicating STC (Seek/Tune Complete)
    // {
        
    // }
    
    si4703_readRegs(0x10, regs);
    regs[si4703_reg_map.CHANNEL] &= ~(1<<TUNE);
    si4703_writeRegs(0x10, regs);
    while(1) {
    si4703_readRegs(0x10, regs);
    if( (regs[si4703_reg_map.STATUSRSSI] & (1<<STC)) == 0) break; //Tuning complete!
  }
}

void si4703_setVol(uint8_t volume, uint16_t regs[16])
{
    if(volume > 15) volume = 15;
    regs[si4703_reg_map.SYSCONFIG2] &= 0xFFF0; // Clear volume bits
    regs[si4703_reg_map.SYSCONFIG2] |= volume; // Set new volume
    si4703_writeRegs(0x10, regs); // Update
}

uint16_t si4703_getFreq(uint16_t regs[16]){
    si4703_readRegs(0x10, regs);
    uint16_t readchan = regs[si4703_reg_map.READCHAN] & 0x03FF;
    uint16_t freq = 875 + readchan;
    return freq;
}
uint8_t si4703_getVolume(uint16_t regs[16]){
    si4703_readRegs(0x10, regs);
    uint8_t volume = regs[si4703_reg_map.SYSCONFIG2] & 0x000F;
    return volume;
}

uint8_t si4703_askForRDS(uint16_t regs[16])
{   
    si4703_readRegs(0x10, regs); // Read current register values
    regs[si4703_reg_map.POWERCFG] = 0x4401; // verbose mod pre RDS
    // regs[si4703_reg_map.SYSCONFIG1] |= (1<< 15); // povolit interrupt na GPIO2
    // regs[si4703_reg_map.SYSCONFIG1] |= (1<< RDS); // Enable RDS
    // regs[si4703_reg_map.SYSCONFIG1] &= ~((1<<3) | (1<<2)); // interrupt GPIO2 na RDS ready spolu s tune / seek ready
    regs[si4703_reg_map.SYSCONFIG1] = 0xd004; 
    si4703_writeRegs(0x10, regs); // Update
    return 1;
}
void si4703_clearRDSRequest(uint16_t regs[16])
{
    
    regs[si4703_reg_map.POWERCFG] &= ~(1<< 11); // disable verbose mod pre RDS
    regs[si4703_reg_map.SYSCONFIG1] &= ~(1<< 15); // zakazat interrupt na GPIO2
    regs[si4703_reg_map.SYSCONFIG1] &= ~(1<< RDS); // Disable RDS
    si4703_writeRegs(0x10, regs); // Update
}