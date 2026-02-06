/**
 * @file midi1_clock_pll_ticks.h
 * @brief Simple integer PLL for MIDI clock synchronization (24 PPQN).
 * @author Jan-Willem Smaal
 * @date 20251229
 * license SPDX-License-Identifier: Apache-2.0
 */
#ifndef MIDI1_CLOCK_PLL_TICKS_H
#define MIDI1_CLOCK_PLL_TICKS_H
#include <stdint.h>

/* Loop filter constants */

/*
 * Low‑pass filter strength keep it high... sudden tempo changes
 * should be followed though.
 */
#define MIDI1_PLL_FILTER_K  4

/*
 * Correction gain keep it low we want to move towards the value
 * but not overshoot
 */
#define MIDI1_PLL_GAIN_G    4

/*
 * Slow loop tracking gain.
 */
#define MIDI1_PLL_TRACK_GAIN 32

#define DEBUG_PLL 0

/**
 * @brief Initialize the MIDI1 PLL with a nominal BPM.
 *
 * @param sbpm  Scaled BPM value (e.g. 12000 for 120.00 BPM)
 */
void midi1_pll_ticks_init(uint16_t sbpm, uint32_t clock_freq);

/**
 * @brief Process an incoming MIDI clock tick interval.
 *
 * @param measured_interval_ticks interval in ticks.
 */
void midi1_pll_ticks_process_interval(uint32_t measured_interval_ticks);

/**
 * @brief Get the current PLL‑corrected 24pqn interval in microseconds.
 *
 * @return interval in microseconds for the next 24pqn internal tick.
 */
uint32_t midi1_pll_ticks_get_interval_us(void);

/**
 * @brief Get the current PLL‑corrected 24pqn tick interval in ticks.
 *
 * @return Interval in ticks for the next 24pqn MIDI clock tick.
 */
int32_t midi1_pll_ticks_get_interval_ticks(void);

#endif                          /* MIDI1_CLOCK_PLL_TICKS_H */
