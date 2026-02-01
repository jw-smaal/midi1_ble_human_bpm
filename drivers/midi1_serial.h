/**
 * @file midi1_serial.h
 * @brief Serial USART implementation of MIDI1.0 for Zephyr 
 *
 * @note 
 * MIDI1.0 implementation created in 2014 initially for ATMEL MCU's then
 * ported to ARM MBED targets in C++, ported back to C for Zephyr-RTOS in 2024.
 * Adjusted in 2025 to return UMP's.  Changed to to support
 * multiple instances in 2026.
 *
 * This version supports 'running_status' and is tested with
 * real MIDI instruments.
 *
 * TODO: Things it DOES NOT do yet.
 * TODO:  - Song position stuff MIDI1.0 spec page 27
 * TODO:  - Universal System exclusive
 * TODO:  - Tuning requests 
 *
 * @author Jan-Willem Smaal <usenet@gispen.org> 
 * updated 20241224
 * updated 20250103
 * updated 20260107
 *
 * license SPDX-License-Identifier: Apache-2.0
 */
#ifndef MIDI1_SERIAL_H
#define MIDI1_SERIAL_H

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <string.h>
#include <stdint.h>

/* MIDI1.0 definitions by Jan-Willem Smaal */
#include "midi1.h"

/*
 * This 'MSQ_SIZE' constant is something you will want to tune.
 * It's not good in Music to have deep buffers due to delay buildup (latency)
 * It's ok e.g. to drop some control changes on a mod wheel sweep.
 * Make this is as low as possible
 */
/* Kconfig has this tuneable value now for the MSGQ  */
#define MSGQ_SIZE CONFIG_MIDI1_SERIAL_ISR_MSGQ_SIZE_MS
#define MSG_SIZE sizeof(uint8_t)

/*-----------------------------------------------------------------------*/
struct midi1_serial_config {
	const struct device *uart;
};

struct midi1_serial_callbacks {
	/*
	 * Callback delegates
	 */
	void (*note_on)(uint8_t channel, uint8_t note, uint8_t velocity);
	void (*note_off)(uint8_t channel, uint8_t note, uint8_t velocity);
	void (*control_change)(uint8_t channel, uint8_t controller,
	                       uint8_t value);

	void (*pitchwheel)(uint8_t channel, uint8_t lsb, uint8_t msb);
	void (*program_change)(uint8_t channel, uint8_t number);
	void (*channel_aftertouch)(uint8_t channel, uint8_t pressure);
	void (*poly_aftertouch)(uint8_t channel, uint8_t note,
	                        uint8_t pressure);

	/* Callback for the realtime messages (clock, stop etc..) */
	void (*realtime)(uint8_t msg);

	/*
	 * Sysex callback data be aware this can be a lot of data e.g. sample
	 * dumps
	 */
	void (*sysex_start)(void);
	void (*sysex_data)(uint8_t data);
	void (*sysex_stop)(void);
};

struct midi1_serial_data {
	/* RX parser state */
	uint8_t running_status_rx;
	uint8_t third_byte_flag;
	uint8_t midi_c2;
	uint8_t midi_c3;

	/* TX running status */
	uint8_t running_status_tx;
	uint8_t running_status_tx_count;
	uint32_t last_status_tx_time;

	/* Message queue filled by the ISR routine */
	struct k_msgq msgq;
	uint8_t msgq_buffer[MSGQ_SIZE];

	/* Set when processing sysex data */
	bool in_sysex;

	/*
	 * Must be filled by the application after init.
	 */
	struct midi1_serial_callbacks cb;
};

/**
 * @note the zephyr API for our MIDI1.0 serial driver
 */
struct midi1_serial_api {
	/* -- == Receive  == --   */
	int (*register_callbacks)(const struct device * dev,
	                          struct midi1_serial_callbacks * cb);
	void (*receiveparser)(const struct device * dev);
	/* -- == Transmit == --   */

	/* Channel mode messages */
	void (*note_on)(const struct device * dev,
	                uint8_t channel, uint8_t key, uint8_t velocity);
	void (*note_off)(const struct device * dev,
	                 uint8_t channel, uint8_t key, uint8_t velocity);
	void (*control_change)(const struct device * dev,
	                       uint8_t channel,
	                       uint8_t controller, uint8_t val);
	void (*channelaftertouch)(const struct device * dev,
	                          uint8_t channel, uint8_t val);
	void (*modwheel)(const struct device * dev,
	                 uint8_t channel, uint16_t val);
	void (*pitchwheel)(const struct device * dev,
	                   uint8_t channel, uint16_t val);
	/* System common */
	void (*timingclock)(const struct device * dev);
	void (*start)(const struct device * dev);
	/* NOTE: Is actually 'continue' but that a reserved word in C :-) */
	void (*continu)(const struct device * dev);
	void (*stop)(const struct device * dev);
	void (*active_sensing)(const struct device * dev);
	void (*reset)(const struct device * dev);
	/* System exclusive */
	void (*sysex_start)(const struct device * dev);
	void (*sysex_char)(const struct device * dev, uint8_t c);
	void (*sysex_data_bulk)(const struct device * dev,
	                        const uint8_t * data, uint32_t len);
	void (*sysex_stop)(const struct device * dev);
};

/*-----------------------------------------------------------------------*/

/**
 * @brief inits the MIDI serial subsystem.  Is called by the kernel
 * after booting automatically.
 *
 * @param *dev MIDI1.0 device serial instance.
 */
int midi1_serial_init(const struct device *dev);

/**
 * @brief After init done by the kernel this should be called
 * if you want to assign callbacks to input messages received
 *
 * @param *dev MIDI1.0 device serial instance.
 * @param *cb callback struct 
 */
int midi1_serial_register_callbacks(const struct device *dev,
                                    struct midi1_serial_callbacks *cb);

/**
 * @brief this needs to be called in a loop to process the received MIDI1
 *
 * @note advice to run this in a seperate thread.   It's a blocking function so
 * @note no delay is required between calls.
 * @note This process then calls the delegate functions / callbacks like
 * void (*note_on)(uint8_t, unit8_t)
 * @param *dev MIDI1.0 device serial instance.
 */
void midi1_serial_receiveparser(const struct device *dev);

/* Channel mode messages */
/* ___ _                       _   __  __         _
  / __| |_  __ _ _ _  _ _  ___| | |  \/  |___  __| |___
 | (__| ' \/ _` | ' \| ' \/ -_) | | |\/| / _ \/ _` / -_)
  \___|_||_\__,_|_||_|_||_\___|_| |_|  |_\___/\__,_\___|
 */

/**
 * @brief send a NOTE ON tx event via the instance inst
 * @param *dev MIDI1.0 device serial instance.
 * @param channel MIDI channel 0 == CH1 (Macro's exist for this e.g. CH16)
 * @param key MIDI key number
 * @param velocity NOTE on velocity
 */
void midi1_serial_note_on(const struct device *dev,
                          uint8_t channel, uint8_t key, uint8_t velocity);

/**
 * @brief send a NOTE OFF tx event via the instance inst
 * @param *dev MIDI1.0 device serial instance.
 * @param channel MIDI channel 0 == CH1 (Macro's exist for this e.g. CH16)
 * @param key MIDI key number
 * @param velocity NOTE OFF velocity (not used by many...)
 */
void midi1_serial_note_off(const struct device *dev,
                           uint8_t channel, uint8_t key, uint8_t velocity);

/**
 * @brief send a Control Change tx event via the instance inst
 * @param *dev MIDI1.0 device serial instance.
 * @param channel MIDI channel 0 == CH1 (Macro's exist for this e.g. CH16)
 * @param controller MIDI controller value
 * @param value MIDI value
 */
void midi1_serial_control_change(const struct device *dev,
                                 uint8_t channel,
                                 uint8_t controller, uint8_t val);

/**
 * @brief send a Channel aftertouch tx event via the instance inst
 * @param *dev MIDI1.0 device serial instance.
 * @param channel MIDI channel 0 == CH1 (Macro's exist for this e.g. CH16)
 * @param value MIDI value
 */
void midi1_serial_channelaftertouch(const struct device *dev,
                                    uint8_t channel, uint8_t val);

/**
 * @brief send a ModWheel (MSB and LSB) via the instance inst
 *
 * @note Modulation Wheel both LSB and MSB
 * @note range: 0 --> 16383
 * @param *dev MIDI1.0 device serial instance.
 * @param channel MIDI channel 0 == CH1 (Macro's exist for this e.g. CH16)
 * @param value MIDI value
 */
void midi1_serial_modwheel(const struct device *dev,
                           uint8_t channel, uint16_t val);

/**
 * @brief send a Pitchwheel change (MSB and LSB) via the instance inst
 *
 * @note PitchWheel is always with 14 bit value.
 * @note       LOW   MIDDLE   HIGH
 * @note range: 0 --> 8192  --> 16383
 *
 * @param *dev MIDI1.0 device serial instance.
 * @param channel MIDI channel 0 == CH1 (Macro's exist for this e.g. CH16)
 * @param value MIDI value
 */
void midi1_serial_pitchwheel(const struct device *dev,
                             uint8_t channel, uint16_t val);

/* System Common messages */
/*___         _                ___
 / __|_  _ __| |_ ___ _ __    / __|___ _ __  _ __  ___ _ _
 \__ \ || (_-<  _/ -_) '  \  | (__/ _ \ '  \| '  \/ _ \ ' \
 |___/\_, /__/\__\___|_|_|_|  \___\___/_|_|_|_|_|_\___/_||_|
 */

/**
 * @brief send MIDI1 timing clock
 *
 * @param *dev MIDI1.0 device serial instance.
 */
void midi1_serial_timingclock(const struct device *dev);

/**
 * @brief send MIDI1 start
 *
 * @param *dev MIDI1.0 device serial instance.
 */
void midi1_serial_start(const struct device *dev);

/**
 * @brief send MIDI1 continue
 *
 * @param *dev MIDI1.0 device serial instance.
 */
void midi1_serial_continue(const struct device *dev);

/**
 * @brief send MIDI1 stop
 *
 * @param *dev MIDI1.0 device serial instance.
 */
void midi1_serial_stop(const struct device *dev);

/**
 * @brief send MIDI1 active sense
 *
 * @param *dev MIDI1.0 device serial instance.
 */
void midi1_serial_active_sensing(const struct device *dev);

/**
 * @brief send MIDI1 RESET
 *
 * @param *dev MIDI1.0 device serial instance.
 */
void midi1_serial_reset(const struct device *dev);

/* System exclusive messages*/
/*___         _               ___        _         _
 / __|_  _ __| |_ ___ _ __   | __|_ ____| |_  _ __(_)_ _____
 \__ \ || (_-<  _/ -_) '  \  | _|\ \ / _| | || (_-< \ V / -_)
 |___/\_, /__/\__\___|_|_|_| |___/_\_\__|_|\_,_/__/_|\_/\___|
 |__/
 */

/**
 * @brief Begin a System Exclusive (SysEx) transmission.
 *
 * Sends the SysEx start byte (0xF0) and prepares the instance
 * for transmitting SysEx payload bytes.
 *
 * @param *dev MIDI1.0 device serial instance.
 */
void midi1_sysex_start(const struct device *dev);

/**
 * @brief Transmit a single SysEx data byte.
 *
 * Only bytes in the range 0–127 are transmitted. Any value with
 * bit 7 set is ignored, as SysEx payload must contain data bytes only.
 *
 * @param *dev MIDI1.0 device serial instance.
 * @param c    SysEx data byte (0–127).
 */
void midi1_sysex_char(const struct device *dev, uint8_t c);

/**
 * @brief Transmit a block of SysEx data bytes.
 *
 * Iterates through the provided buffer and transmits each byte
 * using midi1_sysex_char(). Illegal bytes (>= 128) are ignored.
 *
 * @param *dev MIDI1.0 device serial instance.
 * @param data Pointer to SysEx payload buffer.
 * @param len  Number of bytes in the buffer.
 */
void midi1_sysex_data_bulk(const struct device *dev,
                           const uint8_t * data, uint32_t len);

/**
 * @brief End a System Exclusive (SysEx) transmission.
 *
 * Sends the SysEx end byte (0xF7) and finalizes the message.
 *
 * @param *dev MIDI1.0 device serial instance.
 */
void midi1_sysex_stop(const struct device *dev);

#endif                          /* MIDI1_SERIAL_H */
/* EOF */
