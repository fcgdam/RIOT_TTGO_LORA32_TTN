# RIOT TTGO ESP32 SX127X test program

This code is a sample program to test the feasibility of running RIOT-OS on a TTGO ESP32 Lora board that also has a SSD1306 screen.
It is also a test bed to test multiple threads running, namelly a thread for the OLed display and other for the TTN communication.
No efforts so far where done for energy conservation...

# Requirements

To run this code the following is needed:

  - Riot-OS repository
  - Riot-OS Build docker image
  - ESP-IDF github repository, to flash the firmware, since it is not possible to do it from the docker image
  - The supporting board definitions for the TTGO ESP32 Lora V1 board

# RIOT-OS TTGO ESP32 Lora board support

At the current time of writing this README, out-of-the-box, RIOT-OS doesn't have a board definition for the V1 TTGO ESP32 Lora board.
As such the board definition available at https://github.com/fcgdam/RIOT_TTGOESP32_LORA_V1 must be copied first to the boards directory of your RIOT installation.

After that change the Makefile for your activation type, either OTTA or ABP.

In case of ABP, from the TTN console, copy the Device id, application and network session key to the Makefile, and make sure that are no trailing spaces, otheriwse stange compile errors appear.

Then, just execute:

make flash term

and enjoy.
