// function overload for bpod
// Overload function in Arduino default namescope 

#include <iostream>
#include <sys/time.h> // for clock() CLOCKS_PER_SEC
#include <stdlib.h>
#include <unistd.h>   // for sleep()
#include <stdlib.h>     // for atoi() and exit()
#include <string.h>     // for memset()
#include <unistd.h>     // for close()
#include <termios.h>
typedef unsigned char byte;

const int digitalPinsNumber = 53;
const int analogPinsNumber  = 11;
const int DEFAULT_SENSOR_DATA = 935;
const int POKED_SENSOR_DATA   = 200;

#define A0 (0 + digitalPinsNumber)
#define A1 (1 + digitalPinsNumber)
#define A2 (2 + digitalPinsNumber)
#define A3 (3 + digitalPinsNumber)
#define A4 (4 + digitalPinsNumber)
#define A5 (5 + digitalPinsNumber)
#define A6 (6 + digitalPinsNumber)
#define A7 (7 + digitalPinsNumber)
#define A8 (8 + digitalPinsNumber)
#define A9 (9 + digitalPinsNumber)
#define A10 (10 + digitalPinsNumber)
#define A11 (11 + digitalPinsNumber) 

// ===== Shared Mem Buffer
byte valveState = 0;
int digitalPins[digitalPinsNumber] = { }; 
byte digitalPinsMode[digitalPinsNumber] = { };

int analogPins[analogPinsNumber] = { };
byte analogPinsMode[analogPinsNumber] = { };

void pinMode(byte line, char mode)
{
    if(line < digitalPinsNumber)
        digitalPinsMode[line] = mode; 
        
    else // A0 - A11 indexing
    {
        line -= digitalPinsNumber;
        analogPinsMode[line] = mode;
    }
    return;
}

void pinWrite(byte line, int data)
{
    if(line < digitalPinsNumber)
        digitalPins[line] = data;
    else
    {
        line -= digitalPinsNumber;
        analogPins[line] = data;
    }
}

void analogWrite(byte line, byte data)
{
    pinWrite(line, data);
}

void digitalWrite(byte line, byte data)
{
    pinWrite(line, data);
}

int pinRead(byte line)
{
    if(line < digitalPinsNumber)
        return digitalPins[line];
    
    line -= digitalPinsNumber;
    return analogPins[line];

}

int analogRead(byte line)
{
    return pinRead(line);
}

byte digitalRead(byte line)
{
    return pinRead(line);
}

void valveWrite(int value)
{
    valveState = value;
    return;
}

// Timing
const unsigned long SEC_TO_MILLSEC = 1000;
const unsigned long MILLSEC_TO_MICROSEC = 1000;
unsigned long millis()
{
    return ((float) clock()) / ((float) CLOCKS_PER_SEC) * SEC_TO_MILLSEC;
}

unsigned long micros()
{
    return ((float) clock()) / ((float) CLOCKS_PER_SEC) * SEC_TO_MILLSEC * MILLSEC_TO_MICROSEC;
}

void delay(unsigned long millsec)
{
    usleep(millsec * MILLSEC_TO_MICROSEC);
}

void udelay(unsigned long microsec)
{
    usleep(microsec);
}







