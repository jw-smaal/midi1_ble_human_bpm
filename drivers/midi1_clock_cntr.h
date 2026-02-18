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
	void (*callback_fn)(void);
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
	uint32_t (*get_interval_us)(const struct device *dev);
	uint32_t (*get_interval_tick)(const struct device *dev);
	bool (*get_running)(const struct device *dev);
	void (*register_callback)(const struct device *dev,
				  void (*callback_fn)(void));
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

/**
 * @brief Get the current MIDI clock tick interval in microseconds.
 *
 * This function returns the duration of a single MIDI clock tick,
 * expressed in microseconds. The interval is derived from the current
 * tempo and internal timing configuration.
 *
 * @param dev Pointer to the MIDI clock counter device instance.
 *
 * @return Tick interval in microseconds.
 */
uint32_t midi1_clock_cntr_get_interval_us(const struct device *dev);

/**
 * @brief Get the current MIDI clock tick interval in device ticks.
 *
 * This function returns the duration of a MIDI clock tick expressed in
 * device-specific timer ticks.
 *
 * @param dev Pointer to the MIDI clock counter device instance.
 *
 * @return Tick interval in hardware timer ticks.
 */
uint32_t midi1_clock_cntr_get_interval_tick(const struct device *dev);

/**
 * @brief Check whether the MIDI clock counter is currently running.
 *
 * This function indicates whether the clock generator is active and
 * producing MIDI timing pulses.
 *
 * @param dev Pointer to the MIDI clock counter device instance.
 *
 * @return true if the clock is running, false otherwise.
 */
bool midi1_clock_cntr_get_running(const struct device *dev);

/**
 * @brief Register a callback for MIDI clock tick events.
 *
 * Registers a user‑provided callback function that is invoked on each
 * generated MIDI clock tick. Only one callback may be active at a time;
 * registering a new one replaces the previous callback.
 *
 * @param dev Pointer to the MIDI clock counter device instance.
 * @param callback_fn Function to call on each MIDI clock tick. Passing
 *                    NULL disables the callback.
 */
void midi1_clock_cntr_register_callback(const struct device *dev,
					 void (*callback_fn)(void));

#endif /* MIDI1_CLOCK_COUNTER */
/* EOF */
