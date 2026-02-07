
/*
 * @brief MIDI 1.0 Clock BPM measurement using Zephyr counter device.
 * Fully working and verified with external MIDI gear
 * Hardware-accurate clock.
 *
 * @author Jan-Willem Smaal <usenet@gispen.org>
 * license SPDX-License-Identifier: Apache-2.0
 */
#include <stdint.h>
#include <stdbool.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/logging/log.h>

#include "midi1.h"
#include "midi1_clock_meas_cntr.h" 
#include "midi1_blockavg.h"
LOG_MODULE_REGISTER(midi1_clock_meas_cntr, CONFIG_LOG_DEFAULT_LEVEL);

/* ------------------------------------------------------------------ */
/*
 * Numerator:
 * scaledBPM = (60 * 1_000_000 * 100) / (24 * interval_us)
 *           = 250000000 / interval_us
 */
#define MIDI1_SCALED_BPM_NUMERATOR ((60ull * US_PER_SECOND * BPM_SCALE) / 24ull)

/* ------------------------------------------------------------------ */
/*
 * Read free-running counter note this is running down not up!
 */
static inline uint32_t midi1_clock_meas_now_ticks(const struct device *dev)
{
	const struct midi1_clock_meas_cntr_config *cfg = dev->config;
	uint32_t ticks = 0;

	int err = counter_get_value(cfg->counter_dev, &ticks);
	if (err != 0) {
		LOG_ERR("counter_get_value error");
		return 0;
	}
	return ticks;
}

/*
 * defined but doing nothing I could not find the correct way to
 * switch off the IRQ with the top_cfg...
 */
void midi1_clock_meas_callback(void)
{
	return;
}

/*
 * It's working but I had to configure a callback to stop
 * zephyr from crashing.  If I set the callback to 0 it crashes.
 */
int midi1_clock_meas_cntr_init(const struct device *dev)
{
	const struct midi1_clock_meas_cntr_config *cfg = dev->config;
	struct midi1_clock_meas_cntr_data *data = dev->data;
	
	data->last_ts_ticks = 0;
	data->scaled_bpm = 12000;
	data->last_interval_ticks = 0;
	data->valid = false;
	data->clock_freq = 0;
	data->count_up = false;
	
	/* Init a instance of a block average */
	midi1_blockavg_init(&data->midi1_blockavg);
	
	/* Counter device is already assigned during kernel init */

	if (!device_is_ready(cfg->counter_dev)) {
		LOG_ERR("Clock measurement counter device not ready");
		return -1;
	}
	data->clock_freq = counter_get_frequency(cfg->counter_dev);
	if (!data->clock_freq) {
		LOG_ERR("Clock measurement counter unable to read frequency");
		return -1;
	}
	/* PIT0 counts down, ctimer0 counts up and cannot be changed */
	data->count_up = counter_is_counting_up(cfg->counter_dev);

	/* Do this once and then let it run free .. */
	const struct counter_top_cfg top_cfg = {
		.ticks = 0xFFFFFFFF,    /* full 32‑bit range */
		.callback = (void *)midi1_clock_meas_callback,
		//.callback = 0,
		.user_data = 0,
		.flags = 0,
	};
	counter_set_top_value(cfg->counter_dev, &top_cfg);

	/* Start free-running counter */
	int err = counter_start(cfg->counter_dev);
	if (err != 0) {
		LOG_ERR("Failed to start measurement counter: %d", err);
		return -1;
	}
	/* Initialize last timestamp to current counter value (down-counter) */
	data->last_ts_ticks = midi1_clock_meas_now_ticks(dev);
	return 0;
}

/* ------------------------------------------------------------------ */
void midi1_clock_meas_cntr_pulse(const struct device *dev)
{
	const struct midi1_clock_meas_cntr_config *cfg = dev->config;
	struct midi1_clock_meas_cntr_data *data = dev->data;
	  
	uint32_t now_ticks = midi1_clock_meas_now_ticks(dev);
	uint32_t interval_ticks = 0;

	/* Expose timestamp to PLL or other users */
	data->last_tick_timestamp_ticks = now_ticks;

	/* First pulse after init: we have no previous timestamp yet */
	if (data->last_ts_ticks == 0U) {
		data->last_ts_ticks = now_ticks;
		return;
	}

	/* interval_ticks = g_last_ts_ticks - now_ticks; */
	if (data->count_up) {
		/*
		 * For a up-counter,
		 * elapsed = current - previous  (unsigned wrap-safe)
		 */
		interval_ticks = now_ticks - data->last_ts_ticks;
	} else {
		/*
		 * For a down-counter,
		 * elapsed = previous - current (unsigned wrap-safe)
		 */
		interval_ticks = data->last_ts_ticks - now_ticks;
	}
	data->last_ts_ticks = now_ticks;

	/* Reject zero or obviously bogus intervals to avoid BPM math crashes */
	if (interval_ticks == 0U) {
		return;
	}

	data->last_interval_ticks = interval_ticks;

	uint32_t interval_us = midi1_clock_meas_cntr_interval_us(dev);
	if (interval_us == 0U) {
		return;
	}

	/*
	 * Let average the BPM over 24 clock's 0xF8 received otherwise
	 * it goes all over the place
	 */
	midi1_blockavg_add(&data->midi1_blockavg, interval_ticks);

	if (midi1_blockavg_count(&data->midi1_blockavg) == MIDI1_BLOCKAVG_SIZE) {
		uint32_t avg_ticks = midi1_blockavg_average(&data->midi1_blockavg);
		uint32_t interval_us =
		    counter_ticks_to_us(cfg->counter_dev, avg_ticks);
		data->scaled_bpm = MIDI1_SCALED_BPM_NUMERATOR / interval_us;
		data->valid = true;
	}
}

/*
 * Some convinience functions and getters
 */
uint32_t midi1_clock_meas_cntr_get_sbpm(const struct device *dev)
{
	struct midi1_clock_meas_cntr_data *data = dev->data;
	return data->valid ? data->scaled_bpm : 0;
}

bool midi1_clock_meas_cntr_is_valid(const struct device *dev)
{
	struct midi1_clock_meas_cntr_data *data = dev->data;
	return data->valid;
}

uint32_t midi1_clock_meas_cntr_last_timestamp(const struct device *dev)
{
	struct midi1_clock_meas_cntr_data *data = dev->data;
	return data->last_tick_timestamp_ticks;
}

uint32_t midi1_clock_meas_cntr_interval_ticks(const struct device *dev)
{
	struct midi1_clock_meas_cntr_data *data = dev->data;
	return data->last_interval_ticks;
}

uint32_t midi1_clock_meas_cntr_clock_freq(const struct device *dev)
{
	struct midi1_clock_meas_cntr_data *data = dev->data;
	return data->clock_freq;
}

uint32_t midi1_clock_meas_cntr_interval_us(const struct device *dev)
{
	const struct midi1_clock_meas_cntr_config *cfg = dev->config;
	return counter_ticks_to_us(cfg->counter_dev,
	                           midi1_clock_meas_cntr_interval_ticks(dev));
}

/* Zephyr device driver API link to our actual implementation */
static const struct midi1_clock_meas_cntr_api midi1_clock_meas_cntr_driver_api = {
	.pulse = midi1_clock_meas_cntr_pulse,
	.get_sbpm = midi1_clock_meas_cntr_get_sbpm,
	.is_valid = midi1_clock_meas_cntr_is_valid,
	.last_timestamp = midi1_clock_meas_cntr_last_timestamp,
	.interval_ticks = midi1_clock_meas_cntr_interval_ticks,
	.clock_freq = midi1_clock_meas_cntr_clock_freq,
	.interval_us = midi1_clock_meas_cntr_interval_us,
};


#define MIDI1_CLOCK_MEAS_CNTR_INIT_PRIORITY 85

#define DT_DRV_COMPAT midi1_clock_meas_cntr

#define MIDI1_CLOCK_MEAS_CNTR_DEFINE(inst)                                    \
static struct midi1_clock_meas_cntr_data midi1_clock_meas_cntr_data_##inst;             \
static const struct midi1_clock_meas_cntr_config midi1_clock_meas_cntr_config_##inst = {\
.counter_dev = DEVICE_DT_GET(DT_INST_PROP(inst, counter)),                    \
};                                                                            \
DEVICE_DT_INST_DEFINE(inst,                                                   \
midi1_clock_meas_cntr_init,                                                   \
NULL,                                                                         \
&midi1_clock_meas_cntr_data_##inst,                                           \
&midi1_clock_meas_cntr_config_##inst,                                         \
POST_KERNEL,                                                                  \
MIDI1_CLOCK_MEAS_CNTR_INIT_PRIORITY,                                          \
&midi1_clock_meas_cntr_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MIDI1_CLOCK_MEAS_CNTR_DEFINE)

/* EOF */
