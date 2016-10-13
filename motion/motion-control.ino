/*
   Blink
   Turns on an LED on for one second, then off for one second, repeatedly.

   Most Arduinos have an on-board LED you can control. On the Uno and
   Leonardo, it is attached to digital pin 13. If you're unsure what
   pin the on-board LED is connected to on your Arduino model, check
   the documentation at http:

   This example code is in the public domain.

   modified 8 May 2014
   by Scott Fitzgerald
 */

void setup() {
    pinMode(13, OUTPUT);
    Serial.begin(38400);
}

#define DELAYMS 500

void loop() {
    digitalWrite(13, HIGH);
    Serial.print('1');
    delay(DELAYMS);

    digitalWrite(13, LOW);
    Serial.print('0');
    delay(DELAYMS);
}
