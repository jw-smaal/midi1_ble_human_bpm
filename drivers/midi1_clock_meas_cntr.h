
/**
 * @file midi_clock_meas_cntr.h
 * @brief MIDI 1.0 incoming Clock BPM measurement using Zephyr counter device.
 * @details

 * Uses a free-running hardware counter to timestamp incoming MIDI Clock
 * (0xF8) pulses with microsecond precision. uses PIT0 channel 1

 * Scaled BPM representation (sbpm):
 *   1.00 BPM   -> 100
 *   100.00 BPM -> 10000
 *
 * Call midi1_clock_meas_cntr_pulse() for each received MIDI Clock tick.
 *
 * @author Jan-Willem Smaal <usenet@gispen.org>
 * @date 20251230
 * updated 20260207
 * license SPDX-License-Identifier: Apache-2.0
 */
#ifndef MIDI1_CLOCK_MEAS_CNTR_H
#define MIDI1_CLOCK_MEAS_CNTR_H
#include <stdint.h>
#include <stdbool.h>
#include <zephyr/device.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/logging/log.h>

#include "midi1.h"
#include "midi1_clock_meas_cntr.h"
#include "midi1_blockavg.h"

struct midi1_clock_meas_cntr_config {
	const struct device *counter_dev;
};

struct midi1_clock_meas_cntr_data {
	/* const struct device *counter_dev_ch1; */
	uint32_t last_ts_ticks;
	uint32_t scaled_bpm;
	uint32_t last_interval_ticks;
	bool valid;
	uint32_t clock_freq;
	bool count_up;
	/* Timestamp exposed to PLL */
	uint32_t last_tick_timestamp_ticks;
	/* Moving average instance */
	struct midi1_blockavg midi1_blockavg;
};

struct midi1_clock_meas_cntr_api {
	void (*pulse)(const struct device *dev);
	uint32_t (*get_sbpm)(const struct device *dev);
	bool (*is_valid)(const struct device *dev);
	uint32_t (*last_timestamp)(const struct device *dev);
	uint32_t (*interval_ticks)(const struct device *dev);
	uint32_t (*clock_freq)(const struct device *dev);
	uint32_t (*interval_us)(const struct device *dev);
};

/**
 * @note We are using PIT0 channel 1 one the FRDM MCXC242 development board
 * In the device-tree overlay make sure it's enabled !
 *
 * &pit0 {
 *	status = "okay";
 * };
 *
 */

/**
 * @brief Initialize the measurement subsystem.
 *
 * @note Must be called once at startup or when transport restarts.
 */
int midi1_clock_meas_cntr_init(const struct device *dev);

/**
 * @brief Notify the measurement module that a MIDI Clock (0xF8) pulse arrived.
 *
 * @note Call this from your MIDI handler.
 */
void midi1_clock_meas_cntr_pulse(const struct device *dev);

/**
 * @brief Get last measured BPM in scaled form (BPM * 100).
 * @return 0 if no valid measurement yet.
 */
uint32_t midi1_clock_meas_cntr_get_sbpm(const struct device *dev);

/**
 * @brief Returns true if a valid BPM estimate is available.
 *
 * @return true if measurement is valid
 * @return false if measurement is not available or invalid
 */
bool midi1_clock_meas_cntr_is_valid(const struct device *dev);

/**
 * @brief Returns the timestamp (ticks) when the  MIDI Clock tick.
 * was received
 * This can be used by the PLL e.g.
 * @return timestamp in ticks (decreasing if using PIT0 channel1)
 */
uint32_t midi1_clock_meas_cntr_last_timestamp(const struct device *dev);

/**
 * @brief Returns the interval (ticks) when the  MIDI Clock tick.
 * was received compared to the previous one.
 *
 * @return tick interval in ticks
 */
uint32_t midi1_clock_meas_cntr_interval_ticks(const struct device *dev);

/**
 * @brief Returns the clock frequency of the counter
 *
 * @return clock_freq frequency in Hz.
 */
uint32_t midi1_clock_meas_cntr_clock_freq(const struct device *dev);

/**
 * @brief Returns the interval us when the  MIDI Clock tick.
 * was received compared to the previous one.
 *
 * @return tick interval in us
 */
uint32_t midi1_clock_meas_cntr_interval_us(const struct device *dev);

#endif                          /* MIDI1_CLOCK_MEAS_CNTR_H */
/* EOF */
