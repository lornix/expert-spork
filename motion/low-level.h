/*
 * low-level.h
 * Handle setting pins, digitalWrites and such
 */

void inline pinModeSet(uint8_t pin, uint8_t dir, uint8_t state)
{
    pinMode(pin, dir);
    digitalWrite(pin, state);
}
