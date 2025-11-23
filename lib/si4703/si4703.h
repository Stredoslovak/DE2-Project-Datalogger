#ifndef SI4703_H
    #define SI4703_H
#endif  

#include <avr/io.h>

static const uint16_t SI4703_ADDR = 0x10; // I2C address;

//Define the register names

typedef struct {
    uint16_t DEVICEID;    // 0x00
    uint16_t CHIPID;      // 0x01
    uint16_t POWERCFG;    // 0x02
    uint16_t CHANNEL;     // 0x03
    uint16_t SYSCONFIG1;  // 0x04
    uint16_t SYSCONFIG2;  // 0x05
    uint16_t STATUSRSSI;  // 0x0A
    uint16_t READCHAN;    // 0x0B
    uint16_t RDSA;        // 0x0C
    uint16_t RDSB;        // 0x0D
    uint16_t RDSC;        // 0x0E
    uint16_t RDSD;        // 0x0F
} registers_t;


// prebrate od sparkfunu
	//Register 0x02 - POWERCFG
#define SMUTE   15
#define DMUTE   14
#define SKMODE  10
#define SEEKUP  9
#define SEEK    8
#define TUNE    15  /* register 0x03 uses bit 15 for TUNE in some docs */
#define RDS     12
#define DE      11
#define SPACE1  5
#define SPACE0  4
#define VOLUME0 0
#define RDSR 15
#define STC 14
#define SFBL = 13;
#define AFCRL = 12;
#define RDSS = 11;
#define STEREO = 8;	
#define RDSM 11


void si4703_init_i2c(volatile uint8_t *RST_dataReg,volatile uint8_t *RST_portReg, uint8_t RST_pin,
                     volatile uint8_t *SEN_dataReg,volatile uint8_t *SEN_portReg, uint8_t SEN_pin,
                     volatile uint8_t *SDA_dataReg,volatile uint8_t *SDA_portReg, uint8_t SDA_pin,
                     uint8_t delay);

void si4703_readRegs(uint8_t addr, uint16_t regs[16]);
void si4703_writeRegs(uint8_t addr, uint16_t regs[16]);

void si4703_setRegs(uint16_t regs[16]);
void si4703_tuneTo(uint16_t channel, uint16_t regs[16]);
void si4703_setVol(uint8_t volume, uint16_t regs[16]);
uint16_t si4703_getFreq(uint16_t regs[16]);
uint8_t si4703_getVolume(uint16_t regs[16]);
uint8_t si4703_askForRDS(uint16_t regs[16]);
void si4703_clearRDSRequest(uint16_t regs[16]);