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


#include <avr/io.h>
#include "SPI_routines.h"


/**
 * @brief  Initialize SPI for SD card communication.
 *         Sets Master mode, MSB first, SCK phase low, SCK idle low.
 *         Initial clock rate is set low (125Khz approx) for SD initialization.
 * @return none
 */
void spi_init(void)
{
    
    // Enable SPI, Set as Master
    //- Prescaler: Fosc/16, Enable Interrupts
    //The MOSI, SCK pins
    //SPCR=(1<<SPE)|(1<<MSTR)|(1<<SPR0)|(1<<SPIE);
    //SPR01 
SPCR = 0x52; //setup SPI: Master mode, MSB first, SCK phase low, SCK idle low
SPSR = 0x00;
}


/**
 * @brief  Transmit a byte via SPI.
 * @param  data Byte to transmit.
 * @return Received byte (simultaneous duplex exchange).
 */
unsigned char SPI_transmit(unsigned char data)
{
// Start transmission
// uart_puts("Ts\r\n");
SPDR = data;


// Wait for transmission complete
while(!(SPSR & (1<<SPIF)));
data = SPDR;


// uart_puts("T\r\n");
return(data);
}


/**
 * @brief  Receive a byte via SPI.
 *         Sends 0xFF dummy byte to generate clock pulses.
 * @return Received byte.
 */
unsigned char SPI_receive(void)
{
unsigned char data;
// Wait for reception complete
// uart_puts("Tr\r\n");
SPDR = 0xff;
while(!(SPSR & (1<<SPIF)));
data = SPDR;


// Return data register
// uart_puts("Trr\r\n");
return data;
}


//******** END ****** [www.dharmanitech.com](https://www.dharmanitech.com) *****
