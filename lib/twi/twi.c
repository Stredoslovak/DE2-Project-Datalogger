/*
 * I2C/TWI library for AVR-GCC.
 * (c) 2018-2025 Tomas Fryza, MIT license
 *
 * Developed using PlatformIO and Atmel AVR platform.
 * Tested on Arduino Uno board and ATmega328P, 16 MHz.
 */


// -- Includes ---------------------------------------------
#include <twi.h>



// -- Functions --------------------------------------------

/**
 * @brief  Initialize TWI unit, enable internal pull-ups, and set SCL frequency.
 * @return none
 */
void twi_init(void)
{
    /* Enable internal pull-up resistors */
    DDR(TWI_PORT) &= ~((1<<TWI_SDA_PIN) | (1<<TWI_SCL_PIN));
    TWI_PORT |= (1<<TWI_SDA_PIN) | (1<<TWI_SCL_PIN);


    /* Set SCL frequency */
    TWSR &= ~((1<<TWPS1) | (1<<TWPS0));
    TWBR = TWI_BIT_RATE_REG;
}



/**
 * @brief  Start communication on I2C/TWI bus.
 * @return none
 */
void twi_start(void)
{
    /* Send Start condition */
    TWCR = (1<<TWINT) | (1<<TWSTA) | (1<<TWEN);
    while ((TWCR & (1<<TWINT)) == 0);
}



/**
 * @brief  Write one byte to the I2C/TWI bus.
 * @param  data Byte to be transmitted.
 * @return 0 if ACK received, 1 if NACK received.
 */
uint8_t twi_write(uint8_t data)
{
    uint8_t twi_status;


    /* Send SLA+R, SLA+W, or data byte on I2C/TWI bus */
    TWDR = data;
    TWCR = (1<<TWINT) | (1<<TWEN);
    while ((TWCR & (1<<TWINT)) == 0);


    /* Check value of TWI status register */
    twi_status = TWSR & 0xf8;


    /* Status Code:
         - 0x18: SLA+W has been transmitted and ACK received
         - 0x28: Data byte has been transmitted and ACK has been received
         - 0x40: SLA+R has been transmitted and ACK received
    */
    if (twi_status == 0x18 || twi_status == 0x28 || twi_status == 0x40)
        return 0;   /* ACK received */
    else
        return 1;   /* NACK received */
}



/**
 * @brief  Read one byte from the I2C/TWI bus.
 * @param  ack TWI_ACK to acknowledge receipt, TWI_NACK to stop.
 * @return Received data byte.
 */
uint8_t twi_read(uint8_t ack)
{
    if (ack == TWI_ACK)
        TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWEA);
    else
        TWCR = (1<<TWINT) | (1<<TWEN);
    while ((TWCR & (1<<TWINT)) == 0);


    return (TWDR);
}



/**
 * @brief  Generate Stop condition on I2C/TWI bus.
 * @return none
 */
void twi_stop(void)
{
    TWCR = (1<<TWINT) | (1<<TWSTO) | (1<<TWEN);
}



/**
 * @brief  Test presence of one I2C device on the bus.
 * @param  addr Slave address (7-bit).
 * @return 0 if device present (ACK), 1 otherwise.
 */
uint8_t twi_test_address(uint8_t addr)
{
    uint8_t ack;  // ACK response from Slave


    twi_start();
    ack = twi_write((addr<<1) | TWI_WRITE);
    twi_stop();


    return ack;
}



/**
 * @brief  Read data from specific memory address of a peripheral.
 * @param  addr    Slave address (7-bit).
 * @param  memaddr Internal memory address to start reading from.
 * @param  buf     Pointer to buffer for received data.
 * @param  nbytes  Number of bytes to read.
 * @return none
 */
void twi_readfrom_mem_into(uint8_t addr, uint8_t memaddr, volatile uint8_t *buf, uint8_t nbytes)
{
    twi_start();
    if (twi_write((addr<<1) | TWI_WRITE) == 0)
    {
        // Set starting address
        twi_write(memaddr);
        twi_stop();


        // Read data into the buffer
        twi_start();
        twi_write((addr<<1) | TWI_READ);
        if (nbytes >= 2)
        {
            for (uint8_t i=0; i<(nbytes-1); i++)
            {
                *buf++ = twi_read(TWI_ACK);
            }
        }
        *buf = twi_read(TWI_NACK);
        twi_stop();
    }
    else
    {
        twi_stop();
    }
    
}


/**
 * @brief  Write one byte to specific memory address of a peripheral.
 * @param  addr    Slave address (7-bit).
 * @param  memaddr Internal memory address to write to.
 * @param  data    Data byte to write.
 * @return 0 on success (ACK), 1 on failure.
 */
uint8_t twi_writeto_mem(uint8_t addr, uint8_t memaddr, uint8_t data)
{
    twi_start();
    /* Send SLA+W and check ACK */
    if (twi_write((addr<<1) | TWI_WRITE) == 0)
    {
        /* Send memory address */
        if (twi_write(memaddr) == 0)
        {
            /* Send data byte */
            if (twi_write(data) == 0)
            {
                twi_stop();
                return 0; /* success (ACKs received) */
            }
        }
    }
    twi_stop();
    return 1; /* failure (NACK or other error) */
}


/**
 * @brief  Write two bytes (16-bit) to specific memory address.
 * @param  addr          Slave address (7-bit).
 * @param  memaddr       Internal memory address to write to.
 * @param  dataUpperHalf MSB of data.
 * @param  dataLowerHalf LSB of data.
 * @return 0 on success (ACK), 1 on failure.
 */
uint8_t twi_writeto_mem_16b(uint8_t addr, uint8_t memaddr, uint8_t dataUpperHalf,uint8_t dataLowerHalf)
{
    twi_start();
    /* Send SLA+W and check ACK */
    if (twi_write((addr<<1) | TWI_WRITE) == 0) //vyzva na komunikaciu
    {
        /* Send memory address */
        if (twi_write(memaddr) == 0)    //zapis adresy pociatocnej
        {
            /* Send data byte */
            if (twi_write(dataUpperHalf) == 0) //zapis upperHalf bytu
            {
                if (twi_write(dataLowerHalf) == 0) //zapis lowerHalf bytu
                {
                    twi_stop();
                    return 0; /* success (ACKs received) */
                }
            
            }
        }
    }
    twi_stop();
    return 1; /* failure (NACK or other error) */
}
