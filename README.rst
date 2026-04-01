Overview
********

The **MIDI 1.0 Human Clock** is an automated bridge between heart rate sensors and 
MIDI hardware. It synchronizes musical tempo to human physiology in real-time.

**Operation:**
1. **Auto-Scan**: On boot, the app scans for any available BLE Heart Rate Monitor (HRS).
2. **Connect & Sync**: It automatically connects and subscribes to BPM notifications.
3. **Clock Generation**: Converts the heart rate into a high-precision 24 PPQN MIDI 
   Timing Clock.
4. **Dual Output**: Transmits the clock simultaneously via Serial MIDI 1.0 and 
   USB MIDI 2.0 (UMP).
5. **Monitor**: Tracks and displays incoming MIDI messages (Notes, CC, Pitchwheel) 
   and BPM stability (PLL) on the integrated LCD.

Features:
- **BLE**: Supports Coded PHY (Long Range) for robust sensor connectivity.
- **Precision**: Hardware-assisted MIDI clock generation and measurement.
- **UI**: LVGL-based dashboard with BPM history charts and a MIDI message log.

Requirements
************

- Zephyr board with BLE (e.g., `frdm_rw612`, `frdm_mcxw71`).
- LCD display (e.g., `lcd_par_s035_spi`).
- `Zephyr MIDI1 module <https://github.com/jw-smaal/zephyr-midi1>`_.

Building
********

.. code-block:: console

   west build -b frdm_rw612 --shield lcd_par_s035_spi

--
author Jan-Willem Smaal <usenet@gispen.org
date 20251220
license SPDX-License-Identifier: Apache-2.0
