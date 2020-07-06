// Main function - Bpod emulator

typedef unsigned char BYTE;
#include <zmq.hpp>
#include <iostream>
#include "Bpod_Firmware_with_NewPCB.cpp"
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
