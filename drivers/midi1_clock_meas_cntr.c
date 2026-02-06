
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
/* Internal state */
static const struct device *g_counter_dev_ch1 = NULL;

static uint32_t g_last_ts_ticks = 0;
static uint32_t g_scaled_bpm = 0;
static uint32_t g_last_interval_ticks = 0;
static bool g_valid = false;
static uint32_t g_clock_freq = 0;
static bool g_count_up = false;

/* Timestamp exposed to PLL */
static uint32_t g_last_tick_timestamp_ticks = 0;

/* Moving average instance */
static struct midi1_blockavg midi1_blockavg = { 0 };

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
	uint32_t ticks = 0;

	int err = counter_get_value(g_counter_dev_ch1, &ticks);
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
void midi1_clock_meas_cntr_init(const struct device *dev)
{
	g_last_ts_ticks = 0;
	g_scaled_bpm = 12000;
	g_last_interval_ticks = 0;
	g_valid = false;
	g_clock_freq = 0;
	g_count_up = false;

	/* Init a instance of a block average */
	midi1_blockavg_init(&midi1_blockavg);

	/* g_counter_dev_ch1 = DEVICE_DT_GET(DT_NODELABEL(COUNTER_DEVICE_CH1)); */
	g_counter_dev_ch1 = DEVICE_DT_GET(DT_ALIAS(COUNTER_DEVICE_CH1));

	if (!device_is_ready(g_counter_dev_ch1)) {
		LOG_ERR("Clock measurement counter device not ready");
		return;
	}
	g_clock_freq = counter_get_frequency(g_counter_dev_ch1);
	if (!g_clock_freq) {
		LOG_ERR("Clock measurement counter unable to read frequency");
		return;
	}
	/* PIT0 counts down, ctimer0 counts up and cannot be changed */
	g_count_up = counter_is_counting_up(g_counter_dev_ch1);

	/* Do this once and then let it run free .. */
	const struct counter_top_cfg top_cfg = {
		.ticks = 0xFFFFFFFF,    /* full 32‑bit range */
		.callback = (void *)midi1_clock_meas_callback,
		//.callback = 0,
		.user_data = 0,
		.flags = 0,
	};
	counter_set_top_value(g_counter_dev_ch1, &top_cfg);

	/* Start free-running counter */
	int err = counter_start(g_counter_dev_ch1);
	if (err != 0) {
		LOG_ERR("Failed to start measurement counter: %d", err);
	}
	/* Initialize last timestamp to current counter value (down-counter) */
	g_last_ts_ticks = midi1_clock_meas_now_ticks(dev);
}

/* ------------------------------------------------------------------ */
void midi1_clock_meas_cntr_pulse(const struct device *dev)
{
	uint32_t now_ticks = midi1_clock_meas_now_ticks(dev);
	uint32_t interval_ticks = 0;

	/* Expose timestamp to PLL or other users */
	g_last_tick_timestamp_ticks = now_ticks;

	/* First pulse after init: we have no previous timestamp yet */
	if (g_last_ts_ticks == 0U) {
		g_last_ts_ticks = now_ticks;
		return;
	}

	/* interval_ticks = g_last_ts_ticks - now_ticks; */
	if (g_count_up) {
		/*
		 * For a up-counter,
		 * elapsed = current - previous  (unsigned wrap-safe)
		 */
		interval_ticks = now_ticks - g_last_ts_ticks;
	} else {
		/*
		 * For a down-counter,
		 * elapsed = previous - current (unsigned wrap-safe)
		 */
		interval_ticks = g_last_ts_ticks - now_ticks;
	}
	g_last_ts_ticks = now_ticks;

	/* Reject zero or obviously bogus intervals to avoid BPM math crashes */
	if (interval_ticks == 0U) {
		return;
	}

	g_last_interval_ticks = interval_ticks;

	uint32_t interval_us = midi1_clock_meas_cntr_interval_us(dev);
	if (interval_us == 0U) {
		return;
	}

	/*
	 * Let average the BPM over 24 clock's 0xF8 received otherwise
	 * it goes all over the place
	 */
	midi1_blockavg_add(&midi1_blockavg, interval_ticks);

	if (midi1_blockavg_count(&midi1_blockavg) == MIDI1_BLOCKAVG_SIZE) {
		uint32_t avg_ticks = midi1_blockavg_average(&midi1_blockavg);
		uint32_t interval_us =
		    counter_ticks_to_us(g_counter_dev_ch1, avg_ticks);
		g_scaled_bpm = MIDI1_SCALED_BPM_NUMERATOR / interval_us;
		g_valid = true;
	}
}

/*
 * Some convinience functions.
 */
uint32_t midi1_clock_meas_cntr_get_sbpm(const struct device *dev)
{
	return g_valid ? g_scaled_bpm : 0;
}

bool midi1_clock_meas_cntr_is_valid(const struct device *dev)
{
	return g_valid;
}

uint32_t midi1_clock_meas_cntr_last_timestamp(const struct device *dev)
{
	return g_last_tick_timestamp_ticks;
}

uint32_t midi1_clock_meas_cntr_interval_ticks(const struct device *dev)
{
	return g_last_interval_ticks;
}

uint32_t midi1_clock_meas_cntr_clock_freq(const struct device *dev)
{
	return g_clock_freq;
}

uint32_t midi1_clock_meas_cntr_interval_us(const struct device *dev)
{
	return counter_ticks_to_us(g_counter_dev_ch1,
	                           midi1_clock_meas_cntr_interval_ticks(dev));
}

/* EOF */
