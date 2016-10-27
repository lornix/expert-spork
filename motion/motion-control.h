#ifndef MOTION_CONTROL_H

#  define MOTION_CONTROL_H

#  include <pins_arduino.h>

// shouldn't happen, but still...
#  if defined(ABS8)||defined(ABS16)
#    undef ABS8
#    undef ABS16
#  endif

// only evaluates (x) once
// yes, could use 'typeof' here, but then would evaluate (x) twice
#  define ABS8(x) ({ int8_t _x = (x); ((_x)>=0)?(_x):(-(_x)); })
#  define ABS16(x) ({ int16_t _x = (x); ((_x)>=0)?(_x):(-(_x)); })

// remove Serial statements if not debugging
// 1.5K+ ram difference!
// #  ifdef SERIAL
// #    define Sbegin(x)   Serial.begin(x)
// #    define Sprintln(...) Serial.println(__VA_ARGS__)
// #    define Sprint(...)   Serial.print(__VA_ARGS__)
// #  else
// #    define Sbegin(x)
// #    define Sprintln(...)
// #    define Sprint(...)
// #  endif

// Interrupt volatile variables for state information
struct state {
    // input B1
    volatile bool button1;
    // drive mode
    volatile uint8_t drivemode;
    // SPI digital pot settings:
    volatile uint8_t joyx;
    volatile uint8_t joyy;
    // value set ratio 0-10 (SPEED_MAX)
    uint8_t speedknob;
    // set from speedknob, proportional 0-10 = 0-255
    volatile uint8_t speedknob_real;
    // LED status bit-mapped (---43210)
    volatile uint8_t leds;
    // LED blink selector
    volatile uint8_t flash;
    //
    // used to compute (x,y) from angle/push
    // joystick angle -180..180
    int16_t angle;
    // joystick push (0..10)
    uint8_t push;
} state;

// mostly for convienence and readability
const uint8_t JOY_STOP = 128;
const uint8_t JOY_DELTA_MAX = 100;
const uint8_t JOY_PUSH_MAX = 10;
const uint8_t JOY_LEFT = (-JOY_DELTA_MAX);
const uint8_t JOY_RIGHT = (JOY_DELTA_MAX);
const uint8_t JOY_FORWARD = (JOY_DELTA_MAX);
const uint8_t JOY_BACKWARD = (-JOY_DELTA_MAX);
const uint8_t SPEED_STOP = 0;
const uint8_t SPEED_MAX = 10;
const uint8_t DRIVEMODE_OFF = 0;
const uint8_t DRIVEMODE_ONE = 1;
const uint8_t DRIVEMODE_TWO = 2;
const uint8_t DRIVEMODE_MAX = 2;
const uint16_t DRIVEMODE_DELAY_MS = 1000;
const uint8_t LED0 = 0x01;
const uint8_t LED1 = 0x02;
const uint8_t LED2 = 0x04;
const uint8_t LED3 = 0x08;
const uint8_t LED4 = 0x10;
const uint8_t ALL_OFF = 0;
const uint8_t ALL_ON = LED0 | LED1 | LED2 | LED3 | LED4;
//
// where is hardware attached?
//
// AD520x potentiometers
const uint8_t JOY_X_POT = 0;
const uint8_t JOY_Y_POT = 1;
const uint8_t SPEED_POT = 2;
// nano Digital pins
const uint8_t DM_SWITCH1_PIN = 2;
const uint8_t DM_SWITCH2_PIN = 4;
const uint8_t BUTTON1_PIN = 3;
const uint8_t LED0_PIN = 5;
const uint8_t LED1_PIN = 6;
const uint8_t LED2_PIN = 7;
const uint8_t LED3_PIN = 8;
const uint8_t LED4_PIN = 9;
// nano SPI bus
const uint8_t SPI_SS_PIN = 10;
const uint8_t SPI_MOSI_PIN = 11;
const uint8_t SPI_MISO_PIN = 12;
const uint8_t SPI_SCK_PIN = 13;
// nano analog pins
const uint8_t ADC_SPI0_PIN = A0;
const uint8_t ADC_SPI1_PIN = A1;
const uint8_t ADC_SPI2_PIN = A2;
// nano I2C bus
const uint8_t I2C_SDA_PIN = A4;
const uint8_t I2C_SCL_PIN = A5;

#endif
