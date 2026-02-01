/**
 * @brief MIDI 1.0 receive thread.
 *
 * @author Jan-Willem Smaal <usenet@gispen.org>
 * @date 20260107
 *
 * license SPDX-License-Identifier: Apache-2.0
 */
#include <stdio.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/sys/printk.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/display.h>
#include <zephyr/display/cfb.h>

#include <lvgl.h>
#include <string.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(midi1_receive_thread, CONFIG_LOG_DEFAULT_LEVEL);

/* Moved to ../drivers */
#include "midi1_serial.h"

/* Some helpers to print out the note name */
#include "note.h"

/* Common stuff in the MIDI monitor application */
#include "common.h"

K_MSGQ_DEFINE(midi_msgq, MIDI_LINE_MAX, MIDI_MSGQ_MAX, 4);
K_MSGQ_DEFINE(midi_raw_msgq, MIDI_LINE_MAX, MIDI_MSGQ_MAX, 4);
/**
 * @brief Callbacks/delegates for 'midi1_serial.c' after parsing MIDI1.0
 *
 * @note
 * Do not block in these
 * parser this one is blocked untill the delegate is finished.
 * It's better if you have something running for a longer time
 * to fire off a workqueue.
 *
 * It's channel + 1 because 0 = CH1 in MIDI.
 */
void note_on_handler(uint8_t channel, uint8_t note, uint8_t velocity)
{
	char line[MIDI_LINE_MAX];
	struct midi1_raw mid_raw = {
		.channel = channel,
		.p1 = note,
		.p2 = velocity
	};
	k_msgq_put(&midi_raw_msgq, &mid_raw, K_NO_WAIT);
	
	snprintf(line, sizeof(line), "CH: %d -> Note   on: %s %03d %03d",
	         channel + 1,
	         noteToTextWithOctave(note, false), note, velocity);
	LOG_INF("%s", line);
	k_msgq_put(&midi_msgq, line, K_NO_WAIT);
}

void note_off_handler(uint8_t channel, uint8_t note, uint8_t velocity)
{
	char line[MIDI_LINE_MAX];
	struct midi1_raw mid_raw = {
		.channel = channel,
		.p1 = note,
		.p2 = velocity
	};
	k_msgq_put(&midi_raw_msgq, &mid_raw, K_NO_WAIT);
	
	snprintf(line, sizeof(line), "CH: %d -> Note  off: %s %03d %03d",
	         channel + 1,
	         noteToTextWithOctave(note, false), note, velocity);
	LOG_INF("%s", line);
	k_msgq_put(&midi_msgq, line, K_NO_WAIT);
}

void pitchwheel_handler(uint8_t channel, uint8_t lsb, uint8_t msb)
{
	/* 14 bit value for the pitch wheel  */
	int16_t pwheel = (int16_t) ((msb << 7) | lsb) - PITCHWHEEL_CENTER;

	char line[MIDI_LINE_MAX];
	snprintf(line, sizeof(line), "CH: %d -> Pitchwheel: %d",
	         channel + 1, pwheel);
	LOG_INF("%s", line);
	k_msgq_put(&midi_msgq, line, K_NO_WAIT);

}

#if 0
void control_change_handler_model(uint8_t channel,
                                  uint8_t controller, uint8_t value)
{
	char line[MIDI_LINE_MAX];
	
	snprintf(line, sizeof(line), "CH: %d -> CC: %d value: %d",
	         channel + 1, controller, value);
	LOG_INF("%s", line);
	k_msgq_put(&midi_msgq, line, K_NO_WAIT);
}
#endif

void control_change_handler(uint8_t channel, uint8_t controller, uint8_t value)
{
	char line[MIDI_LINE_MAX];
	struct midi1_raw mid_raw = {
		.channel = channel,
		.p1 = controller,
		.p2 = value
	};
	k_msgq_put(&midi_raw_msgq, &mid_raw, K_NO_WAIT);
	
	snprintf(line, sizeof(line), "CH: %d -> CC: %d value: %d",
	         channel + 1, controller, value);
	LOG_INF("%s", line);
	k_msgq_put(&midi_msgq, line, K_NO_WAIT);
}

void realtime_handler(uint8_t msg)
{
	LOG_INF("Realtime: %d", msg);
}

void sysex_start_handler(void)
{
	LOG_INF("sysex_start_handler()");
}

void sysex_data_handler(uint8_t data)
{
	LOG_INF("%x ", data);
}

void sysex_stop_handler(void)
{
	LOG_INF("sysex_stop_handler()");
}

/* ---------------------------- THREADS ------------------------------------ */

/**
 * Serial receive parser thread - receiveparser keeps reading data
 * filled by the ISR and then calls callbacks.
 */
void midi1_serial_receive_thread(void)
{
	const struct device *midi = DEVICE_DT_GET(DT_NODELABEL(midi0));

	if (!device_is_ready(midi)) {
		LOG_ERR("receive_thread Serial MIDI1 device not ready");
		return;
	}
	const struct midi1_serial_api *mid = midi->api;

	/*
	 * Set the callbacks in the driver to our own callbacks.  Pointers
	 * left null are not used in the callbacks.
	 * e.g. right now the aftertouch is not handled.
	 */
	struct midi1_serial_callbacks my_cb = {
		.note_on = note_on_handler,
		.note_off = note_off_handler,
		.control_change = control_change_handler,
		.pitchwheel = pitchwheel_handler,
		.sysex_start = sysex_start_handler,
		.sysex_data = sysex_data_handler,
		.sysex_stop = sysex_stop_handler
	};
	/* midi1_serial_register_callbacks(midi, &my_cb); */
	mid->register_callbacks(midi, &my_cb);

	while (1) {
		/* As this call is blocking no need to sleep in between */
		mid->receiveparser(midi);
	}
}

/*
 * Make sure the MIDI receive thread gets enough cycles.
 * that is why i set to 1
 */
K_THREAD_DEFINE(midi1_serial_receive_tid, 4096,
                midi1_serial_receive_thread, NULL, NULL, NULL, 1, 0, 0);

/* EOF */
