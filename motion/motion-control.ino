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
 *      SW1  -> Digital Output D3 - Profile switch 1
 *      SW2  -> Digital Output D4 - Profile Switch 2
 *          Logic:    SW1-Off   SW1-On
 *          SW2-Off   DRIVE-2   OFF
 *          SW2-On    DRIVE-1   *NOT*ALLOWED*
 *      Indicators: (charlieplexed)
 *          LED0 - Digital Output D5 - Illuminated button lamp
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
 *      SPI1 AIN  A1 --- D3 OUT SW1
 *      SPI2 AIN  A2 --- D4 OUT SW2
 *      ?    ?    A3 --- D5 OUT LED0
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
    // LEDS - Bulb check
    state.flash = ALL_OFF;
    state.leds = ALL_ON;
    delay(1000);
    state.leds = ALL_OFF;
    delay(500);
}

// Interrupt is called once a millisecond
ISR(TIMER0_COMPA_vect)
{
    static uint8_t ISRtick = 0;
    // keep track of flash
    static bool blinkstate = false;
    static uint8_t led_state;

    // Heartbeat ticker, used for timing
    ISRtick++;

    //
    // update LEDS
    // use ISRtick to toggle blinkstate, gives ~2Hz blink rate
    if (!ISRtick) {
        blinkstate = !blinkstate;
    }
    //
    // only update LED's every 16th tick (64Hz)
    if (!(ISRtick & 0x0f)) {
        //
        // LED0
        led_state = state.leds & LED0;
        if (state.flash & led_state) {
            led_state = blinkstate;
        }
        digitalWrite(LED0_PIN, led_state);
        //
        // LED1
        led_state = state.leds & LED1;
        if (state.flash & led_state) {
            led_state = blinkstate;
        }
        digitalWrite(LED1_PIN, led_state);
        //
        // LED2
        led_state = state.leds & LED2;
        if (state.flash & led_state) {
            led_state = blinkstate;
        }
        digitalWrite(LED2_PIN, led_state);
        //
        // LED3
        led_state = state.leds & LED3;
        if (state.flash & led_state) {
            led_state = blinkstate;
        }
        digitalWrite(LED3_PIN, led_state);
        //
        // LED4
        led_state = state.leds & LED4;
        if (state.flash & led_state) {
            led_state = blinkstate;
        }
        digitalWrite(LED4_PIN, led_state);
    }

    //
    // only update every 32nd tick (32Hz)
    if ((ISRtick & 0x1F) == 0) {
        //
        // set SWITCH1/2 based on state.drivemode
        if (state.drivemode == DRIVEMODE_ONE) {
            digitalWrite(SWITCH1_PIN, LOW);
            digitalWrite(SWITCH2_PIN, HIGH);
        } else if (state.drivemode == DRIVEMODE_TWO) {
            digitalWrite(SWITCH1_PIN, LOW);
            digitalWrite(SWITCH2_PIN, LOW);
        } else {
            // DRIVEMODE_OFF or ANYTHING else, shut down
            digitalWrite(SWITCH1_PIN, HIGH);
            digitalWrite(SWITCH2_PIN, LOW);
        }
        //
        // update BUTTON1 (Will this need debounce?)
        state.button1 = digitalRead(BUTTON1_PIN);
        //
        // update JOYX, JOYY, SPEEDKNOB
        // involves sending three bytes to SPI bus for AD5206 Digital Pot
    }
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

void loop()
{
    // state.light1 = true;
    // Serial.print('1');
    // delay(250);
    // state.light1 = false;
    // Serial.print('0');
    // delay(250);

    state.leds = LED2 | LED4;
    delay(1000);
    state.leds = LED1 | LED3;
    delay(1000);
}
