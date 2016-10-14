/*
 * motion-control:
 * Accept commands from I2C bus, activate appropriate outputs to cause
 * desired actions.
 *
 * I2C address: 0x10
 *
 * I2C Bus (Pins A4+A5) used for inter-device communications
 * Command codes (TBD)
 * Response codes (TBD)
 *
 *  Inputs: Hardware
 *      BUTTON1 - Digital Input D2 - Illuminated button
 *
 *  Possibly only for testing, ADC reads are expensive
 *      SPI0 - Analog Input A0 - Joystick X sense (0-1023, 0-5VDC)
 *      SPI1 - Analog Input A1 - Joystick Y sense (0-1023, 0-5VDC)
 *      SPI2 - Analog Input A2 - Speed knob sense (0-1023, 0-5VDC)
 *
 *  Outputs: Hardware
 *
 *  SPI uses 4 pins, 10,11,12,13 for 1 device (AD5206 Digital Pot)
 *      SPI0 -> AD5206 Digital Pot - Joystick X (32-192, 128 default)
 *          Usable Range: 1.00vdc -> 4.00vdc range, 2.500vdc default
 *      SPI1 -> AD5206 Digital Pot - Joystick Y (32-192, 128 default)
 *          Usable Range: 1.00vdc -> 4.00vdc range, 2.500vdc default
 *      SPI2 -> AD5206 Digital Pot - Speed Knob (0-255, 0 default)
 *          Turtle to Rabbit speed setting
 *
 *  Digital Outputs use 1 pin each:
 *      LIGHT1 -> Digital Output D3 - Illuminated button lamp
 *      SW1 -> Digital Output D4 - Profile switch 1
 *      SW2 -> Digital Output D5 - Profile Switch 2
 *          Logic:    SW1-Off   SW1-On
 *          SW2-Off   DRIVE-2   OFF
 *          SW2-On    DRIVE-1   *NOT*ALLOWED*
 *      Indicators: (charlieplexed)
 *          LED1 - Digital Output D6
 *          LED2 - Digital Output D7
 *          LED3 - Digital Output D8
 *          LED4 - Digital Output D9
 *
 *  For all Pots (Digital AD5206 or otherwise), Terminal B is ALWAYS
 *  ground, Terminal A is ALWAYS VCC (5VDC), Wiper W varies (0=B,255=A).
 *
 *  Pins used on Arduino Pro Mini (3.3v) or Nano (5.0v)
 *      SPI0 AIN  A0 --- D2 DIN BUTTON1
 *      SPI1 AIN  A1 --- D3 OUT LIGHT1
 *      SPI2 AIN  A2 --- D4 OUT SW1
 *      ?    ?    A3 --- D5 OUT SW2
 *      SDA  I2C  A4 --- D6 OUT LED1
 *      SCL  I2C  A5 --- D7 OUT LED2
 *      SS   SPI D10 --- D8 OUT LED3
 *      MOSI SPI D11 --- D9 OUT LED4
 *      MISO SPI D12 --- D0 SER RX
 *      SCK  SPI D13 --- D1 SER TX
 */

#include "motion-control.h"

void setDefaultState()
{
    // set power-on state of variables so interrupt routine will output good
    // values to hardware
    // set button1 pin to input+pullup
    pinMode(BUTTON1_PIN, INPUT);
    digitalWrite(BUTTON1_PIN, HIGH);
    state.button1 = false;
    // the light in the button, off
    pinMode(LIGHT1_PIN, OUTPUT);
    digitalWrite(LIGHT1_PIN, LOW);
    state.light1 = false;
    // initial setup of digital pots / SPI communication
    pinMode(SPI_SS_PIN, OUTPUT);
    digitalWrite(SPI_SS_PIN, HIGH);
    pinMode(SPI_MOSI_PIN, OUTPUT);
    digitalWrite(SPI_MOSI_PIN, HIGH);
    pinMode(SPI_SCK_PIN, OUTPUT);
    digitalWrite(SPI_SCK_PIN, HIGH);
    pinMode(SPI_MISO_PIN, INPUT);
    digitalWrite(SPI_MISO_PIN, LOW);
    // switch settings, no direct access to SWITCH1/2
    state.drivemode = DRIVEMODE_OFF;
    // Joystick initial position, centered @ 128,128
    state.joyx = JOY_STOP;
    state.joyy = JOY_STOP;
    // Speed knob initial position, turtle slow @ 0
    state.speedknob = SPEED_STOP;
    // LEDS
    state.leds = ALL_OFF;
    state.blinkers = ALL_OFF;
}

// Interrupt is called once a millisecond
ISR(TIMER0_COMPA_vect)
{
    // keep track of blinkers
    static uint8_t blinktick = 0;
    static bool blinkstate = false;
    static bool led_state = false;
    // update BUTTON1
    state.button1 = digitalRead(BUTTON1_PIN);
    // update LIGHT1
    digitalWrite(LIGHT1_PIN, (state.light1 == true) ? HIGH : LOW);
    if (state.drivemode == DRIVEMODE_ONE) {
        // DRIVEMODE_ONE
        digitalWrite(SWITCH1_PIN, LOW);
        digitalWrite(SWITCH2_PIN, HIGH);
    } else if (state.drivemode == DRIVEMODE_TWO) {
        // DRIVEMODE_TWO
        digitalWrite(SWITCH1_PIN, LOW);
        digitalWrite(SWITCH2_PIN, LOW);
    } else {
        // DRIVEMODE_OFF or ANYTHING else, shut down
        digitalWrite(SWITCH1_PIN, HIGH);
        digitalWrite(SWITCH2_PIN, LOW);
    }
    // update JOYX
    // update JOYY
    // update SPEEDKNOB
    // update LEDS
    blinkstate = (blinktick++ >> 7);

    led_state = blinkstate;
    if (!(state.blinkers & state.leds & LED1)) {
        led_state = state.leds & LED1;
    }
    digitalWrite(LED1_PIN, led_state);

    led_state = blinkstate;
    if (!(state.blinkers & state.leds & LED2)) {
        led_state = state.leds & LED2;
    }
    digitalWrite(LED2_PIN, led_state);

    led_state = blinkstate;
    if (!(state.blinkers & state.leds & LED3)) {
        led_state = state.leds & LED3;
    }
    digitalWrite(LED3_PIN, led_state);

    led_state = blinkstate;
    if (!(state.blinkers & state.leds & LED4)) {
        led_state = state.leds & LED4;
    }
    digitalWrite(LED4_PIN, led_state);
}

void setup()
{
    // Timer0 is already used for millis()
    // We'll use 'interrupt on compare match' to obtain a 1000Hz
    // interrupt.  The OCR0A exact value is irrelevant, I like 13.
    OCR0A = 0x0D;
    TIMSK0 |= _BV(OCIE0A);

    // initial state setup
    setDefaultState();
    // slow and reliable
    Serial.begin(9600);
}

int which = 0;

void loop()
{
    delay(250);
    state.light1 = false;
    Serial.print('0');
    delay(250);
    state.light1 = true;
    Serial.print('1');

    state.blinkers = ALL_OFF;

    state.leds = LED1;
    delay(250);
    state.leds |= LED2;
    delay(250);
    state.leds |= LED3;
    delay(250);
    state.leds |= LED4;
    delay(500);
    state.leds &= ~LED4;
    delay(250);
    state.leds &= ~LED3;
    delay(250);
    state.leds &= ~LED2;
    delay(250);
    state.leds &= ~LED1;
    delay(250);
    state.leds = ALL_OFF;
    delay(250);
}
