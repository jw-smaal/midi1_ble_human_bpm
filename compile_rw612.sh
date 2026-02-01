#!/bin/bash 
rm -rf build/
west build -b frdm_rw612 --shield lcd_par_s035_spi
