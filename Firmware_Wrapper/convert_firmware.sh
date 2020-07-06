#!/bin/bash

# Backup current 
mv Bpod_Firmware_with_NewPCB.cpp Bpod_Firmware_with_NewPCB.`date +%y%m%d`

# Add the headers
sed '/#include <SPI.h>/r header.txt' ../../bpod/Bpod\ Firmware/Bpod_Firmware_with_NewPCB/Bpod_Firmware_with_NewPCB.ino > Bpod_Firmware_with_NewPCB.cpp

# Apple uses a dumb BSD version of sed
if [ `uname` = "Linux" ]
then
    BACKUPARGS="-ibak"
else
    BACKUPARGS="-i bak"
fi

# Remove SPI include
sed $BACKUPARGS 's/#include <SPI.h>//' Bpod_Firmware_with_NewPCB.cpp

# fix setup call
sed $BACKUPARGS 's/void setup()/void setup(char* serialAddr)/
	   ' Bpod_Firmware_with_NewPCB.cpp

sed $BACKUPARGS 's/int ValveRegWrite(/void ValveRegWrite(/
	   ' Bpod_Firmware_with_NewPCB.cpp

sed $BACKUPARGS 's/int SyncRegWrite(/void SyncRegWrite(/
	   ' Bpod_Firmware_with_NewPCB.cpp

sed $BACKUPARGS '/void ValveRegWrite(/a\
    \  valveWrite(value);
    ' Bpod_Firmware_with_NewPCB.cpp

sed $BACKUPARGS '/pinMode(BlueLEDPin, OUTPUT);/a\
    \ SerialUSB.setSerialAddr(serialAddr);
    ' Bpod_Firmware_with_NewPCB.cpp

sed $BACKUPARGS '/updateStatusLED(1)/ a\
    \ \ \ \ udelay(100);
    ' Bpod_Firmware_with_NewPCB.cpp


