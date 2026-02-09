/**
 * @file midi1_pll.c
 * @brief Simple integer PLL for MIDI clock synchronization (24 PPQN).
 * @author Jan-Willem Smaal <usenet@gispen.org>
 * @date 20251229
 * license SPDX-License-Identifier: Apache-2.0
 */
#include <stdint.h>

/* MIDI1.0 clock Phase Locked Loop (PLL) */
#include "midi1_pll.h"
/* sbpm_to_us_interval() is defined here 1*/
#include "midi1.h"

/* TODO: implement the BPM setting as it's ignored now.  */
void midi1_pll_init(struct midi1_pll_data *data,
			  uint16_t sbpm,
			  uint32_t clock_freq)
{
	/* If the user has not provided settings take the defaults */
	if (!data->k ) data->k 	= MIDI1_PLL_FILTER_K;
	if (!data->gain) data->gain = MIDI1_PLL_GAIN_G;
	if (!data->tracking_g) data->tracking_g = MIDI1_PLL_TRACK_GAIN;
	/*
	 * TODO: We set an average value for the BPM rather than starting from
	 * TODO: later base it on BPM.
	 */
	data->nominal_interval_ticks = 503000;
	data->internal_interval_ticks = (int32_t) data->nominal_interval_ticks;
	data->filtered_error = 0;
	data->clock_freq = clock_freq;
}

/*
 * measured_interval_ticks is in hardware clock ticks of course
 */
void midi1_pll_process_interval(struct midi1_pll_data *data,
				      uint32_t measured_interval_ticks)
{
	if (measured_interval_ticks == 0U) {
		/* ignore bogus measurement */
		return;
	}

	/* 1. Interval error: measured - internal */
	int32_t error =
	    (int32_t) measured_interval_ticks - data->internal_interval_ticks;

	/* 2. Low-pass filter the error */
	data->filtered_error +=
	    (error - data->filtered_error) / data->k;

	/* 3. Adjust internal interval around nominal */
	data->internal_interval_ticks =
		(int32_t) data->nominal_interval_ticks +
		data->filtered_error / data->gain;

	/*
	 * Slow tracking: adapt nominal interval towards long-term average.
	 *
	 * Add a small fraction of the filtered error each pulse.
	 * This makes nominal_interval_ticks follow real BPM over time.
	 */
	data->nominal_interval_ticks +=
	    (int32_t) data->filtered_error / data->tracking_g;
}

int32_t midi1_pll_get_interval_ticks(struct midi1_pll_data *data)
{
	return data->nominal_interval_ticks;
}

uint32_t midi1_pll_get_interval_us(struct midi1_pll_data *data)
{
	if (data->clock_freq == 0) {
		return 0;
	}

	uint64_t us =
	    ((uint64_t) data->nominal_interval_ticks * 1000000ULL) /
	data->clock_freq;
	return (uint32_t) us;
}
