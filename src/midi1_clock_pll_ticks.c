/**
 * @file midi1_clock_pll_ticks.c
 * @brief Simple integer PLL for MIDI clock synchronization (24 PPQN).
 * @author Jan-Willem Smaal <usenet@gispen.org>
 * @date 20251229
 * license SPDX-License-Identifier: Apache-2.0
 */
#include <stdint.h>
/* For printk only */
#include <zephyr/kernel.h>

/* MIDI1.0 clock Phase Locked Loop (PLL) */
#include "midi1_clock_pll_ticks.h"
/* sbpm_to_us_interval() is defined here 1*/
#include "midi1.h"

static uint32_t midi1_nominal_interval_ticks;
static int32_t midi1_internal_interval_ticks;
static int32_t midi1_filtered_error = 0;
static uint32_t midi1_clock_freq = 0;

/* TODO: implement!!!  it's ignoring sbpm now! */
void midi1_pll_ticks_init(uint16_t sbpm, uint32_t clock_freq)
{
	// TODO: implement now set a static value
	midi1_nominal_interval_ticks = 503000;
	midi1_internal_interval_ticks = (int32_t) midi1_nominal_interval_ticks;
	midi1_filtered_error = 0;
	midi1_clock_freq = clock_freq;
}

/*
 * measured_interval_ticks is in hardware clock ticks of course
 */
void midi1_pll_ticks_process_interval(uint32_t measured_interval_ticks)
{
	if (measured_interval_ticks == 0U) {
		/* ignore bogus measurement */
		return;
	}

	/* 1. Interval error: measured - internal */
	int32_t error =
	    (int32_t) measured_interval_ticks - midi1_internal_interval_ticks;

	/* 2. Low-pass filter the error */
	midi1_filtered_error +=
	    (error - midi1_filtered_error) / MIDI1_PLL_FILTER_K;

	/* 3. Adjust internal interval around nominal */
	midi1_internal_interval_ticks =
	    (int32_t) midi1_nominal_interval_ticks +
	    midi1_filtered_error / MIDI1_PLL_GAIN_G;

	/*
	 * Slow tracking: adapt nominal interval towards long-term average.
	 *
	 * Add a small fraction of the filtered error each pulse.
	 * This makes nominal_interval_ticks follow real BPM over time.
	 */
	midi1_nominal_interval_ticks +=
	    (int32_t) midi1_filtered_error / MIDI1_PLL_TRACK_GAIN;

#if DEBUG_PLL
	printk("PLL meas=%u  err=%d  filt=%d  int=%d nominal=%d\n",
	       measured_interval_ticks,
	       error,
	       midi1_filtered_error,
	       midi1_internal_interval_ticks, midi1_nominal_interval_ticks);
#endif
}

int32_t midi1_pll_ticks_get_interval_ticks(void)
{
	return midi1_nominal_interval_ticks;
}

uint32_t midi1_pll_ticks_get_interval_us(void)
{
	if (midi1_clock_freq == 0) {
		return 0;
	}

	uint64_t us =
	    ((uint64_t) midi1_nominal_interval_ticks * 1000000ULL) /
	    midi1_clock_freq;
	return (uint32_t) us;
}
