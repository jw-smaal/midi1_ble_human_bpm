#ifndef MIDI1_CLOCK_CNTR
#define MIDI1_CLOCK_CNTR
/**
 * @file midi1_clock_counter.h
 * @brief MIDI1.0 clock driver for zephyr RTOS using hardware clock timer.
 * NXP pit0_channel0, ctimer lptimer or anything that supports the Zephyr
 * counter API.
 *
 * @author Jan-Willem Smaal <usenet@gispen.org>
 * @date 20251214
 * license SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include <stdint.h>
#include <stddef.h>


struct midi1_clock_cntr_config {
	const struct device *counter_dev;
	const struct device *midi1_serial_dev;
};

struct midi1_clock_cntr_data {
	uint32_t interval_us;
	uint32_t interval_ticks;
	bool running_cntr;
	uint16_t sbpm;
	bool count_up_clk;
};

struct midi1_clock_cntr_api {
	uint32_t (*cpu_frequency)(const struct device *dev);
	void (*start)(const struct device *dev, uint32_t interval_us);
	void (*ticks_start)(const struct device * dev, uint32_t ticks);
	void (*update_ticks)(const struct device * dev, uint32_t new_ticks);
	void (*stop)(const struct device *dev);
	void (*gen)(const struct device *dev, uint16_t sbpm);
	void (*gen_sbpm)(const struct device *dev, uint16_t sbpm);
	uint16_t (*get_sbpm)(const struct device *dev);

};

/**
 * @brief Initialize MIDI clock subsystem with the MIDI device handle.
 *
 * @note Call once at startup before starting the clock.
 * @param midi1_dev MIDI device pointer
 */
int midi1_clock_cntr_init(const struct device *dev);

/**
 * @brief getter for the internal counter frequency in MHZ
 *
 * @return frequency in MHz
 */
uint32_t midi1_clock_cntr_cpu_frequency(const struct device *dev);

/**
 * @brief Start periodic MIDI clock. interval_us must be > 0.
 *
 * @param interval_us interval in us must be higher than 0
 */
void midi1_clock_cntr_start(const struct device *dev, uint32_t interval_us);

/**
 * @brief Start clock with MIDI ticks as argument (more accurate)
 *
 * @param ticks tick reference to the frequency in clocks
 * @see midi1_clock_cntr_cpu_frequency(void)
 */
void midi1_clock_cntr_ticks_start(const struct device *dev, uint32_t ticks);

/**
 * @brief placeholder for updating the clock
 *
 * @note this is not supported on PIT0 channel 0 on NXP
 */
void midi1_clock_cntr_update_ticks(const struct device *dev,
				   uint32_t new_ticks);

/**
 * @brief Stop the clock
 */
void midi1_clock_cntr_stop(const struct device *dev);

/**
 * @brief Generate MIDI1.0 clock
 *
 * @param midi1_dev MIDI device pointer
 * @param sbpm scaled BPM like 123.12 must be entered like 12312
 */
void midi1_clock_cntr_gen(const struct device *dev, uint16_t sbpm);

/**
 * @brief Generate MIDI1.0 clock
 *
 * @param sbpm scaled BPM like 123.12 must be entered like 12312
 */
void midi1_clock_cntr_gen_sbpm(const struct device *dev, uint16_t sbpm);

/**
 * @brief Getter for the current bpm
 *
 * @return sbpm scaled BPM like 123.12 is returned like 12312
 */
uint16_t midi1_clock_cntr_get_sbpm(const struct device *dev);

#endif /* MIDI1_CLOCK_COUNTER */
/* EOF */
