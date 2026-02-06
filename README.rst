Overview
********

This application specifically looks for heart-rate monitors and reports the
heart-rate readings once connected.  This has been combined with MIDI1.0 
serial USART code that transmits a MIDI clock in the same tempo.  

In effect this is a human MIDI1.0 BPM clock. 

Requirements
************

* BlueZ running on the host, or
* A board with Bluetooth LE support
* Counter support 

Building and Running
********************
west build -b frdm_rw612 --shield lcd_par_s035_spi


