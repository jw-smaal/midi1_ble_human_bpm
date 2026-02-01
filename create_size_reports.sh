#!/bin/bash
west build -t ram_report > ram_report.txt
west build -t rom_report > rom_report.txt
#west build -t rom_plot
