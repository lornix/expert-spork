/*
 * low-level.h
 * Handle setting pins, digitalWrites and such
 */

/*
 * These routines allow optimizations to be
 * made for frequently used functions.
 */

void static inline digWrite(uint8_t pin, uint8_t state)
{
    digitalWrite(pin, state);
}

void static inline pinModeSet(uint8_t pin, uint8_t dir, uint8_t state)
{
    pinMode(pin, dir);
    digWrite(pin, state);
}
