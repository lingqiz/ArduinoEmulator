// ===================
// Add the following section to the top of any arduino code
#include "function_overload.cpp"
#include "io_overload.cpp"

const char INPUT  = 0;
const char OUTPUT = 1;
const char INPUT_PULLUP = 3;
const char LOW  = 0;
const char HIGH = 1;
typedef bool boolean;
typedef unsigned char byte;
Serial SerialUSB;
SilentSerial Serial1;
SilentSerial Serial2;
Valve SPI{}; // This will not necessary in most code, but 

#define bitRead(value, bit) (((value) >> (bit)) & 0x01)
#define lowByte(w) ((uint8_t) ((w) & 0xff))
#define highByte(w) ((uint8_t) ((w) >> 8))
// ===================end emulator section


// If you use particular libraries, you may need to add extra code above.

// if you need to use virtual serial communication put
/*
   void setip(char* serialAddr){
      SerialUSB.setSerialAddr(serialAddr);
      ... the rest of setup
     }
*/

void setup(){
    pinMode(44, OUTPUT);
    pinMode(43, INPUT);
}


void loop()
{


    
}




