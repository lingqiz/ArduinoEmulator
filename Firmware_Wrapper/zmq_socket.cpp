#include <zmq.hpp>
#include <string>
#include <stdio.h>
#include <iostream>
#include <cstdlib>
#include <pthread.h>

#ifndef _WIN32
#include <unistd.h>
#else
#include <windows.h>
#endif


zmq::context_t context (1);
// Message of format %2d:%3d (idx:data)
const int outputMessageLength = 6;
int outputPort = 8002;

// Message of format %d:%3d (idx:data)
const int inputMessageLength  = 5;
int inputPort  = 8003;

/*
    ZMQ threads for sending information to the GUI side 
    Loop through shared memory and send one array entry each time
    Refresh rate of 30 MS 
*/
const unsigned long REFRESH_SPEED_MS = 30;
void *sender_thread_exec(void *threadid)
{
    zmq::socket_t  sender(context, ZMQ_PUSH);

    sender.bind("tcp://*:8002");  //outputPort
    zmq::message_t message(outputMessageLength);

    while (true)
    {
        for (int pinIdx = 0; pinIdx < digitalPinsNumber; pinIdx++)
        {
            message.rebuild(outputMessageLength);
            sprintf((char *) message.data(), "%2d:%3d", pinIdx, digitalPins[pinIdx]);
            sender.send(message);
        }
        
        message.rebuild(outputMessageLength);
        sprintf((char *) message.data(), "%2d:%3d", digitalPinsNumber, valveState);       
        sender.send(message);

        usleep(REFRESH_SPEED_MS * MILLSEC_TO_MICROSEC);
    }

    pthread_exit(NULL);
}
    
/*
    Wait on ZMQ socket, whenever updated value of analog pin recevied, 
    write it to shared memory.
*/
void *receiver_thread_exec(void* threadid)
{
     
    zmq::socket_t  receiver(context, ZMQ_PULL);

    receiver.connect("tcp://127.0.0.1:8003"); //inputPort
    zmq:: message_t message;

    while(true)
    {
        receiver.recv(&message);
        std::string msg(static_cast<char*>(message.data()), message.size());
            
        int idx = std::atoi(msg.substr(0, 1).c_str()); // Analog A0 - A8
        int val = std::atoi(msg.substr(2, 3).c_str()); // Poke Value 
            
        pinWrite(PortAnalogInputLines[idx], val);
    }

    pthread_exit(NULL);
}

  
void startSender()
{
    pthread_t sender_thread;
    int rc = pthread_create(&sender_thread, NULL, sender_thread_exec, NULL);
    if (rc)
    {         
        exit(-1);
    }
}

void startReceiver()
{
    pthread_t receiver_thread;
    int rc = pthread_create(&receiver_thread, NULL, receiver_thread_exec, NULL);
    if (rc)
    {         
        exit(-1);
    }
}