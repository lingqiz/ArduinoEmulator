//
//  io_overload.cpp
//  Bpod_EM
//
//
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>

#include <stdlib.h>  // for exit()
#include <strings.h> // for bzero()
#include <unistd.h>  // for read()
#include <sys/ioctl.h>

typedef unsigned char byte;

// True & False constants
#define FALSE 0
#define TRUE 1

// Controls whether our "port" is on / off

/*
    Serial Class 
    Override orginal SPI, Serial class in Arduino global namescope 
    Communicate over socat to Matlab socket inferface 
*/

volatile int STOP = FALSE;
class Serial
{
    private:
    const int BAUDRATE = B9600;    
//    const byte _POSIX_SOURCE = 1;
    char* MODEMDEVICE;
    
    int fd, c, res;
    struct termios oldtio, newtio;
    
    char readBuffer[255], writeBuffer[255];
    fd_set readfs;
    
    public:
    void setSerialAddr(char* addr)
    {
        MODEMDEVICE = addr;
    }

    void begin(int rate)
    {
        /*
         Open modem device for reading and writing and not as controlling tty
         because we don't want to get killed if linenoise sends CTRL-C.
         */
        fd = open(MODEMDEVICE, O_RDWR | O_NOCTTY | O_NONBLOCK);
        if (fd <0) {perror(MODEMDEVICE); exit(-1); }
                                         
        tcgetattr(fd,&oldtio); // Save current serial port settings
        bzero(&newtio, sizeof(newtio)); // Clear struct for new port settings
        
        newtio.c_cflag = BAUDRATE | CRTSCTS | CS8 | CLOCAL | CREAD;
        newtio.c_iflag = IGNPAR;
        newtio.c_oflag = 0;
        // newtio.c_lflag = ICANON;        
        /*
         Initialize all control characters
         Default values can be found in /usr/include/termios.h
         */
        newtio.c_cc[VINTR]    = 0;     /* Ctrl-c */
        newtio.c_cc[VQUIT]    = 0;     /* Ctrl-\ */
        newtio.c_cc[VERASE]   = 0;     /* del */
        newtio.c_cc[VKILL]    = 0;     /* @ */
        newtio.c_cc[VEOF]     = 4;     /* Ctrl-d */
        newtio.c_cc[VTIME]    = 0;     /* inter-character timer unused */
        newtio.c_cc[VMIN]     = 1;     /* blocking read until 1 character arrives */        
        newtio.c_cc[VSTART]   = 0;     /* Ctrl-q */
        newtio.c_cc[VSTOP]    = 0;     /* Ctrl-s */
        newtio.c_cc[VSUSP]    = 0;     /* Ctrl-z */
        newtio.c_cc[VEOL]     = 0;     /* '\0' */
        newtio.c_cc[VREPRINT] = 0;     /* Ctrl-r */
        newtio.c_cc[VDISCARD] = 0;     /* Ctrl-u */
        newtio.c_cc[VWERASE]  = 0;     /* Ctrl-w */
        newtio.c_cc[VLNEXT]   = 0;     /* Ctrl-v */
        newtio.c_cc[VEOL2]    = 0;     /* '\0' */
        // newtio.c_cc[VSWTC]    = 0;     /* '\0' */ Does not work with mac
        
        
        // Now clean the modem line and activate the settings for the port
        tcflush(fd, TCIFLUSH);
        tcsetattr(fd,TCSANOW,&newtio);
        return;
    }
    
    int available()
    {
        int bytes;
        ioctl(fd, FIONREAD, &bytes);

        return bytes > 0;                
    }
    
    byte read()
    {
        ::read(fd,readBuffer,1);
        return readBuffer[0];
    }
    
    void write(byte data)
    {
        writeBuffer[0] = data;
        ::write(fd,writeBuffer,1);
    }
    
    void flush()
    {
        tcflush(fd, TCIFLUSH);
        return;
    }
    
    void print(int input)
    {
        return;
    }
    
    
};


// For Serial1, Serial2 and Valve control 
// No need for implementation for now 
class SilentSerial
{
public:
    
    void begin(int rate)
    {
        return;
    }
    
    int available()
    {
        return 0;
    }
    
    int read()
    {
        return 0x00;
    }
    
    void write(byte data)
    {
        return;
    }
    
};

class Valve
{
public:
    
    void begin()
    {
        return;
    }
    
    void transfer(byte value)
    {
        return;
    }
    
};
