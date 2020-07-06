Bpod Emulator v0.1
==================

Architecture
------
<img src="/Images/System_Architecture.png">

Notes
-----

This emulator was developed for an altered version of Bpod in the Erlich Lab. It has not yet been tested with the official Bpod firmware.

Install
-------
1. Clone this repository
3. Run install script  
`cd bpod-emulator; ./install_emulator_mac` (for Mac)  
`cd bpod-emulator; ./install_emulator_linux` (for Debian/Ubuntu)


Or install it manually:  
1. [socat:][1] Use `brew install socat` on Mac
2. [zeroMQ:][3] Use `brew install zmq` on Mac
3. [zeroMQ C++ binding:][4] Put `zmq.hpp` in search path  
    On Mac: copy `zmq.hpp` to `/usr/local/include`
4. [kivy:][5] Python GUI framework  
(Try `pip install cython` and `pip install kivy` first)  

Usage
-------
Try this first:  
1. `./start_emulator [path_for_pty] [port_number_for_matlab]`  
(e.g., `./start_emulator ~/Desktop/emulator 9000`)  
(might need to run `chmod 744 ./start_emulator` for the first time) 
2. Inside matlab: `start_bpod localhost 9000`


Or start it manually: 
1. Start a socat relay process (in a different terminal):  
`socat -d -d pty,echo=0,raw TCP-LISTEN:9000,fork`  
(See [TCP over socat][2] if you want to understand what we are doing)  
4. It will return something like this:  
`socat[36146] N PTY is /dev/ttys001`  
`socat[36146] N listening on LEN=16 AF=2 0.0.0.0:9000`  
5. `./bpod_em /dev/ttys001` (change the serial port path accordingly)
7. `python kivy_gui.py`  
6. Inside matlab: `start_bpod localhost 9000` (change port number accordingly)  

[1]: http://www.dest-unreach.org/socat/
[2]: http://www.dest-unreach.org/socat/doc/socat-ttyovertcp.txt
[3]: http://zeromq.org/intro:get-the-software
[4]: https://github.com/zeromq/cppzmq
[5]: https://kivy.org/#download


Notes for updating the emaultor firmware when changes are made
-------

If you have installed bpod in a sister directory to this repository then:

1. `cd FirmwareWrapper/`
2. `./convert_firmware.sh`


The `covert_firmware.sh` file will look for a specific firmware (used in the ErlichLab) and copy it and make the following changes:
1. Add [headers][6]
2. `setup()` takes an [extra input][7]
3. Add [this line][8] to `setup()`
4. `setup()` and `loop()` need to be moved to the end of the code
5. Add [delay][9] to prevent emulator taking up all CPU time 

If you are using this tool with Sanworks Bpod you will need to change one line of the script to point to the correct firmware.

FAQ
---
**Q: When i try to restart the emulator it doesn't start / i get errors. How do I fix this?**  
A: If Bpod is still running, you need to shut it down first. 
Just like with real hardware, you cannot "unplug" and "re-plug" the emulator and
expect everything to work. In general, you should close Bpod first and then close the emulator.

**Q: Sometimes when I click a poke, it registers as two pokes.**  
A: When you move _focus_ to another window, the events when you bring focus back
to the emulator window are not handled well. It is recommended to click in the 
background (not on a poke) to get focus back to the emulator before poking.

**Q: I can't successfully start emulator even if I successfully install it on mac**<br>
A: Change `g++ -std=c++11 -pthread -lzmq ./Firmware_Wrapper/main.cpp -o ./Firmware_Wrapper/bpod_em`

to `g++ -std=c++11 -I /usr/local/include -L /usr/local/lib -pthread -lzmq ./Firmware_Wrapper/main.cpp -o`

`./Firmware_Wrapper/bpod_em` in `start_emulator`. Your need to explicitly specify your linker if it's not working.


[6]: https://gitlab.erlichlab.org/erlichlab/bpod-emulator/blob/gui/Firmware_Wrapper/Bpod_Firmware_with_NewPCB.cpp#L22-41
[7]: https://gitlab.erlichlab.org/erlichlab/bpod-emulator/blob/gui/Firmware_Wrapper/Bpod_Firmware_with_NewPCB.cpp#L506
[8]: https://gitlab.erlichlab.org/erlichlab/bpod-emulator/blob/gui/Firmware_Wrapper/Bpod_Firmware_with_NewPCB.cpp#L542
[9]: https://gitlab.erlichlab.org/erlichlab/bpod-emulator/blob/gui/Firmware_Wrapper/Bpod_Firmware_with_NewPCB.cpp#L556
