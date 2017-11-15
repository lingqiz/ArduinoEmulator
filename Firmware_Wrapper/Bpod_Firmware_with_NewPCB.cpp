/*{
----------------------------------------------------------------------------

This file is part of the Bpod Project
Copyright (C) 2014 Joshua I. Sanders, Cold Spring Harbor Laboratory, NY, USA
Heavily Modified by

Jeffrey Erlich
Yidi Chen

----------------------------------------------------------------------------

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, version 3.

This program is distributed  WITHOUT ANY WARRANTY and without even the
implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

// Bpod Finite State Machine v 0.5




// ===================
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
Valve SPI{};

#define bitRead(value, bit) (((value) >> (bit)) & 0x01)
#define lowByte(w) ((uint8_t) ((w) & 0xff))
#define highByte(w) ((uint8_t) ((w) >> 8))
// ===================

//////////////////////////////////////////////////////////////////////////////////////////////
const int OutputPokeNum = 9;
const int InputPokeNum = 9;  //merge this with OutputPokeNum once the 9th input is sorted
const int WireChannelNum = 4;
const int BNCChannelNum = 2;
const int InputEventNum = 52;
const int InputEventNumWithoutTC = 42;  // PCB03: TC stands for timer and counter~
const int OutputActionNum = 27;
const int UnusedEventIndex=41;  //for PCB03, was 39 before
//////////////////////////////////////////////////////////////////////////////////////////////
const byte FirmwareMajorVersion = 1;
const byte FirmwareMinorVersion = 2;
const byte FirmwareFixVersion = 0;

//////////////////////////////
// Hardware mapping:         /
//////////////////////////////

//     8 Sensor/Valve/LED Ports
//byte PortDigitalInputLines[8] = {28, 30, 32, 34, 36, 38, 40, 42};
const byte PortDigitalOutputLines[OutputPokeNum] = {28, 30, 32, 34, 36, 38, 40, 42, 44};
const byte PortPWMOutputLines[OutputPokeNum] = {9, 8, 7, 6, 5, 4, 3, 2, 45};  //PCB01:1 more PWM control added for valve
const byte PortAnalogInputLines[InputPokeNum] = {A0, A1, A2, A3, A4, A5, A6, A7, A8};
//     Dsub
const byte WireDigitalInputLines[WireChannelNum] = {35, 33, 31, 29};
const byte WireDigitalOutputLines[WireChannelNum] = {43, 41, 39, 37};

//     SPI device latch pins
const byte ValveRegisterLatch = 22;
const byte SyncRegisterLatch = 23;

//      Bnc
const byte BncOutputLines[BNCChannelNum] = {25, 24};
const byte BncInputLines[BNCChannelNum] = {11, 10};

//      Indicator
const byte RedLEDPin = 13;
const byte GreenLEDPin = 14;
const byte BlueLEDPin = 12;


//////////////////////////////////
// Initialize system state vars: /
//////////////////////////////////
//byte PortPWMOutputState[8] = {0};
byte PortPWMOutputState[OutputPokeNum] = {0};  // State of all 9 output lines (As 8-bit PWM value from 0-255 representing duty cycle. PWM cycles at 1KHZ) PCB01
byte PortDigitalOutputState[OutputPokeNum] = {0};  // State of all 9 output lines (for UV control) PCB02  //PCB05 CHAGE {0} TO LOW
byte PortValveOutputState = 0;    // State of all 8 valves
byte PortInputsEnabled[InputPokeNum] = {0};  // Enabled or disabled input reads of port IR lines
byte WireInputsEnabled[WireChannelNum] = {0};  // Enabled or disabled input reads of wire lines
byte DigitalInputsEnabled[8] = {1};
boolean PortInputLineValue[InputPokeNum] = {0};  // Direct reads of digital values of IR beams
boolean PortInputLineLastKnownStatus[InputPokeNum] = {0};  // Last known status of IR beams
int PortLowThreshold[InputPokeNum] = {0};  // default values are set in setup()
int PortHighThreshold[InputPokeNum] = {0};  // default values are set in setup()
boolean DigitalInputLineValue[8] = {0};  // Direct reads of digital values of IR beams
boolean DigitalInputLineLastKnownStatus[8] = {0};  // Last known status of IR beams
boolean BNCInputLineValue[BNCChannelNum] = {0};  // Direct reads of BNC input lines
boolean BNCInputLineLastKnownStatus[BNCChannelNum] = {0};  // Last known status of BNC input lines
boolean WireInputLineValue[WireChannelNum] = {0};  // Direct reads of Wire terminal input lines
boolean WireInputLineLastKnownStatus[WireChannelNum] = {0};  // Last known status of Wire terminal input lines
boolean MatrixFinished = false;  // Has the system exited the matrix (final state)?
boolean MatrixAborted = false;  // Has the user aborted the matrix before the final state?
boolean MeaningfulStateTimer = false;  // Does this state's timer get us to another state when it expires?
int CurrentState = 1;  // What state is the state machine currently in? (State 0 is the final state)
int NewState = 1;
byte OverrideFlag = 0;  // If 1, ignores input channels for current cycle
byte CurrentEvent[10] = {0};  // What event code just happened and needs to be handled. Up to 10 can be acquired per 30us loop.
byte nCurrentEvents = 0;  // Index of current event
byte SoftEvent = 0;  // What soft event code just happened


//////////////////////////////////
// Initialize general use vars:  /
//////////////////////////////////

byte CommandByte = 0;   // Op code to specify handling of an incoming USB serial message
byte VirtualEventTarget = 0;  // Op code to specify which virtual event type (Port, BNC, etc)
byte VirtualEventData = 0;  // State of target
byte BrokenBytes[4] = {0};  // Outgoing bytes (used in BreakLong function)
int nWaves = 0;  // number of scheduled waves registered
byte CurrentWave = 0;  // Scheduled wave currently in use
byte LowByte = 0;  // LowByte through FourthByte are used for reading bytes that will be combined to 16 and 32 bit integers
byte SecondByte = 0; byte ThirdByte = 0; byte FourthByte = 0;
unsigned long LongInt = 0;
int nStates = 0;
int nTotalEvents = 0;
int nEvents = 0;
int LEDBrightnessAdjustInterval = 5;
byte LEDBrightnessAdjustDirection = 1;
byte LEDBrightness = 0;
byte InputStateMatrix[128][InputEventNum] = {0};  // Matrix containing all of Bpod's inputs and corresponding state transitions PCB03:FROM 50 TO 52;
// Cols: 1-16 = IR beam in...out... 17-20 = BNC1 high...low 21-28 = Dsub1high...low 29=Tup 30=Unused 31-40=SoftEvents  PCB03 :Now every number add 2~~

//byte OutputStateMatrix[128][17] = {0};  // Matrix containing all of Bpod's output actions for each Input state


byte OutputStateMatrix[128][OutputActionNum] = {0};  // Cols: 1=Valves 2=BNC 3=Wire 4=Serial1 Op Code 5=Serial2 Op Code 6=StateIndependentTimerTrig 7=StateIndependentTimerCancel 8-15=PWM values (LED)        PCBO1: Expending the matrix to 18PCB02~~!!!!!!!!!!27

byte GlobalTimerMatrix[128][5] = {0};  // Matrix contatining state transitions for global timer elapse events
byte GlobalCounterMatrix[128][5] = {0};  // Matrix contatining state transitions for global timer elapse events
boolean GlobalTimersActive[5] = {0};  // 0 if timer x is inactive, 1 if it's active.
unsigned long GlobalTimerEnd[5] = {0};  // Future Times when active global timers will elapse
unsigned long GlobalTimers[5] = {0};  // Timers independent of states
unsigned long GlobalCounterCounts[5] = {0};  // Event counters
byte GlobalCounterAttachedEvents[5] = {254};  // Event each event counter is attached to
unsigned long GlobalCounterThresholds[5] = {0};  // Event counter thresholds (trigger events if crossed)
unsigned long TimeStamps[10000] = {0};  // TimeStamps for events on this trial
int MaxTimestamps = 10000;  // Maximum number of timestamps (to check when to start event-dropping)
int CurrentColumn = 0;  // Used when re-mapping event codes to columns of global timer and counter matrices
unsigned long StateTimers[128] = {0};  // Timers for each state
unsigned long StartTime = 0;  // System Start Time
unsigned long MatrixStartTime = 0;  // Trial Start Time
unsigned long MatrixStartTimeMillis = 0;  // Used for 32-bit timer wrap-over correction in client
unsigned long StateStartTime = 0;  // Session Start Time
unsigned long NextLEDBrightnessAdjustTime = 0;
byte ConnectedToClient = 0;
unsigned long CurrentTime = 0;
unsigned long TimeFromStart = 0;
unsigned long Num2Break = 0;  // For conversion from int32 to bytes
unsigned long SessionStartTime = 0;


unsigned long ReadLong() {
  while (SerialUSB.available() == 0) {}
  LowByte = SerialUSB.read();
  while (SerialUSB.available() == 0) {}
  SecondByte = SerialUSB.read();
  while (SerialUSB.available() == 0) {}
  ThirdByte = SerialUSB.read();
  while (SerialUSB.available() == 0) {}
  FourthByte = SerialUSB.read();
  LongInt =  (unsigned long)(((unsigned long)FourthByte << 24) | ((unsigned long)ThirdByte << 16) | ((unsigned long)SecondByte << 8) | ((unsigned long)LowByte));
  return LongInt;
}

void breakLong(unsigned long LongInt2Break) {
   //BrokenBytes is a global array for the output of long int break operations
  BrokenBytes[3] = (byte)(LongInt2Break >> 24);
  BrokenBytes[2] = (byte)(LongInt2Break >> 16);
  BrokenBytes[1] = (byte)(LongInt2Break >> 8);
  BrokenBytes[0] = (byte)LongInt2Break;
}

void SetBNCOutputLines(int BNCState) {
  switch(BNCState) {
        case 0: {digitalWrite(BncOutputLines[0], LOW); digitalWrite(BncOutputLines[1], LOW);} break;
        case 1: {digitalWrite(BncOutputLines[0], HIGH); digitalWrite(BncOutputLines[1], LOW);} break;
        case 2: {digitalWrite(BncOutputLines[0], LOW); digitalWrite(BncOutputLines[1], HIGH);} break;
        case 3: {digitalWrite(BncOutputLines[0], HIGH); digitalWrite(BncOutputLines[1], HIGH);} break;
  }
}
void ValveRegWrite(int value) {
  valveWrite(value);
   // Write to water chip
  SPI.transfer(value);
  digitalWrite(ValveRegisterLatch,HIGH);
  digitalWrite(ValveRegisterLatch,LOW);
}

void SyncRegWrite(int value) {
   // Write to LED driver chip
  SPI.transfer(value);
  digitalWrite(SyncRegisterLatch,HIGH);
  digitalWrite(SyncRegisterLatch,LOW);
}

void UpdatePWMOutputStates() {

  for (int x = 0; x < OutputPokeNum-1; x++) {  //For PCB01

      analogWrite(PortPWMOutputLines[x], PortPWMOutputState[x]);
  }
   // The last "PWM" port is actually digital :(
  digitalWrite(PortPWMOutputLines[OutputPokeNum-1], PortPWMOutputState[OutputPokeNum-1]);

}  //pcb05

void UpdateDigitalOutputStates() {
  for (int x = 0; x < OutputPokeNum; x++) {
    digitalWrite(PortDigitalOutputLines[x], PortDigitalOutputState[x]);
  }
}

void SetWireOutputLines(int WireState) {
  for (int x = 0; x < WireChannelNum; x++) {
    digitalWrite(WireDigitalOutputLines[x],bitRead(WireState,x));
  }
}

void updateStatusLED(int Mode) {
  CurrentTime = millis();
  switch (Mode) {
    case 0: {
      analogWrite(RedLEDPin, 0);
      digitalWrite(GreenLEDPin, 0);
      analogWrite(BlueLEDPin, 0);
    } break;
    case 1: {  // Waiting for matrix
    if (ConnectedToClient == 0) {
        if (CurrentTime > NextLEDBrightnessAdjustTime) {
          NextLEDBrightnessAdjustTime = CurrentTime + LEDBrightnessAdjustInterval;
          if (LEDBrightnessAdjustDirection == 1) {
            if (LEDBrightness < 255) {
              LEDBrightness = LEDBrightness + 1;
            } else {
              LEDBrightnessAdjustDirection = 0;
            }
          }
          if (LEDBrightnessAdjustDirection == 0) {
            if (LEDBrightness > 0) {
              LEDBrightness = LEDBrightness - 1;
            } else {
              LEDBrightnessAdjustDirection = 2;
            }
          }
          if (LEDBrightnessAdjustDirection == 2) {
            NextLEDBrightnessAdjustTime = CurrentTime + 500;
            LEDBrightnessAdjustDirection = 1;
          }
          analogWrite(BlueLEDPin, LEDBrightness);
        }
      } else {
        analogWrite(BlueLEDPin, 0);
        digitalWrite(GreenLEDPin, 1);
      }
    } break;
    case 2: {
      analogWrite(BlueLEDPin, 0);
      digitalWrite(GreenLEDPin, 1);
      analogWrite(RedLEDPin, 128);
    } break;
  }
}

void setStateOutputs(byte State) {
    byte CurrentTimer = 0;  // Used when referring to the timer currently being triggered
    byte CurrentCounter = 0;  // Used when referring to the counter currently being reset
    ValveRegWrite(OutputStateMatrix[State][0]);
    SetBNCOutputLines(OutputStateMatrix[State][1]);
    SetWireOutputLines(OutputStateMatrix[State][2]);
    Serial1.write(OutputStateMatrix[State][3]);
    Serial2.write(OutputStateMatrix[State][4]);
    if (OutputStateMatrix[State][5] > 0) {
      SerialUSB.write(2);  // Code for soft-code byte
      SerialUSB.write(OutputStateMatrix[State][5]);  // Code for soft-code byte
    }

    for (int x = 0; x < OutputPokeNum; x++) {
      PortPWMOutputState[x]=OutputStateMatrix[State][x+9];  //for PCB01
      PortDigitalOutputState[x]=OutputStateMatrix[State][x+18];  //PCB02: Changed to output line
    }
    UpdatePWMOutputStates();
    UpdateDigitalOutputStates();

/*   for (int x = 0; x < 18; x++)
      { int y=x;
        if (x<9) {
          if (x<8) {
            analogWrite(PortPWMOutputLines[x], OutputStateMatrix[State][x+9]);
          } else {
            digitalWrite(PortPWMOutputLines[x], OutputStateMatrix[State][x+9]);
          }
        }


       else
       {
        int z= (x-9);                // one of the trickiest thing to change to PCB02 (NOT SUPER SURE ABOUT IT!!!!!!!!!!!!!!!!)
      digitalWrite(PortDigitalOutputLines[z], OutputStateMatrix[State][x+9]);  //PCB02: Leaving this as pwm output~~~PCB05:OPTIMISING : FROM analogWrite to digitalWrite
    }
    } */


     // Trigger global timers
    CurrentTimer = OutputStateMatrix[State][6];
    if (CurrentTimer > 0){
      CurrentTimer = CurrentTimer - 1;  // Convert to 0 index
      GlobalTimersActive[CurrentTimer] = true;
      GlobalTimerEnd[CurrentTimer] = CurrentTime+GlobalTimers[CurrentTimer];
    }
     // Cancel global timers
    CurrentTimer = OutputStateMatrix[State][7];
    if (CurrentTimer > 0){
      CurrentTimer = CurrentTimer - 1;  // Convert to 0 index
      GlobalTimersActive[CurrentTimer] = false;
    }
     // Reset event counters
    CurrentCounter = OutputStateMatrix[State][8];
    if (CurrentCounter > 0){
      CurrentCounter = CurrentCounter - 1;  // Convert to 0 index
      GlobalCounterCounts[CurrentCounter] = 0;
    }
    if (InputStateMatrix[State][UnusedEventIndex] != State) {MeaningfulStateTimer = true;} else {MeaningfulStateTimer = false;}
    SyncRegWrite((State+1));  // Output binary state code, corrected for zero index
}

void manualOverrideOutputs() {
  byte OutputType = 0;
  while (SerialUSB.available() == 0) {}
  OutputType = SerialUSB.read();
  switch(OutputType) {
    case 'P':   // Override PWM lines
     /* for (int x = 0; x < 8; x++) {
        while (SerialUSB.available() == 0) {}
        PortPWMOutputState[x] = SerialUSB.read();
      }*/

      for (int x = 0; x < OutputPokeNum; x++) {   //for PCB01
        while (SerialUSB.available() == 0) {}
        PortPWMOutputState[x] = SerialUSB.read();
      }
      UpdatePWMOutputStates();

      for (int x = 0; x < OutputPokeNum; x++) {   //this loop is for the PCB02: Updating the digitalline
        while (SerialUSB.available() == 0) {}
        PortDigitalOutputState[x] = SerialUSB.read();
      }
      UpdateDigitalOutputStates();


      break;
    case 'V':   // Override valves
      while (SerialUSB.available() == 0) {}
      LowByte = SerialUSB.read();
      ValveRegWrite(LowByte);
      break;
    case 'B':  // Override BNC lines
      while (SerialUSB.available() == 0) {}
      LowByte = SerialUSB.read();
      SetBNCOutputLines(LowByte);
      break;
    case 'W':   // Override wire terminal output lines
      while (SerialUSB.available() == 0) {}
      LowByte = SerialUSB.read();
      SetWireOutputLines(LowByte);
      break;
    case 'S':  // Override serial module port 1
      while (SerialUSB.available() == 0) {}
      LowByte = SerialUSB.read();   // Read data to send
      Serial1.write(LowByte);
      break;
    case 'T':  // Override serial module port 2
      while (SerialUSB.available() == 0) {}
      LowByte = SerialUSB.read();   // Read data to send
      Serial2.write(LowByte);
      break;
    }
 }

boolean analoglogic(int x) {
   int val;
   val =  analogRead(PortAnalogInputLines[x]);
   if(PortInputLineLastKnownStatus[x] == HIGH) {
      if (val < PortHighThreshold[x])
        {return HIGH; }
      else
        {return LOW ;}
    } else {
      if (val < PortLowThreshold[x])
        {return HIGH; }
      else
        {return LOW ;}

    }

 }


void setup(char* serialAddr)
{
 for (int x = 0; x < InputPokeNum; x++)
 {
   pinMode(PortAnalogInputLines[x], INPUT_PULLUP);
   PortHighThreshold[x] = 860;
   PortLowThreshold[x] = 600;
 }

 for (int x = 0; x < OutputPokeNum; x++)
 {
   pinMode(PortPWMOutputLines[x], OUTPUT);  //for PCB01
   pinMode(PortDigitalOutputLines[x], OUTPUT);  //PCB02: Changed to output line setting it as OUTPUT
   if (x<OutputPokeNum-1){
      analogWrite(PortPWMOutputLines[x], 0);  //for PCB01
    } else {
      digitalWrite(PortPWMOutputLines[x], LOW);
    }
     // The last PWM pin is actually digital.
   digitalWrite(PortDigitalOutputLines[x], LOW);  //PCB02: Changed to output line
  }

 for (int x = 0; x < WireChannelNum; x++)
 {
   pinMode(WireDigitalInputLines[x], INPUT_PULLUP);
   pinMode(WireDigitalOutputLines[x], OUTPUT);
 }

 for (int x = 0; x < BNCChannelNum; x++)
 {
   pinMode(BncInputLines[x], INPUT);
   pinMode(BncOutputLines[x], OUTPUT);
 }

 pinMode(ValveRegisterLatch, OUTPUT);
 pinMode(SyncRegisterLatch, OUTPUT);
 pinMode(RedLEDPin, OUTPUT);
 pinMode(GreenLEDPin, OUTPUT);
 pinMode(BlueLEDPin, OUTPUT);
 SerialUSB.setSerialAddr(serialAddr);
 SerialUSB.begin(115200);
 Serial1.begin(115200);
 Serial2.begin(115200);
 SPI.begin();
 SetWireOutputLines(0);
 SetBNCOutputLines(0);
 updateStatusLED(0);
 ValveRegWrite(0);
}

void loop()
{
  while (SerialUSB.available() == 0) {   // Wait for MATLAB command, run LED breathing cycle
    updateStatusLED(1);
    udelay(100);
  }
  updateStatusLED(0);
  CommandByte = SerialUSB.read();   // P for Program, R for Run, O for Override, 6 for Device ID
  switch (CommandByte) {
    case '6':   // Initialization handshake
      digitalWrite(25, HIGH);
      SerialUSB.print(5);
      delay(100);
      SerialUSB.flush();
      SessionStartTime = millis();
      digitalWrite(25, LOW);
      break;
    case 'C':
      {byte poke;  // a byte is fine as long as we never have more than 256 pokes.
       unsigned long n_samp;   // A long is 4 bytes, so we can ask for up to 2^32 samples.
       int val;  // analogRead returns an int (which is also 4 bytes)

      while (SerialUSB.available() == 0) {}
       poke = SerialUSB.read();  // which poke should we calibrate?

       n_samp = ReadLong();  // this reads 4 bytes and then converts to a long.

       if ((poke < 0) || (poke >= InputPokeNum))  // matlab passed a bad poke number
       {
         for (long sx = 0; sx < n_samp; sx++){
          val = 0;
          breakLong(val);  // break up val into 4 bytes and send.
          SerialUSB.write(BrokenBytes[0]);
          SerialUSB.write(BrokenBytes[1]);
          SerialUSB.write(BrokenBytes[2]);
          SerialUSB.write(BrokenBytes[3]);
                 // wait a few ms to do the next analogRead.
          }
        break;
       }  // end if bad poke number

      for (long sx = 0; sx < n_samp; sx++){
          val = analogRead(PortAnalogInputLines[poke]);
          breakLong(val);  // break up val into 4 bytes and send.
          SerialUSB.write(BrokenBytes[0]);
          SerialUSB.write(BrokenBytes[1]);
          SerialUSB.write(BrokenBytes[2]);
          SerialUSB.write(BrokenBytes[3]);
          delay(3);        // wait a few ms to do the next analogRead.
        }  //end for
      }  // end case 'C'
      break;
    case 'D':
    { int val;
      byte poke;  // a byte is fine as long as we never have more than 256 pokes.

      while (SerialUSB.available() == 0) {}
       poke = SerialUSB.read();  // which poke should we calibrate?

        if ((poke < 0) || (poke >= InputPokeNum))
        {
          val = ReadLong();
          val = ReadLong();
           // just discard the input.
        }
        else
        {
          PortLowThreshold[poke] = ReadLong();
          PortHighThreshold[poke] = ReadLong();
        }
    }
    break;
    case 'F':   // Return firmware build number
      SerialUSB.write(FirmwareMajorVersion);
      SerialUSB.write(FirmwareMinorVersion);
      SerialUSB.write(FirmwareFixVersion);
      ConnectedToClient = 1;
      break;
    case 'O':   // Override hardware state
      manualOverrideOutputs();
      break;

    case 'Z':   // Bpod governing machine has closed the client program
      ConnectedToClient = 0;
      SerialUSB.write('1');
      break;
    case 'V':  // Virtual event. Since not in a state matrix, read bytes and ignore data.
        while (SerialUSB.available() == 0) {}
        VirtualEventTarget = SerialUSB.read();
        while (SerialUSB.available() == 0) {}
        VirtualEventData = SerialUSB.read();
      break;
    case 'S':  // Soft code. Since not in a state matrix, read bytes and ignore data.
        while (SerialUSB.available() == 0) {}
        VirtualEventTarget = SerialUSB.read();
        while (SerialUSB.available() == 0) {}
        VirtualEventData = SerialUSB.read();
      break;
    case 'H':  // Recieve byte from USB and send to serial module 1 or 2
        while (SerialUSB.available() == 0) {}
        LowByte = SerialUSB.read();
        while (SerialUSB.available() == 0) {}
        SecondByte = SerialUSB.read();
        switch (LowByte) {
          case 1:  // Send to serial port 1
            Serial1.write(SecondByte);
          break;
          case 2:  // Send to serial port 2
            Serial2.write(SecondByte);
          break;
        }
    break;

   //----------ADD COMMANDS FOR PLUGINS HERE--------------
   // i.e. send ethernet ip, soft-abort sound,etc

   //-----------------------------------------------------

    case 'P':   // Get new state matrix from client
      while (SerialUSB.available() == 0) {}
      nStates = SerialUSB.read();
       // Get Input state matrix
      for (int x = 0; x < nStates; x++) {
        for (int y = 0; y < InputEventNumWithoutTC; y++) {
          while (SerialUSB.available() == 0) {}
          InputStateMatrix[x][y] = SerialUSB.read();
        }
      }
       // Get Output state matrix

         for (int x = 0; x < nStates; x++) {
        for (int y = 0; y < OutputActionNum; y++) {
          while (SerialUSB.available() == 0) {}
          OutputStateMatrix[x][y] = SerialUSB.read();
        }
      }


       // Get global timer matrix
      for (int x = 0; x < nStates; x++) {
        for (int y = 0; y < 5; y++) {
          while (SerialUSB.available() == 0) {}
          GlobalTimerMatrix[x][y] = SerialUSB.read();
        }
      }
       // Get global counter matrix
      for (int x = 0; x < nStates; x++) {
        for (int y = 0; y < 5; y++) {
          while (SerialUSB.available() == 0) {}
          GlobalCounterMatrix[x][y] = SerialUSB.read();
        }
      }
       // Get global counter attached events
      for (int x = 0; x < 5; x++) {
          while (SerialUSB.available() == 0) {}
          GlobalCounterAttachedEvents[x] = SerialUSB.read();
      }
       // Get input channel configuration
      for (int x = 0; x < InputPokeNum; x++) {
          while (SerialUSB.available() == 0) {}
          PortInputsEnabled[x] = SerialUSB.read();
      }
      for (int x = 0; x < WireChannelNum; x++) {
          while (SerialUSB.available() == 0) {}
          WireInputsEnabled[x] = SerialUSB.read();
      }

       // Get state timers
      for (int x = 0; x < nStates; x++) {
              StateTimers[x] = ReadLong();
      }
       // Get global timers
      for (int x = 0; x < 5; x++) {
              GlobalTimers[x] = ReadLong();
      }
       // Get global counter event count thresholds
      for (int x = 0; x < 5; x++) {
          GlobalCounterThresholds[x] = ReadLong();
      }
      SerialUSB.write(1);
      break;   // end case P
    case 'R':   // Run State Matrix
      updateStatusLED(2);
      NewState = 0;
      CurrentState = 0;
      nTotalEvents = 0;
      nEvents = 0;
      MatrixFinished = false;
       // Reset event counters
      for (int x = 0; x < 5; x++) {
        GlobalCounterCounts[x] = 0;
      }
       // Read initial state of sensors
      for (int x = 0; x < InputPokeNum; x++) {
        if (PortInputsEnabled[x] == 1) {
          PortInputLineValue[x] = analoglogic(x);  // Read each photogate's current state into an array
          if (PortInputLineValue[x] == HIGH) {PortInputLineLastKnownStatus[x] = HIGH;} else {PortInputLineLastKnownStatus[x] = LOW;}  // Update last known state of input line
        } else {
          PortInputLineLastKnownStatus[x] = LOW; PortInputLineValue[x] = LOW;
        }
      }
      for (int x = 0; x < BNCChannelNum; x++) {
        BNCInputLineValue[x] = digitalRead(BncInputLines[x]);
        if (BNCInputLineValue[x] == HIGH) {BNCInputLineLastKnownStatus[x] = true;} else {BNCInputLineLastKnownStatus[x] = false;}
      }
      for (int x = 0; x < WireChannelNum; x++) {
        if (WireInputsEnabled[x] == 1) {
          WireInputLineValue[x] = digitalRead(WireDigitalInputLines[x]);
          if (WireInputLineValue[x] == HIGH) {WireInputLineLastKnownStatus[x] = true;} else {WireInputLineLastKnownStatus[x] = false;}
        }
      }
       // Set meaningful state timer variable (false if state timer is not used, so that a Tup event isn't generated)
      if (InputStateMatrix[CurrentState][38] != CurrentState) {
        MeaningfulStateTimer = true;
      } else {
        MeaningfulStateTimer = false;
      }

       // Reset timers
      MatrixStartTime = micros();
      StateStartTime = MatrixStartTime;
      CurrentTime = MatrixStartTime;
      MatrixStartTimeMillis = millis();

       // Adjust outputs, scheduled waves, serial codes and sync port for first state
      setStateOutputs(CurrentState);

      while (MatrixFinished == false) {
        OverrideFlag = false;
        nCurrentEvents = 0;
        CurrentEvent[0] = 254;  // Event 254 = No event
        SoftEvent = 254;
         // Check for manual override
        if (SerialUSB.available() > 0) {
          CommandByte = SerialUSB.read();
          switch (CommandByte) {
            case 'O':   // Override outputs
              manualOverrideOutputs();
              OverrideFlag = true;  // Skips this loop iteration's input state refresh to ensure intended effect and avoid negation by a state change
              break;
            case 'V':  // Manual override: execute virtual event
              OverrideFlag = true;   // Skips this loop iteration's input state refresh to ensure intended effect and avoid negation by a state change
              while (SerialUSB.available() == 0) {}
              VirtualEventTarget = SerialUSB.read();
              while (SerialUSB.available() == 0) {}
              VirtualEventData = SerialUSB.read();
              switch (VirtualEventTarget) {
                case 'P':  // Virtual poke PortInputLineLastKnownStatus
                  if (PortInputLineLastKnownStatus[VirtualEventData] == LOW) {
                    PortInputLineValue[VirtualEventData] = HIGH;
                  } else {
                    PortInputLineValue[VirtualEventData] = LOW;
                  }
                  break;
                case 'B':  // Virtual BNC input
                  if (BNCInputLineLastKnownStatus[VirtualEventData] == LOW) {
                    BNCInputLineValue[VirtualEventData] = HIGH;
                  } else {
                    BNCInputLineValue[VirtualEventData] = LOW;
                  }
                  break;
                case 'W':  // Virtual Wire input
                  if (WireInputLineLastKnownStatus[VirtualEventData] == LOW) {
                      WireInputLineValue[VirtualEventData] = HIGH;
                    } else {
                      WireInputLineValue[VirtualEventData] = LOW;
                    }
                break;
                case 'S':   // Soft event
                      SoftEvent = VirtualEventData;
                break;
              }  // end switch VirtualEventTarget
              break;  // end case V
            case 'X':    // Exit state matrix and return data
              MatrixFinished = true;
              setStateOutputs(0);  // Returns all lines to low by forcing final state




              break;  // end case X
            }
         }

          // BpodController didn't send any specific action so just run?
         CurrentTime = micros();




         if (OverrideFlag == false) {
            // Refresh state of sensors and inputs
           for (int x = 0; x < InputPokeNum; x++) {
             if (PortInputsEnabled[x] == 1) {
              PortInputLineValue[x] = analoglogic(x);
             }
          }
         /*
          for (int x = 0; x < 8; x++) {
             if (DigitalInputsEnabled[x] == 1) {
              DigitalInputLineValue[x] = digitalRead(PortDigitalInputLines[x]);
             }
          }*/   //PCB02: deleted this out since it's not input anymore
          for (int x = 0; x < BNCChannelNum; x++) {
            BNCInputLineValue[x] = digitalRead(BncInputLines[x]);
          }
          for (int x = 0; x < WireChannelNum; x++) {
            if (WireInputsEnabled[x] == 1) {
              WireInputLineValue[x] = digitalRead(WireDigitalInputLines[x]);
            }
          }
         }
          // Determine which port event occurred
         int Ev = 0;  // Since port-in and port-out events are indexed sequentially, Ev replaces x in the loop.
         for (int x = 0; x < InputPokeNum; x++) {
                // Determine port entry events
               if ((PortInputLineValue[x] == HIGH) && (PortInputLineLastKnownStatus[x] == LOW)) {
                  PortInputLineLastKnownStatus[x] = HIGH; CurrentEvent[nCurrentEvents] = Ev; nCurrentEvents++;
               }
               Ev = Ev + 1;
                // Determine port exit events
               if ((PortInputLineValue[x] == LOW) && (PortInputLineLastKnownStatus[x] == HIGH)) {
                  PortInputLineLastKnownStatus[x] = LOW; CurrentEvent[nCurrentEvents] = Ev; nCurrentEvents++;
               }
               Ev = Ev + 1;
         }
          // Determine which BNC event occurred
         for (int x = 0; x < BNCChannelNum; x++) {
            // Determine BNC low-to-high events
           if ((BNCInputLineValue[x] == HIGH) && (BNCInputLineLastKnownStatus[x] == LOW)) {
              BNCInputLineLastKnownStatus[x] = HIGH; CurrentEvent[nCurrentEvents] = Ev; nCurrentEvents++;
           }
           Ev = Ev + 1;
            // Determine BNC high-to-low events
           if ((BNCInputLineValue[x] == LOW) && (BNCInputLineLastKnownStatus[x] == HIGH)) {
              BNCInputLineLastKnownStatus[x] = LOW; CurrentEvent[nCurrentEvents] = Ev; nCurrentEvents++;
           }
           Ev = Ev + 1;
         }
          // Determine which Wire event occurred
         for (int x = 0; x < WireChannelNum; x++) {
            // Determine Wire low-to-high events
             if ((WireInputLineValue[x] == HIGH) && (WireInputLineLastKnownStatus[x] == LOW)) {
                WireInputLineLastKnownStatus[x] = HIGH; CurrentEvent[nCurrentEvents] = Ev; nCurrentEvents++;
             }
             Ev = Ev + 1;
              // Determine Wire high-to-low events
             if ((WireInputLineValue[x] == LOW) && (WireInputLineLastKnownStatus[x] == HIGH)) {
                WireInputLineLastKnownStatus[x] = LOW; CurrentEvent[nCurrentEvents] = Ev; nCurrentEvents++;
             }
             Ev = Ev + 1;
         }
           // Map soft events to event code scheme
          if (SoftEvent < 254) {
            CurrentEvent[nCurrentEvents] = SoftEvent + Ev; nCurrentEvents++;
          }

           for (int x = 0; x < 8; x++) {
                // Determine port entry events
               if ((DigitalInputLineValue[x] == HIGH) && (DigitalInputLineLastKnownStatus[x] == LOW)) {
                  DigitalInputLineLastKnownStatus[x] = HIGH; CurrentEvent[nCurrentEvents] = Ev; nCurrentEvents++;
               }
               Ev = Ev + 1;
                // Determine Digital exit events
               if ((DigitalInputLineValue[x] == LOW) && (DigitalInputLineLastKnownStatus[x] == HIGH)) {
                  DigitalInputLineLastKnownStatus[x] = LOW; CurrentEvent[nCurrentEvents] = Ev; nCurrentEvents++;
               }
               Ev = Ev + 1;
         }

          Ev = InputEventNumWithoutTC;  //PCB03:FROM 40 TO 42
           // Determine if a global timer expired
          for (int x = 0; x < 5; x++) {
            if (GlobalTimersActive[x] == true) {
              if (CurrentTime > GlobalTimerEnd[x]) {
                CurrentEvent[nCurrentEvents] = Ev; nCurrentEvents++;
                GlobalTimersActive[x] = false;
              }
            }
            Ev = Ev + 1;
          }
           // Determine if a global event counter threshold was exceeded
          for (int x = 0; x < 5; x++) {
            if (GlobalCounterAttachedEvents[x] < 254) {
               // Check for and handle threshold crossing
              if (GlobalCounterCounts[x] == GlobalCounterThresholds[x]) {
                CurrentEvent[nCurrentEvents] = Ev; nCurrentEvents++;
              }
               // Add current event to count (Crossing triggered on next cycle)
              if (CurrentEvent[0] == GlobalCounterAttachedEvents[x]) {
                GlobalCounterCounts[x] = GlobalCounterCounts[x] + 1;
              }
            }
            Ev = Ev + 1;
          }
          Ev = UnusedEventIndex;  //PCB03 didnt define these, was 39 for Unused~~~~~
           // Determine if a state timer expired
          TimeFromStart = CurrentTime - StateStartTime;
          if ((TimeFromStart > StateTimers[CurrentState]) && (MeaningfulStateTimer == true)) {
            CurrentEvent[nCurrentEvents] = Ev; nCurrentEvents++;
          }

           // Now determine if a state transition should occur, based on the first CurrentEvent detected
          if (CurrentEvent[0] < InputEventNumWithoutTC) {  //PCB03 FROM 40 to InputEventNumWithoutTC
            NewState = InputStateMatrix[CurrentState][CurrentEvent[0]];
          } else if (CurrentEvent[0] < 47) {//PCB03 : FROM 45 TO 47
            CurrentColumn = CurrentEvent[0] - InputEventNumWithoutTC;  //PCB03: FROM 40 TO 42
            NewState = GlobalTimerMatrix[CurrentState][CurrentColumn];
          } else if (CurrentEvent[0] < InputEventNum) {//PCB03:FROM 50 TO 52
            CurrentColumn = CurrentEvent[0] - 47;  //PCB03:FROM 45 TO 47
            NewState = GlobalCounterMatrix[CurrentState][CurrentColumn];
          }
           // Store timestamp of events captured in this cycle
          if ((nTotalEvents + nCurrentEvents) < MaxTimestamps) {
            for (int x = 0; x < nCurrentEvents; x++) {
              TimeStamps[nEvents] = CurrentTime;
              nEvents++;
            }
          }
           // Make state transition if necessary
          if (NewState != CurrentState) {
             if (NewState == nStates) {
                MatrixFinished = true;


             } else {
                setStateOutputs(NewState);
                StateStartTime = CurrentTime;
                CurrentState = NewState;
             }
          }
           // Write events captured to USB (if events were captured)
          if (nCurrentEvents > 0) {
            SerialUSB.write(1);  // Code for returning events
            SerialUSB.write(nCurrentEvents);
            for (int x = 0; x < nCurrentEvents; x++) {
              SerialUSB.write(CurrentEvent[x]);
            }
          }

      }  // End while(MatrixFinished == false)
      SyncRegWrite(0);  // Reset the sync lines
      ValveRegWrite(0);  // Reset valves

      for (int x = 0; x < OutputPokeNum; x++) {
      // Reset PWM lines
      // reset digital lines
        PortPWMOutputState[x] = 0;
        PortDigitalOutputState[x] = 0;
        }
      UpdatePWMOutputStates();
      UpdateDigitalOutputStates();
      SetBNCOutputLines(0);  // Reset BNC outputs
      SetWireOutputLines(0);  // Reset wire outputs
      SerialUSB.write(1);  // Op Code for sending events
      SerialUSB.write(1);  // Read one event
      SerialUSB.write(255);  // Send Matrix-end code
       // Send trial-start timestamp (in milliseconds, basically immune to microsecond 32-bit timer wrap-over)
      Num2Break = MatrixStartTimeMillis-SessionStartTime;
      breakLong(Num2Break);
      SerialUSB.write(BrokenBytes[0]);
      SerialUSB.write(BrokenBytes[1]);
      SerialUSB.write(BrokenBytes[2]);
      SerialUSB.write(BrokenBytes[3]);
       // Send matrix start timestamp (in microseconds)
      breakLong(MatrixStartTime);
      SerialUSB.write(BrokenBytes[0]);
      SerialUSB.write(BrokenBytes[1]);
      SerialUSB.write(BrokenBytes[2]);
      SerialUSB.write(BrokenBytes[3]);
      if (nEvents > 9999) {nEvents = 10000;}
      SerialUSB.write(lowByte(nEvents));
      SerialUSB.write(highByte(nEvents));
      for (int x = 0; x < nEvents; x++) {
          Num2Break = TimeStamps[x];
          breakLong(Num2Break);
          SerialUSB.write(BrokenBytes[0]);
          SerialUSB.write(BrokenBytes[1]);
          SerialUSB.write(BrokenBytes[2]);
          SerialUSB.write(BrokenBytes[3]);
        }
         // Terminate all active scheduled waves
      for (int x = 0; x < 5; x++) {
        GlobalTimersActive[x] = 0;
        }
      updateStatusLED(0);
      for (int x = 0; x < 5; x++) {  // Shut down active global timers
        GlobalTimersActive[x] = false;
      }
      break;
  }  // End command byte switch/case
}  // End main loop

