/*
 * motion-control:
 * Accept commands from I2C bus, activate appropriate outputs to cause
 * desired actions.
 */

#include "motion-control.h"
#include "low-level.h"
#include "uart_support.h"
#include "spi_support.h"

void inline setLED(uint8_t mask)
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
        mode = DRIVEMODE_OFF;
    }
    state.drivemode = mode;
    if (mode!=DRIVEMODE_OFF) {
        // give time for control module to start
        delay(DRIVEMODE_DELAY_MS);
    }
}

void inline setSpeedKnob(uint8_t speedknob)
{
    if (speedknob > SPEED_MAX) {
        speedknob = SPEED_MAX;
    }
    state.speedknob = speedknob;
    // don't do math in interrupt routine
    state.speedknob_real = speedknob * 255 / SPEED_MAX;
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

void setAnglePush(int16_t angle, uint8_t push)
{
    if ((ABS16(angle)>180)||(push>JOY_PUSH_MAX)) {
        // out of range? zero!
        angle=0;
        push=0;
    }
    //
    // convert to radians
    // float angleRads = (float)angle * PI / 180.0;
    //
    // I don't really want to invoke sin/cos, they're
    // memory expensive (1.6K+!).  CPU time is ok, only invoked
    //  once to set joyx,joyy positions
    // int16_t x=sin(angleRads)*force;
    // int16_t y=cos(angleRads)*force;

    int16_t force=(JOY_DELTA_MAX*push)/JOY_PUSH_MAX;
    int16_t force7 = (force * 7) / 10;

    int16_t x=0;
    int16_t y=0;

    int16_t absangle=ABS16(angle);

    // decode 8 directions (4 here, then flipped across Y-axis)
    if (absangle<23) { // 0
        x=0; y=force;
    } else if ((absangle>22)&&(absangle<68)) { // 45
        x=force7; y=force7;
    } else if ((absangle>67)&&(absangle<113)) { // 90
        x=force; y=0;
    } else if ((absangle>112)&&(absangle<158)) { // 135
        x=force7; y=-force7;
    } else { // 180
        x=0; y=-force;
    }
    //
    // flip across Y-axis if angle negative
    if (angle<0) {
        x=-x;
    }
    setJoy(x,y);
}

void Init_Timer0Interrupt()
{
    // Timer0 is already used for millis()
    // We'll use 'interrupt on compare match' to obtain a 1000Hz
    // interrupt.  The OCR0A exact value is irrelevant, I like 13.
    OCR0A = 0x0D;
    TIMSK0 |= _BV(OCIE0A);
}

void initDefaultState()
{
    //
    // set power-on state of variables so interrupt routine
    // will output proper values to hardware
    //
    // set mode for switch outputs (default OFF)
    pinModeSet(DM_SWITCH1_PIN, OUTPUT, HIGH);
    pinModeSet(DM_SWITCH2_PIN, OUTPUT, HIGH);
    setDrivemode(DRIVEMODE_OFF);
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
    setFlash(ALL_OFF);
    setLED(ALL_OFF);
    //
    // Speed knob initial position, turtle slow @ 0
    setSpeedKnob(0);
    //
    // Joystick initial position, centered @ 128,128
    // setJoy(0, 0);
    //
    // set angle / push to 0's initially
    setAnglePush(0,0);
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
    // update LEDs
    // use ISRtick to toggle blinkstate, gives ~2Hz blink rate
    if (!ISRtick) {
        blinkstate = !blinkstate;
    }
    //
    // only update LED's every 16th tick (64Hz)
    if ((ISRtick & 0x0f) == 5 ) {
        // offset by 5 ticks so LEDs and SPI
        // don't occur same tick
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
    if ((ISRtick & 0x7F) == 7) {
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
        SPI_send(SPEED_POT, state.speedknob_real);
    }
}

void setup()
{
    // slow and reliable
    init_UART(9600);

    // set timer0 interrupt for 1024 ticks/sec
    Init_Timer0Interrupt();

    // initialize SPI system
    Init_SPI();

    // initial state setup
    initDefaultState();
}

void loop()
{
    static bool led=true;
    setDrivemode(DRIVEMODE_OFF);
    while (1) {
        UART_char(led+48);
        setLED(led?LED0|LED1:LED1);
        led=!led;
        delay(2000);
    }
}
