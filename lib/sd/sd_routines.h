//**************************************************************
// ****** FUNCTIONS FOR SD RAW DATA TRANSFER *******
//**************************************************************
//Controller        : ATmega32 (Clock: 8 Mhz-internal)
//Compiler          : AVR-GCC (winAVR with AVRStudio-4)
//Project Version   : DL_1.0
//Author            : CC Dharmani, Chennai (India)
//                    [www.dharmanitech.com](https://www.dharmanitech.com)
//Date              : 10 May 2011
//**************************************************************


//Link to the Post: [http://www.dharmanitech.com/2009/01/sd-card-interfacing-with-atmega8-fat32.html](http://www.dharmanitech.com/2009/01/sd-card-interfacing-with-atmega8-fat32.html)


#ifndef _SD_ROUTINES_H_
#define _SD_ROUTINES_H_


/** @brief Use this macro to disable multiple block access functions (not required for FAT32). */
#define FAT_TESTING_ONLY


/**
 * @defgroup SDChipSelect SD Card Chip Select Macros
 * @{
 */
//use following macros if PB1 pin is used for Chip Select of SD
//#define SD_CS_ASSERT     PORTB &= ~0x02
//#define SD_CS_DEASSERT   PORTB |= 0x02


//use following macros if SS (PB4) pin is used for Chip Select of SD
// #define SD_CS_ASSERT     PORTB &= ~0x10
// #define SD_CS_DEASSERT   PORTB |= 0x10

//use following macros if SS (PD4) pin is used for Chip Select of SD (Arduino Ethernet Shield)
#define SD_CS_ASSERT    PORTD &= ~0x10
#define SD_CS_DEASSERT  PORTD |= 0x10
/** @} */


/**
 * @defgroup SDCommands SD Card Command Definitions
 * @{
 */
#define GO_IDLE_STATE            0
#define SEND_OP_COND             1
#define SEND_IF_COND             8
#define SEND_CSD                 9
#define STOP_TRANSMISSION        12
#define SEND_STATUS              13
#define SET_BLOCK_LEN            16
#define READ_SINGLE_BLOCK        17
#define READ_MULTIPLE_BLOCKS     18
#define WRITE_SINGLE_BLOCK       24
#define WRITE_MULTIPLE_BLOCKS    25
#define ERASE_BLOCK_START_ADDR   32
#define ERASE_BLOCK_END_ADDR     33
#define ERASE_SELECTED_BLOCKS    38
#define SD_SEND_OP_COND          41   //ACMD
#define APP_CMD                  55
#define READ_OCR                 58
#define CRC_ON_OFF               59
/** @} */


#define ON     1
#define OFF    0


/** @brief Global variables for SD card operations. */
volatile unsigned long startBlock, totalBlocks; 
volatile unsigned char SDHC_flag, cardType, buffer[512];


/**
 * @brief  Initialize SD/SDHC card in SPI mode.
 * @return 0 on success, error code otherwise.
 */
unsigned char SD_init(void);

/**
 * @brief  Send command to SD card with argument.
 * @param  cmd Command byte value.
 * @param  arg 32-bit command argument.
 * @return Response byte from card.
 */
unsigned char SD_sendCommand(unsigned char cmd, unsigned long arg);

/**
 * @brief  Read single 512-byte block from SD card into buffer.
 * @param  startBlock Block address to read.
 * @return 0 on success, error code otherwise.
 */
unsigned char SD_readSingleBlock(unsigned long startBlock);

/**
 * @brief  Write single 512-byte block to SD card from buffer.
 * @param  startBlock Block address to write.
 * @return 0 on success, error code otherwise.
 */
unsigned char SD_writeSingleBlock(unsigned long startBlock);

/**
 * @brief  Read multiple blocks from SD card and send to UART.
 * @param  startBlock  Starting block address.
 * @param  totalBlocks Number of blocks to read.
 * @return 0 on success, error code otherwise.
 */
unsigned char SD_readMultipleBlock (unsigned long startBlock, unsigned long totalBlocks);

/**
 * @brief  Receive data from UART and write to multiple SD card blocks.
 * @param  startBlock  Starting block address.
 * @param  totalBlocks Number of blocks to write.
 * @return 0 on success, error code otherwise.
 */
unsigned char SD_writeMultipleBlock(unsigned long startBlock, unsigned long totalBlocks);

/**
 * @brief  Erase specified number of blocks from SD card.
 * @param  startBlock  First block address to erase.
 * @param  totalBlocks Number of blocks to erase.
 * @return 0 on success, error code otherwise.
 */
unsigned char SD_erase (unsigned long startBlock, unsigned long totalBlocks);


#endif
