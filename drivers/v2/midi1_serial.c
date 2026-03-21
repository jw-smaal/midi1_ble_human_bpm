/**
 * @file midi1_serial.c
 * @brief MIDI USART implementation on Zephyr RTOS
 *
 * @note
 * MIDI USART implementation that also implements the MIDI1.0
 * "running status" on transmit (optional) and receive (manditory).
 *
 * The implementation intends to be as complete and close to the MIDI1.0
 * specifications as possible.
 *
 * Originally created in 2014 for AVR MCU's ported to Zephyr RTOS in 2024.
 * @author Jan-Willem Smaal <usenet@gispen.org>
 * updated 20241224
 * updated 20260103
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/ring_buffer.h>
#include <errno.h>

#include "midi1.h"
#include "midi1_serial.h"

/*
 * Empty NO OP (noop) callbacks assigned if the caller leaves the callbacks
 * empty.  Gives the caller flexibillity in what to respond to.
 */
static inline void midi1_noop_note_on(uint8_t channel, uint8_t note, uint8_t velocity)
{
}

static inline void midi1_noop_note_off(uint8_t channel, uint8_t note, uint8_t velocity)
{
}

static inline void midi1_noop_control_change(uint8_t channel, uint8_t controller, uint8_t value)
{
}

static inline void midi1_noop_pitchwheel(uint8_t channel, uint8_t lsb, uint8_t msb)
{
}

static inline void midi1_noop_program_change(uint8_t channel, uint8_t number)
{
}

static inline void midi1_noop_channel_aftertouch(uint8_t channel, uint8_t pressure)
{
}

static inline void midi1_noop_poly_aftertouch(uint8_t channel, uint8_t note, uint8_t pressure)
{
}

/* System common */
static inline void midi1_noop_realtime(uint8_t msg)
{
}

static inline void midi1_noop_mtc_quarter_frame(uint8_t data)
{
}

static inline void midi1_noop_song_position(uint16_t pos)
{
}

static inline void midi1_noop_song_select(uint8_t song)
{
}

static inline void midi1_noop_tune_request(void)
{
}

/* SysEx */
static inline void midi1_noop_sysex_start(void)
{
}

static inline void midi1_noop_sysex_data(uint8_t data)
{
}

static inline void midi1_noop_sysex_stop(void)
{
}

/**
 * @brief MIDI Byte Receive parser implementation - Interrupt Service Routine
 *
 * @param device device pointer
 * @param user_data user data (in our case the instance struct)
 * @note we use a Message Queue FIFO buffer to store the incoming MIDI messages
 */
#ifdef CONFIG_ZTEST
void midi1_serial_isr_callback(const struct device *dev, void *user_data)
#else
static void midi1_serial_isr_callback(const struct device *dev, void *user_data)
#endif
{
	uint8_t c;
	const struct device *midi1_serial = (const struct device *)user_data;
	const struct midi1_serial_config *cfg = midi1_serial->config;
	struct midi1_serial_data *data = midi1_serial->data;

	if (!uart_irq_update(cfg->uart)) {
		return;
	}
	if (!uart_irq_rx_ready(cfg->uart)) {
		return;
	}

	/* read until FIFO empty */
	while (uart_fifo_read(cfg->uart, &c, 1) == 1) {
		if (k_msgq_put(&data->msgq, &c, K_NO_WAIT) != 0) {
			/* Buffer full, message dropped */
		}
	}
}

/**
 * Inits the serial USART ISR routine so we start processing incoming MIDI.
 */
int midi1_serial_init(const struct device *dev)
{
	const struct midi1_serial_config *cfg = dev->config;
	struct midi1_serial_data *data = dev->data;

	data->running_status_rx = 0;
	data->third_byte_flag = 0;
	data->midi_c2 = 0;
	data->midi_c3 = 0;

	data->running_status_tx = 0;
	data->running_status_tx_count = 0;
	data->in_sysex = false;
	data->last_status_tx_time = k_uptime_get_32() - 500U;

	/* If a null pointer is given reassign to the NO OP function */
	data->cb.note_on = midi1_noop_note_on;
	data->cb.note_off = midi1_noop_note_off;
	data->cb.control_change = midi1_noop_control_change;
	data->cb.realtime = midi1_noop_realtime;
	data->cb.mtc_quarter_frame = midi1_noop_mtc_quarter_frame;
	data->cb.song_position = midi1_noop_song_position;
	data->cb.song_select = midi1_noop_song_select;
	data->cb.tune_request = midi1_noop_tune_request;
	data->cb.pitchwheel = midi1_noop_pitchwheel;
	data->cb.program_change = midi1_noop_program_change;
	data->cb.channel_aftertouch = midi1_noop_channel_aftertouch;
	data->cb.poly_aftertouch = midi1_noop_poly_aftertouch;
	data->cb.sysex_start = midi1_noop_sysex_start;
	data->cb.sysex_data = midi1_noop_sysex_data;
	data->cb.sysex_stop = midi1_noop_sysex_stop;

	/* Assign a MSQ to this instance */
	k_msgq_init(&data->msgq, data->msgq_buffer, MSG_SIZE, MSGQ_SIZE);

	if (!device_is_ready(cfg->uart)) {
		return -ENODEV;
	}
	int ret =
		uart_irq_callback_user_data_set(cfg->uart, midi1_serial_isr_callback, (void *)dev);
	if (ret < 0) {
		return ret;
	}

	uart_irq_rx_enable(cfg->uart);
	return 0;
}

int midi1_serial_register_callbacks(const struct device *dev, struct midi1_serial_callbacks *cb)
{
	if (!cb) {
		return -EINVAL;
	}

	struct midi1_serial_data *data = dev->data;

	if (cb->note_on) {
		data->cb.note_on = cb->note_on;
	}
	if (cb->note_off) {
		data->cb.note_off = cb->note_off;
	}
	if (cb->control_change) {
		data->cb.control_change = cb->control_change;
	}
	if (cb->pitchwheel) {
		data->cb.pitchwheel = cb->pitchwheel;
	}
	if (cb->program_change) {
		data->cb.program_change = cb->program_change;
	}
	if (cb->channel_aftertouch) {
		data->cb.channel_aftertouch = cb->channel_aftertouch;
	}
	if (cb->poly_aftertouch) {
		data->cb.poly_aftertouch = cb->poly_aftertouch;
	}
	if (cb->realtime) {
		data->cb.realtime = cb->realtime;
	}
	if (cb->mtc_quarter_frame) {
		data->cb.mtc_quarter_frame = cb->mtc_quarter_frame;
	}
	if (cb->song_position) {
		data->cb.song_position = cb->song_position;
	}
	if (cb->song_select) {
		data->cb.song_select = cb->song_select;
	}
	if (cb->tune_request) {
		data->cb.tune_request = cb->tune_request;
	}
	if (cb->sysex_start) {
		data->cb.sysex_start = cb->sysex_start;
	}
	if (cb->sysex_data) {
		data->cb.sysex_data = cb->sysex_data;
	}
	if (cb->sysex_stop) {
		data->cb.sysex_stop = cb->sysex_stop;
	}

	return 0;
}

#define RUNSTAT_TMOUT CONFIG_MIDI1_SERIAL_RUNNING_STATUS_TIMEOUT_MS
#define RUNSTAT_TIMES CONFIG_MIDI1_SERIAL_RUNNING_STATUS_REPEAT

static bool midi1_need_status(struct midi1_serial_data *data, uint8_t status)
{
	uint32_t now = k_uptime_get_32();

	bool need_status = (now - data->last_status_tx_time > RUNSTAT_TMOUT) ||
			   (status != data->running_status_tx) ||
			   (data->running_status_tx_count >= RUNSTAT_TIMES);

	return need_status;
}

void midi1_serial_note_on(const struct device *dev, uint8_t channel, uint8_t key, uint8_t velocity)
{
	const struct midi1_serial_config *cfg = dev->config;
	struct midi1_serial_data *data = dev->data;
	uint32_t now = k_uptime_get_32();
	uint8_t status = C_NOTE_ON | (channel & 0x0F);

	if (key > 127 || velocity > 127) {
		return;
	}

	if (midi1_need_status(data, status)) {
		uart_poll_out(cfg->uart, status);
		data->running_status_tx = status;
		data->running_status_tx_count = 0;
		data->last_status_tx_time = now;
	}
	uart_poll_out(cfg->uart, key);
	uart_poll_out(cfg->uart, velocity);
	data->running_status_tx_count++;
}

void midi1_serial_note_off(const struct device *dev, uint8_t channel, uint8_t key, uint8_t velocity)
{
	const struct midi1_serial_config *cfg = dev->config;
	struct midi1_serial_data *data = dev->data;
	uint32_t now = k_uptime_get_32();
	uint8_t status = C_NOTE_OFF | (channel & 0x0F);

	if (key > 127 || velocity > 127) {
		return;
	}

	if (midi1_need_status(data, status)) {
		uart_poll_out(cfg->uart, status);
		data->running_status_tx = status;
		data->running_status_tx_count = 0;
		data->last_status_tx_time = now;
	}
	uart_poll_out(cfg->uart, key);
	uart_poll_out(cfg->uart, velocity);
	data->running_status_tx_count++;
}

void midi1_serial_control_change(const struct device *dev, uint8_t channel, uint8_t controller,
				 uint8_t val)
{
	const struct midi1_serial_config *cfg = dev->config;
	struct midi1_serial_data *data = dev->data;
	uint32_t now = k_uptime_get_32();
	uint8_t status = C_CONTROL_CHANGE | (channel & 0x0F);

	if (controller > 127 || val > 127) {
		return;
	}

	if (midi1_need_status(data, status)) {
		uart_poll_out(cfg->uart, status);
		data->running_status_tx = status;
		data->running_status_tx_count = 0;
		data->last_status_tx_time = now;
	}
	uart_poll_out(cfg->uart, controller);
	uart_poll_out(cfg->uart, val);
	data->running_status_tx_count++;
}

void midi1_serial_channelaftertouch(const struct device *dev, uint8_t channel, uint8_t val)
{
	const struct midi1_serial_config *cfg = dev->config;
	struct midi1_serial_data *data = dev->data;
	uint32_t now = k_uptime_get_32();
	uint8_t status = C_CHANNEL_AFTERTOUCH | (channel & 0x0F);

	if (val > 127) {
		return;
	}

	if (midi1_need_status(data, status)) {
		uart_poll_out(cfg->uart, status);
		data->running_status_tx = status;
		data->running_status_tx_count = 0;
		data->last_status_tx_time = now;
	}
	uart_poll_out(cfg->uart, val);
	data->running_status_tx_count++;
}

void midi1_serial_program_change(const struct device *dev, uint8_t channel, uint8_t number)
{
	const struct midi1_serial_config *cfg = dev->config;
	struct midi1_serial_data *data = dev->data;
	uint32_t now = k_uptime_get_32();
	uint8_t status = C_PROGRAM_CHANGE | (channel & 0x0F);

	if (number > 127) {
		return;
	}

	if (midi1_need_status(data, status)) {
		uart_poll_out(cfg->uart, status);
		data->running_status_tx = status;
		data->running_status_tx_count = 0;
		data->last_status_tx_time = now;
	}
	uart_poll_out(cfg->uart, number);
	data->running_status_tx_count++;
}

void midi1_serial_polyaftertouch(const struct device *dev, uint8_t channel, uint8_t key,
				 uint8_t val)
{
	const struct midi1_serial_config *cfg = dev->config;
	struct midi1_serial_data *data = dev->data;
	uint32_t now = k_uptime_get_32();
	uint8_t status = C_POLYPHONIC_AFTERTOUCH | (channel & 0x0F);

	if (key > 127 || val > 127) {
		return;
	}

	if (midi1_need_status(data, status)) {
		uart_poll_out(cfg->uart, status);
		data->running_status_tx = status;
		data->running_status_tx_count = 0;
		data->last_status_tx_time = now;
	}
	uart_poll_out(cfg->uart, key);
	uart_poll_out(cfg->uart, val);
	data->running_status_tx_count++;
}

void midi1_serial_modwheel(const struct device *dev, uint8_t channel, uint16_t val)
{
	midi1_serial_control_change(dev, channel, CTL_MSB_MODWHEEL, (val >> 7) & 0x7F);
	midi1_serial_control_change(dev, channel, CTL_LSB_MODWHEEL, val & 0x7F);
}

void midi1_serial_pitchwheel(const struct device *dev, uint8_t channel, uint16_t val)
{
	const struct midi1_serial_config *cfg = dev->config;
	struct midi1_serial_data *data = dev->data;
	uint32_t now = k_uptime_get_32();
	uint8_t status = C_PITCH_WHEEL | (channel & 0x0F);

	if (val > 16383) {
		return;
	}

	if (midi1_need_status(data, status)) {
		uart_poll_out(cfg->uart, status);
		data->running_status_tx = status;
		data->running_status_tx_count = 0;
		data->last_status_tx_time = now;
	}
	uart_poll_out(cfg->uart, val & 0x7F);
	uart_poll_out(cfg->uart, (val >> 7) & 0x7F);
	data->running_status_tx_count++;
}

void midi1_serial_mtc_quarter_frame(const struct device *dev, uint8_t data_byte)
{
	const struct midi1_serial_config *cfg = dev->config;
	struct midi1_serial_data *data = dev->data;
	uart_poll_out(cfg->uart, SYSTEM_MTC_QUARTER_FRAME);
	uart_poll_out(cfg->uart, data_byte & 0x7F);
	data->running_status_tx = 0;
}

void midi1_serial_song_position(const struct device *dev, uint16_t pos)
{
	const struct midi1_serial_config *cfg = dev->config;
	struct midi1_serial_data *data = dev->data;
	uart_poll_out(cfg->uart, SYSTEM_SONG_POSITION);
	uart_poll_out(cfg->uart, pos & 0x7F);
	uart_poll_out(cfg->uart, (pos >> 7) & 0x7F);
	data->running_status_tx = 0;
}

void midi1_serial_song_select(const struct device *dev, uint8_t song)
{
	const struct midi1_serial_config *cfg = dev->config;
	struct midi1_serial_data *data = dev->data;
	uart_poll_out(cfg->uart, SYSTEM_SONG_SELECT);
	uart_poll_out(cfg->uart, song & 0x7F);
	data->running_status_tx = 0;
}

void midi1_serial_tune_request(const struct device *dev)
{
	const struct midi1_serial_config *cfg = dev->config;
	struct midi1_serial_data *data = dev->data;
	uart_poll_out(cfg->uart, SYSTEM_TUNE_REQUEST);
	data->running_status_tx = 0;
}

void midi1_serial_timingclock(const struct device *dev)
{
	uart_poll_out(((struct midi1_serial_config *)dev->config)->uart, RT_TIMING_CLOCK);
}

void midi1_serial_start(const struct device *dev)
{
	uart_poll_out(((struct midi1_serial_config *)dev->config)->uart, RT_START);
}

void midi1_serial_continue(const struct device *dev)
{
	uart_poll_out(((struct midi1_serial_config *)dev->config)->uart, RT_CONTINUE);
}

void midi1_serial_stop(const struct device *dev)
{
	uart_poll_out(((struct midi1_serial_config *)dev->config)->uart, RT_STOP);
}

void midi1_serial_active_sensing(const struct device *dev)
{
	uart_poll_out(((struct midi1_serial_config *)dev->config)->uart, RT_ACTIVE_SENSING);
}

void midi1_serial_reset(const struct device *dev)
{
	struct midi1_serial_data *data = dev->data;
	uart_poll_out(((struct midi1_serial_config *)dev->config)->uart, RT_RESET);
	data->running_status_tx = 0;
}

void midi1_sysex_start(const struct device *dev)
{
	struct midi1_serial_data *data = dev->data;
	uart_poll_out(((struct midi1_serial_config *)dev->config)->uart, SYSTEM_EXCLUSIVE_START);
	data->running_status_tx = 0;
}

void midi1_sysex_char(const struct device *dev, uint8_t c)
{
	if (!(c & 0x80)) {
		uart_poll_out(((struct midi1_serial_config *)dev->config)->uart, c);
	}
}

void midi1_sysex_data_bulk(const struct device *dev, const uint8_t *data, uint32_t len)
{
	for (uint32_t i = 0; i < len; i++) {
		midi1_sysex_char(dev, data[i]);
	}
}

void midi1_sysex_stop(const struct device *dev)
{
	struct midi1_serial_data *data = dev->data;
	uart_poll_out(((struct midi1_serial_config *)dev->config)->uart, SYSTEM_EXCLUSIVE_END);
	data->running_status_tx = 0;
}

void midi1_serial_receiveparser(const struct device *dev)
{
	struct midi1_serial_data *data = dev->data;
	uint8_t c;

	if (k_msgq_get(&data->msgq, &c, K_FOREVER) != 0) {
		return;
	}

	if (c & 0x80) {
		if (c >= 0xF8) {
			if (c == RT_RESET) {
				data->running_status_rx = 0;
			}
			data->cb.realtime(c);
		} else {
			data->in_sysex = false;
			data->third_byte_flag = 0;
			if (c < 0xF0) {
				data->running_status_rx = c;
			} else {
				data->running_status_rx = 0;
				if (c == SYSTEM_TUNE_REQUEST) {
					data->cb.tune_request();
				} else if (c == SYSTEM_EXCLUSIVE_START) {
					data->in_sysex = true;
					data->cb.sysex_start();
				} else if (c == SYSTEM_EXCLUSIVE_END) {
					data->cb.sysex_stop();
				} else if (c == SYSTEM_MTC_QUARTER_FRAME ||
					   c == SYSTEM_SONG_SELECT || c == SYSTEM_SONG_POSITION) {
					data->running_status_rx = c;
				}
			}
		}
		return;
	}

	if (data->in_sysex) {
		data->cb.sysex_data(c);
		return;
	}

	if (data->third_byte_flag) {
		data->third_byte_flag = 0;
		data->midi_c3 = c;

		if (data->running_status_rx == SYSTEM_SONG_POSITION) {
			data->cb.song_position((data->midi_c3 << 7) | data->midi_c2);
			data->running_status_rx = 0;
			return;
		}

		uint8_t status = data->running_status_rx & 0xF0;
		uint8_t chan = data->running_status_rx & 0x0F;

		if (status == 0x90) {
			if (data->midi_c3 == 0) {
				data->cb.note_off(chan, data->midi_c2, 0);
			} else {
				data->cb.note_on(chan, data->midi_c2, data->midi_c3);
			}
		} else if (status == 0x80) {
			data->cb.note_off(chan, data->midi_c2, data->midi_c3);
		} else if (status == 0xE0) {
			data->cb.pitchwheel(chan, data->midi_c2, data->midi_c3);
		} else if (status == 0xA0) {
			data->cb.poly_aftertouch(chan, data->midi_c2, data->midi_c3);
		} else if (status == 0xB0) {
			data->cb.control_change(chan, data->midi_c2, data->midi_c3);
		}
	} else {
		if (data->running_status_rx == 0) {
			return;
		}
		if (data->running_status_rx == SYSTEM_MTC_QUARTER_FRAME) {
			data->cb.mtc_quarter_frame(c);
			data->running_status_rx = 0;
		} else if (data->running_status_rx == SYSTEM_SONG_SELECT) {
			data->cb.song_select(c);
			data->running_status_rx = 0;
		} else if (data->running_status_rx < 0xC0 ||
			   (data->running_status_rx >= 0xE0 && data->running_status_rx < 0xF0) ||
			   data->running_status_rx == SYSTEM_SONG_POSITION) {
			data->midi_c2 = c;
			data->third_byte_flag = 1;
		} else if (data->running_status_rx < 0xE0) {
			uint8_t status = data->running_status_rx & 0xF0;
			uint8_t chan = data->running_status_rx & 0x0F;
			if (status == 0xC0) {
				data->cb.program_change(chan, c);
			} else if (status == 0xD0) {
				data->cb.channel_aftertouch(chan, c);
			}
		}
	}
}

static const struct midi1_serial_api midi1_serial_driver_api = {
	.register_callbacks = midi1_serial_register_callbacks,
	.receiveparser = midi1_serial_receiveparser,
	.note_on = midi1_serial_note_on,
	.note_off = midi1_serial_note_off,
	.control_change = midi1_serial_control_change,
	.channelaftertouch = midi1_serial_channelaftertouch,
	.program_change = midi1_serial_program_change,
	.polyaftertouch = midi1_serial_polyaftertouch,
	.modwheel = midi1_serial_modwheel,
	.pitchwheel = midi1_serial_pitchwheel,
	.mtc_quarter_frame = midi1_serial_mtc_quarter_frame,
	.song_position = midi1_serial_song_position,
	.song_select = midi1_serial_song_select,
	.tune_request = midi1_serial_tune_request,
	.timingclock = midi1_serial_timingclock,
	.start = midi1_serial_start,
	.continu = midi1_serial_continue,
	.stop = midi1_serial_stop,
	.active_sensing = midi1_serial_active_sensing,
	.reset = midi1_serial_reset,
	.sysex_start = midi1_sysex_start,
	.sysex_char = midi1_sysex_char,
	.sysex_data_bulk = midi1_sysex_data_bulk,
	.sysex_stop = midi1_sysex_stop};

#define DT_DRV_COMPAT midi1_serial
#define MIDI1_SERIAL_DEFINE(inst)                                                                  \
	static struct midi1_serial_data midi1_serial_data_##inst;                                  \
	static const struct midi1_serial_config midi1_serial_config_##inst = {                     \
		.uart = DEVICE_DT_GET(DT_INST_PROP(inst, uart)),                                   \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, midi1_serial_init, NULL, &midi1_serial_data_##inst,            \
			      &midi1_serial_config_##inst, POST_KERNEL, 79,                        \
			      &midi1_serial_driver_api);
DT_INST_FOREACH_STATUS_OKAY(MIDI1_SERIAL_DEFINE)
