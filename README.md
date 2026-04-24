# ESP32 Tesla_BLE_TPMS Reader v0.3
ESP32 Tesla BLE TPMS v0.3 by Conny (c)2026
This tool is made by reverse engineering Tesla's M3 and MY TPMS sensors BLE payloads to be read with any esp32 microcontroller via Serial Monitor. Could really be used with any microcontroller and BT module with little to no modifications.

It can read both first-gen and highland m3 and my tpms. NO MS and MX (they're 433mhz).
Accuracy is still to be perfectioned, the device works pretty fine tho.
If the tire isn't inflated or is not moving with enough speed, the sensor will report very few to no data (try turning debug on).
If you wish to trigger them without having them pressured up and running, just make something that makes a 125kHz square wave near them.
Feedback from the device will be provided by serial monitor (baudrate 115200),
and several commands are available to play with it. Just type "help" in the serial monitor.
It also stores in the flash memory how many tpms have been read and the unique MACs to be able to recognise new devices from already known.
Dedicated command also to delete one, the other or both.

ESP32 SETUP:
ESP32 boards library version 3.3.8, if the device crashes on startup try compiling with Huge APP partition. (shouldn't be needed).
Works pretty fine in ESP32-D0WD-V3 (revision v3.1), ESP32U, should also work in ESP32-C3 (single core) and ESP32-S3 (tested partially).
On the device i tested it successfully, Sketch uses 1118865 bytes (85%) of program storage space. Maximum is 1310720 bytes.

Feel free to comment and report issues and feedback!
Enjoy!
