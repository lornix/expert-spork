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
 *          < 128 LEFT, > 128 RIGHT
 *      SPI1 -> AD5206 Digital Pot - Joystick Y (32-192, 128 default)
 *          Usable Range: 1.00vdc -> 4.00vdc range, 2.500vdc default
 *          < 128 FORWARD, > 128 BACKWARD.
 *          Flipped for sanity in SetJoy function
 *      SPI2 -> AD5206 Digital Pot - Speed Knob (0-255, 0 default)
 *          Turtle to Rabbit speed setting
 *
 *  Digital Outputs use 1 pin each:
 *      SW1  -> Digital Output D3 - Profile switch 1
 *      SW2  -> Digital Output D4 - Profile Switch 2
 *          Logic:    SW1-Off   SW1-On
 *          SW2-Off   DRIVE-2   OFF
 *          SW2-On    DRIVE-1   *NOT*ALLOWED*
 *      Indicators:
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

// #define DEBUG

#include "motion-control.h"

void inline setLEDS(uint8_t mask)
{
    state.leds = mask;
}

void inline setFlash(uint8_t mask)
{
    state.flash = mask;
}

void inline setDrivemode(uint8_t mode)
{
    if (mode > DRIVEMODE_MAX) {
        mode = 0;
    }
    state.drivemode = mode;
}

void inline setSpeedKnob(uint8_t speedknob)
{
    state.speedknob = speedknob;
}

void inline setJoy(int8_t xpos, int8_t ypos)
{
    // either out of range?  ignore request
    if ((ABS8(xpos) > JOY_DELTA_MAX) || (ABS8(ypos) > JOY_DELTA_MAX)) {
        // out of range?   STOP!!!
        xpos = 0;
        ypos = 0;
        // return
    }
    // -xpos is left, +xpos is right
    state.joyx = JOY_STOP + xpos;
    // flip sign of fwd/bkw signal
    // -ypos is backwards, +ypos is forwards
    state.joyy = JOY_STOP - ypos;
}

void inline pinModeSet(uint8_t pin, uint8_t dir, uint8_t state)
{
    pinMode(pin, dir);
    digitalWrite(pin, state);
}

void Init_Timer0Interrupt()
{
    // Timer0 is already used for millis()
    // We'll use 'interrupt on compare match' to obtain a 1000Hz
    // interrupt.  The OCR0A exact value is irrelevant, I like 13.
    OCR0A = 0x0D;
    TIMSK0 |= _BV(OCIE0A);
}

void Init_SPI()
{
    // initial setup of SPI module

    SPrint(__func__);
    SPrint(": ");

    pinMode(SPI_MOSI_PIN, OUTPUT);
    pinMode(SPI_SCK_PIN, OUTPUT);
    pinMode(SPI_MISO_PIN, INPUT);
    // SS pin MUST be OUTPUT/HIGH for Master SPI
    pinModeSet(SPI_SS_PIN, OUTPUT, HIGH);
    // set up initial SPI ports & control registers
    SPCR = (1 << SPE) | (1 << MSTR);
    // clear SPI2X in Status register, no 2X speed!
    SPSR = 0;

    SPrintln("Done");
}

void initDefaultState()
{
    SPrint(__func__);
    SPrint(": ");

    // set power-on state of variables so interrupt routine
    // will output proper values to hardware
    //
    // set button1 pin to input+pullup
    pinModeSet(BUTTON1_PIN, INPUT, HIGH);
    //
    // LEDS 0-4
    pinModeSet(LED0_PIN, OUTPUT, LOW);
    pinModeSet(LED1_PIN, OUTPUT, LOW);
    pinModeSet(LED2_PIN, OUTPUT, LOW);
    pinModeSet(LED3_PIN, OUTPUT, LOW);
    pinModeSet(LED4_PIN, OUTPUT, LOW);
    //
    // switch settings, no direct access to DM_SWITCH1/2 after this
    // set mode for switch outputs (default OFF, 1/0)
    setDrivemode(DRIVEMODE_OFF);
    pinModeSet(DM_SWITCH1_PIN, OUTPUT, HIGH);
    pinModeSet(DM_SWITCH2_PIN, OUTPUT, LOW);
    //
    // Joystick initial position, centered @ 128,128
    setJoy(0, 0);
    //
    // Speed knob initial position, turtle slow @ 0
    setSpeedKnob(0);
    //
    // LEDS - Bulb check
    SPrint("(Lamp Check) ");
    setFlash(ALL_OFF);
    setLEDS(ALL_ON);
    delay(2000);
    SPrint("(All Off) ");
    setLEDS(ALL_OFF);
    delay(500);

    SPrintln("Done");
}

void SPI_send(uint8_t pot, uint8_t value)
{
    // used inside interrupt routine, no INTERRUPTS!
    // so, no delay or serial
    //
    // send two bytes to SPI bus (AD520x)
    // ignore returned data, AD520x doesn't talk
    //
    // select AD520x device (nCS = LOW)
    digitalWrite(SPI_SS_PIN, LOW);
    SPDR = pot;
    while (!(SPSR & 0x80)) ;
    SPDR = value;
    while (!(SPSR & 0x80)) ;
    // allow AD520x to latch value (nCS = HIGH)
    digitalWrite(SPI_SS_PIN, HIGH);
}

// Interrupt ticks at 1024Hz
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
        //
        // update BUTTON1 (Will this need debounce?)
        state.button1 = digitalRead(BUTTON1_PIN);
    }
    //
    // only update every 128th tick (8Hz)
    if ((ISRtick & 0x7F) == 0) {
        //
        // set DM_SWITCH1/2 based on state.drivemode
        if (state.drivemode == DRIVEMODE_ONE) {
            digitalWrite(DM_SWITCH1_PIN, LOW);
            digitalWrite(DM_SWITCH2_PIN, LOW);
        } else if (state.drivemode == DRIVEMODE_TWO) {
            digitalWrite(DM_SWITCH1_PIN, LOW);
            digitalWrite(DM_SWITCH2_PIN, HIGH);
        } else {
            // DRIVEMODE_OFF or ANYTHING else, shut down
            digitalWrite(DM_SWITCH1_PIN, HIGH);
            digitalWrite(DM_SWITCH2_PIN, LOW);
        }
        //
        // update JOYX, JOYY, SPEEDKNOB
        // involves sending three bytes to SPI bus for AD5206 Digital Pot
        // pot 1 (Joystick X)
        SPI_send(JOY_X_POT, state.joyx);
        // pot 2 (joystick Y)
        SPI_send(JOY_Y_POT, state.joyy);
        // pot 3 (Speed Knob)
        SPI_send(SPEED_POT, state.speedknob);
    }
}

void setup()
{
    // slow and reliable
    SBegin(9600);
    SPrint(__func__);
    SPrintln(": ");

    // set timer0 interrupt for 1024 ticks/sec
    Init_Timer0Interrupt();

    // initialize SPI system
    Init_SPI();

    // initial state setup
    initDefaultState();

    SPrintln("Setup complete");
}

void loop()
{
    const uint16_t wait = 1500;
    const uint16_t longwait = wait * 5;

    setJoy(0, 0);
    setDrivemode(DRIVEMODE_ONE);
    delay(3000);
    setDrivemode(DRIVEMODE_TWO);
    delay(3000);
    setDrivemode(DRIVEMODE_OFF);
    delay(3000);
    return;
    setJoy(0, JOY_FORWARD);
    SPrintln("Forward");
    delay(longwait);
    setJoy(0, 0);
    delay(wait);
    setJoy(JOY_RIGHT, 0);
    SPrintln("Right");
    delay(longwait);
    setJoy(0, 0);
    delay(wait);
    setJoy(0, JOY_BACKWARD);
    SPrintln("Backward");
    delay(longwait);
    setJoy(0, 0);
    delay(wait);
    setJoy(JOY_LEFT, 0);
    SPrintln("Left");
    delay(longwait);
    setJoy(0, 0);
    delay(wait);
    setJoy(0, 0);
    SPrintln("Stop");
    delay(longwait);
}
