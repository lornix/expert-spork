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
 *          L0 ->    +     -     +     -
 *                  LED1  LED3   +     -
 *          L1 ->    -     +    LED5  LED6
 *                  LED2  LED4   -     +
 *          L2 ->    +     -     -     +
 *
 *          LED1 = L0-H L1-L L2-Z   H=High (Output High)
 *          LED2 = L0-Z L1-L L2-H   L=Low  (Output Low)
 *          LED3 = L0-L L1-H L2-Z   Z=Tri-state (Input)
 *          LED4 = L0-Z L1-H L2-L
 *          LED5 = L0-H L1-Z L2-L
 *          LED6 = L0-L L1-Z L2-H
 *
 *  For all Pots (Digital AD5206 or otherwise), Terminal B is ALWAYS
 *  ground, Terminal A is ALWAYS VCC (5VDC), Wiper W varies (0=B,255=A).
 *
 *  Pins used on Arduino Pro Mini (3.3v) or Nano (5.0v)
 *      A0  ADC SPI0 --- D2 IN  BUTTON1
 *      A1  ADC SPI1 --- D3 OUT LIGHT1
 *      A2  ADC SPI2 --- D4 OUT SW1
 *      A3  ?   ?    --- D5 OUT SW2
 *      A4  I2C SDA  --- D6 I/O L0
 *      A5  I2C SCL  --- D7 I/O L1
 *      D10 SPI SS   --- D8 I/O L2
 *      D11 SPI MOSI --- D9 ?   ?
 *      D12 SPI MISO --- D0 SER RX
 *      D13 SPI SCK  --- D1 SER TX
 */

#include "motion-control.h"

void setDefaultState()
{
    // set power-on state of variables so interrupt routine will output good
    // values to hardware
    SET_PIN_PULLUP(BUTTON1_PIN);
    state.button1=false;
    SET_PIN_LOW(LIGHT1_PIN);
    state.light1=false;
    // see logic chart above about switch1/switch2 defaults
    SET_PIN_HIGH(SWITCH1_PIN);
    state.switch1=true;
    SET_PIN_LOW(SWITCH2_PIN);
    state.switch2=false;
    // initial setup of digital pots / SPI communication
    SET_PIN_HIGH(SPI_SS_PIN);
    SET_PIN_HIGH(SPI_MOSI_PIN);
    SET_PIN_HIGH(SPI_SCK_PIN);
    SET_PIN_PULLUP(SPI_MISO_PIN);
    state.drivemode=DRIVEMODE_OFF;
    state.joyx=JOY_STOP;
    state.joyy=JOY_STOP;
    state.speedknob=SPEED_STOP;
    // LEDS, hardware off, but all on for self test. ISR will set
    SET_LED_PINS(PIN_HIZ,PIN_HIZ,PIN_HIZ);
    state.leds=0b00111111;
    // tell interrupt routine to update things
    state.outputs=NOTVALID;
    // busy loop until they're updated, this is important!
    while (state.outputs==NOTVALID);
}

// Interrupt is called once a millisecond
ISR(TIMER0_COMPA_vect)
{
    static uint8_t ledTick=1;
    static uint8_t ledTock=0;

    // outputs flagged as changed, needing update?
    if (state.outputs==NOTVALID) {
        // update BUTTON1
        state.button1=digitalRead(BUTTON1_PIN);
        // update LIGHT1
        digitalWrite(LIGHT1_PIN,(state.light1==true)?HIGH:LOW);
        // update SWITCH1
        digitalWrite(SWITCH1_PIN,(state.switch1==true)?HIGH:LOW);
        // update SWITCH2
        digitalWrite(SWITCH2_PIN,(state.switch2=true)?HIGH:LOW);
        // update JOYX
        // update JOYY
        // update SPEEDKNOB
        // update LEDS
        // we only update every 16 milliseconds
        //   it's probably going to flicker... adjust me!
        ledTick--;
        if (!ledTick) {
            ledTick=16;
            ledTock--;
            switch (ledTock) {
                case 6: // LED6
                    if (state.leds&0b00100000) {
                        SET_LED_PINS(PIN_LOW,PIN_HIZ,PIN_HIGH);
                    }
                    break;
                case 5: // LED5
                    if (state.leds&0b00010000) {
                        SET_LED_PINS(PIN_HIGH,PIN_HIZ,PIN_LOW);
                    }
                    break;
                case 4: // LED4
                    if (state.leds&0b00001000) {
                        SET_LED_PINS(PIN_HIZ,PIN_HIGH,PIN_LOW);
                    }
                    break;
                case 3: // LED3
                    if (state.leds&0b00000100) {
                        SET_LED_PINS(PIN_LOW,PIN_HIGH,PIN_HIZ);
                    }
                    break;
                case 2: // LED2
                    if (state.leds&0b00000010) {
                        SET_LED_PINS(PIN_HIZ,PIN_LOW,PIN_HIGH);
                    }
                    break;
                case 1: // LED1
                    if (state.leds&0b00000001) {
                        SET_LED_PINS(PIN_HIGH,PIN_LOW,PIN_HIZ);
                    }
                    break;
                default:
                    // turn everything off
                    SET_LED_PINS(PIN_HIZ,PIN_HIZ,PIN_HIZ);
                    // reset to repeat loop
                    ledTock=7;
                    break;
            }
        }
        state.outputs=VALID;
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
    delay(500);
    state.light1=false;
    state.outputs=NOTVALID;
    while (state.outputs==NOTVALID);
    Serial.print('0');
    delay(500);
    state.light1=true;
    state.outputs=NOTVALID;
    while (state.outputs==NOTVALID);
    Serial.print('1');
}
