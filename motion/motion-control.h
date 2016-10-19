#ifndef MOTION_CONTROL_H

#  define MOTION_CONTROL_H

#  include <pins_arduino.h>

// Interrupt volatile variables for state information
struct state {
    // input B1
    volatile bool button1;
    // output B1
    volatile bool led0;
    // output SW1
    volatile bool switch1;
    // output SW2
    volatile bool switch2;
    // drive mode
    volatile uint8_t drivemode;
    // SPI digital pot settings:
    volatile uint8_t joyx;
    volatile uint8_t joyy;
    volatile uint8_t speedknob;
    // LED status bit-mapped (---43210)
    volatile uint8_t leds;
    // LED blink selector
    volatile uint8_t flash;
} state;

// mostly for convienence and readability
const uint8_t JOY_STOP = 127;
const uint8_t SPEED_STOP = 0;
const uint8_t DRIVEMODE_OFF = 0;
const uint8_t DRIVEMODE_ONE = 1;
const uint8_t DRIVEMODE_TWO = 2;
const uint8_t LED0 = 0x01;
const uint8_t LED1 = 0x02;
const uint8_t LED2 = 0x04;
const uint8_t LED3 = 0x08;
const uint8_t LED4 = 0x10;
const uint8_t ALL_OFF = 0;
const uint8_t ALL_ON = LED0 | LED1 | LED2 | LED3 | LED4;
// where is hardware attached?
const uint8_t BUTTON1_PIN  = 2;

const uint8_t SWITCH1_PIN  = 3;
const uint8_t SWITCH2_PIN  = 4;

const uint8_t LED0_PIN     = 5;
const uint8_t LED1_PIN     = 6;
const uint8_t LED2_PIN     = 7;
const uint8_t LED3_PIN     = 8;
const uint8_t LED4_PIN     = 9;

const uint8_t SPI_SS_PIN   = 10;
const uint8_t SPI_MOSI_PIN = 11;
const uint8_t SPI_MISO_PIN = 12;
const uint8_t SPI_SCK_PIN  = 13;

const uint8_t ADC_SPI0_PIN = A0;
const uint8_t ADC_SPI1_PIN = A1;
const uint8_t ADC_SPI2_PIN = A2;

const uint8_t I2C_SDA_PIN  = A4;
const uint8_t I2C_SCL_PIN  = A5;

#endif
