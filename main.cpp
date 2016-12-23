// Main function - Bpod emulator

typedef unsigned char BYTE;
#include <zmq.hpp>
#include <iostream>

#include "arduino_firmware.cpp"
// You can change this line to point to your altered arduino firmware

#include "zmq_socket.cpp"

#include <cstdlib>

int main(int argc, const char* argv[])
{

    startSender();
    startReceiver();

    setup((char*) argv[1]);
    while(true)
    {
        loop( );
    }
}
