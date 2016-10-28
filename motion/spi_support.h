/*
 * spi_support.h
 * Setup, transmit & receive on SPI bus
 */

void static Init_SPI()
{
    // initial setup of SPI module

    pinModeSet(SPI_MOSI_PIN, OUTPUT, HIGH);
    pinModeSet(SPI_SCK_PIN, OUTPUT, HIGH);
    pinModeSet(SPI_MISO_PIN, INPUT, HIGH);
    // SS pin MUST be OUTPUT/HIGH for Master SPI
    pinModeSet(SPI_SS_PIN, OUTPUT, HIGH);
    // set up initial SPI ports & control registers
    SPCR = (1 << SPE) | (1 << MSTR);
    // clear SPI2X in Status register, no 2X speed!
    SPSR = 0;
}

void static SPI_sendPots(uint8_t pot, uint8_t value)
{
    // used inside interrupt routine, no INTERRUPTS!
    // so, no delay or serial
    //
    // send two bytes to SPI bus (AD520x)
    // ignore returned data, AD520x doesn't talk
    //
    // select AD520x device (nCS = LOW)
    digWrite(SPI_SS_PIN, LOW);
    SPDR = pot;
    while (!(SPSR & 0x80)) ;
    SPDR = value;
    while (!(SPSR & 0x80)) ;
    // allow AD520x to latch value (nCS = HIGH)
    digWrite(SPI_SS_PIN, HIGH);
}
