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

#include <Wire.h>

void i2c_scanner(void)
{
    byte error, address;
    int nDevices=0;
    // delay to let serial monitor connection
    delay(2000);
    Wire.begin();
    for (address = 0; address < 127; address++) {
        // The i2c_scanner uses the return value of
        // the Write.endTransmission to see if
        // a device did acknowledge to the address.
        Wire.beginTransmission(address);
        error = Wire.endTransmission();
        if (error == 0) {
            Serial.print(F("I2C device found at address 0x"));
            if (address<16) {
                Serial.print(F("0"));
            }
            Serial.println(address,HEX);
            nDevices++;
        }
        else if (error==4) {
            Serial.print(F("Unknown error at address 0x"));
            if (address<16) {
                Serial.print(F("0"));
            }
            Serial.println(address,HEX);
        }
    }
    if (nDevices == 0) {
        Serial.println(F("No I2C devices found."));
    }
}

void setup()
{
    Serial.begin(115200);
    // i2c_scanner();
    Wire.begin(); // master
    delay(1);
    Wire.beginTransmission(0x52);
    Wire.write(0xF0);
    Wire.write(0x55);
    Wire.endTransmission();
    delay(1);
    Wire.beginTransmission(0x52);
    Wire.write(0xFB);
    Wire.write(0x00);
    Wire.endTransmission();
}

void send_zero()
{
    Wire.beginTransmission(0x52);
    Wire.write(0x00);
    Wire.endTransmission();
}

char str[]="\r(+xxx,+xxx) X: +xxx  Y: +xxx  Z: +xxx  C: +xxx  Z: +xxx";
//          "01234567890123456789012345678901234567890123456789012345";
//          "00000000001111111111222222222233333333334444444444555555";

const char digits[]="0123456789";

void putval(int val, int pos)
{
    str[pos+0]=(val<0)?'-':' ';
    if (val<0) {
        val=-val;
    }
    byte v1=val/100;
    byte v2=(val%100)/10;
    byte v3=(val%10);
    str[pos+1]=digits[v1];
    str[pos+2]=digits[v2];
    str[pos+3]=digits[v3];
    if (val<100) str[pos+1]=' ';
    if (val<10) str[pos+2]=' ';
}

void loop()
{
    byte cnt;
    byte buf[6];
    while (1) {
        cnt=0;
        send_zero();
        Wire.requestFrom(0x52,6);
        while (Wire.available()) {
            buf[cnt++]=Wire.read();
        }
        putval(buf[0]-128, 2);
        putval(buf[1]-128, 7);
        putval(buf[2]-128,16);
        putval(buf[3]-128,25);
        putval(buf[4]-128,34);
        putval((buf[5]&2)==0,43);
        putval((buf[5]&1)==0,52);
        Serial.print(str);
    }
}
