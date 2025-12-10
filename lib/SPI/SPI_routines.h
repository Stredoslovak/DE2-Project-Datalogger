//**************************************************************
// ****** FUNCTIONS FOR SPI COMMUNICATION *******
//**************************************************************
//Controller        : ATmega32 (Clock: 8 Mhz-internal)
//Compiler          : AVR-GCC (winAVR with AVRStudio-4)
//Project Version   : DL_1.0
//Author            : CC Dharmani, Chennai (India)
//                    [www.dharmanitech.com](https://www.dharmanitech.com)
//Date              : 10 May 2011
//**************************************************************


#ifndef _SPI_ROUTINES_H_
#define _SPI_ROUTINES_H_


/** @brief Configure SPI for SD card initialization (low speed, ~125 kHz). */
#define SPI_SD             SPCR = 0x52

/** @brief Configure SPI for high speed data transfer (max speed, F_CPU/2). */
#define SPI_HIGH_SPEED     SPCR = 0x50; SPSR |= (1<<SPI2X)



/**
 * @brief  Initialize SPI module (Master mode).
 * @return none
 */
void spi_init(void);

/**
 * @brief  Transmit a byte via SPI.
 * @param  data Byte to send.
 * @return Received byte.
 */
unsigned char SPI_transmit(unsigned char data);

/**
 * @brief  Receive a byte via SPI.
 * @return Received byte.
 */
unsigned char SPI_receive(void);


#endif
