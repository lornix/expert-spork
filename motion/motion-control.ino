/*
 * motion-control:
 * Accept commands from I2C bus, activate appropriate outputs to cause
 * desired actions.
 */

#define DEBUG

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

    Sprint(__func__);
    Sprint(": ");

    pinMode(SPI_MOSI_PIN, OUTPUT);
    pinMode(SPI_SCK_PIN, OUTPUT);
    pinMode(SPI_MISO_PIN, INPUT);
    // SS pin MUST be OUTPUT/HIGH for Master SPI
    pinModeSet(SPI_SS_PIN, OUTPUT, HIGH);
    // set up initial SPI ports & control registers
    SPCR = (1 << SPE) | (1 << MSTR);
    // clear SPI2X in Status register, no 2X speed!
    SPSR = 0;

    Sprintln("Done");
}

void initDefaultState()
{
    Sprint(__func__);
    Sprint(": ");

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
    Sprint("(Lamp Check) ");
    setFlash(ALL_OFF);
    setLEDS(ALL_ON);
    delay(2000);
    Sprint("(All Off) ");
    setLEDS(ALL_OFF);
    delay(500);

    Sprintln("Done");
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
        // remember logic is reversed due to relay interface board
        if (state.drivemode == DRIVEMODE_ONE) {
            digitalWrite(DM_SWITCH1_PIN, LOW);
            digitalWrite(DM_SWITCH2_PIN, HIGH);
        } else if (state.drivemode == DRIVEMODE_TWO) {
            digitalWrite(DM_SWITCH1_PIN, LOW);
            digitalWrite(DM_SWITCH2_PIN, LOW);
        } else {
            // DRIVEMODE_OFF or ANYTHING else, shut down
            digitalWrite(DM_SWITCH1_PIN, HIGH);
            digitalWrite(DM_SWITCH2_PIN, HIGH);
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
    Sbegin(9600);
    Sprint(__func__);
    Sprintln(": ");

    // set timer0 interrupt for 1024 ticks/sec
    Init_Timer0Interrupt();

    // initialize SPI system
    Init_SPI();

    // initial state setup
    initDefaultState();

    Sprintln("Setup complete");
}

void loop()
{
    setJoy(0, 0);
    setDrivemode(DRIVEMODE_ONE);
    while (1) ;
}
