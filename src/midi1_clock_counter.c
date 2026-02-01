/**
 * @file midi1_clock_counter.c
 * @brief implementation of midi1_clock_clock
 * this is a _hardware_ based counter PIT0 channel 0
 * tested with NXP FRDM_MCXC242 in zephyr.
 *
 * @author Jan-Willem Smaal <usenet@gispen.org
 * @date 20251214
 * license SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/audio/midi.h>
#include <zephyr/drivers/counter.h>

#if 0
/* This is part of the MIDI2 library prj.conf
 * CONFIG_MIDI2_UMP_STREAM_RESPONDER=y
 * /zephyr/lib/midi2/ump_stream_responder.h it get's linked in.
 */
#include <sample_usbd.h>
#include <zephyr/usb/class/usbd_midi2.h>
#include <ump_stream_responder.h>
#endif


#define MIDI_CLOCK_ON_PIN 0

#if MIDI_CLOCK_ON_PIN
/*
 * To measure MIDI clock externally we toggle a PIN and measure with
 * the oscilloscope
 */
#include <zephyr/drivers/gpio.h>
#endif

/* MIDI helpers by J-W Smaal*/
#include "midi1.h"
#include "midi1_clock_counter.h"


#if LEGACY_CODE
/* TODO: put this in a struct */
static atomic_t g_midi1_running_cntr = ATOMIC_INIT(0);
static uint16_t g_sbpm = 0;
/* This is a USB-MIDI device handle */
static const struct device *g_midi1_dev;
const struct device *g_counter_dev;
static bool g_midi1_clk_count_up_clk = false;
#endif 

/*
 * MIDI clock measurement on a PIN.
 * I used PTC8 on the FRDM_MCXC242 scope confirms correct implementation.  
 */
#if MIDI_CLOCK_ON_PIN
#define CLOCK_FREQ_OUT DT_NODELABEL(freq_out)
static const struct gpio_dt_spec clock_pin =
GPIO_DT_SPEC_GET(CLOCK_FREQ_OUT, gpios);

static void midi1_debug_gpio_init(void)
{
	int ret = gpio_pin_configure_dt(&clock_pin, GPIO_OUTPUT_INACTIVE);
	if (ret < 0) {
		printk("Error configing pin\n");
		return;
	}
}
#endif

/* 
 * This is the ISR/callback TODO: check if usbd_midi_send is non-blocking
 * TODO: add uart send as well
 */
static void midi1_cntr_handler(const struct device *dev, void *midi1_dev_arg)
{
#if MIDI_CLOCK_ON_PIN
	gpio_pin_toggle_dt(&clock_pin);
#endif

	if (!atomic_get(&g_midi1_running_cntr)) {
		return;
	}
	if (g_midi1_dev) {
		//usbd_midi_send(g_midi1_dev, midi1_timing_clock());
	}
	return;
}

uint32_t midi1_clock_cntr_cpu_frequency(const struct device *dev)
{
	return counter_get_frequency(g_counter_dev);
}


/*
 * Empty NO OP (noop) callbacks assigned if the caller leaves the callbacks
 * empty.
 */
static inline void midi1_clock_cntr_noop_cb()
{
}

/*
 * Initialize MIDI clock subsystem
 */
void midi1_clock_cntr_init(const struct device *dev)
{
	const struct midi1_clock_cntr_config *cfg = dev->config;
	struct midi1_clock_cntr_data *data = dev->data;
	
	data->running_cntr = false;
	data->sbpm = 12000;
	//cfg->counter_dev = DEVICE_DT_GET(DT_ALIAS(COUNTER_DEVICE));
	
	if (!device_is_ready(g_counter_dev)) {
		printk("Counter device not ready\n");
		return;
	}
	/* PIT0 counts down, ctimer0 counts up and cannot be changed */
	data->count_up_clk = counter_is_counting_up(g_counter_dev);
#if MIDI_CLOCK_ON_PIN
	/*
	 * TODO: future implementation make an optional GPIO output
	 * TODO: for the clock
	 */
#endif
	return;
}

/*
 * Initialize MIDI clock subsystem with the MIDI device handle. Call
 * once at startup before starting the clock.
 */
void midi1_clock_cntr_init2(const struct device *midi1_dev_arg)
{
	atomic_set(&g_midi1_running_cntr, 0);
	/* g_counter_dev = DEVICE_DT_GET(DT_NODELABEL(COUNTER_DEVICE)); */
	g_counter_dev = DEVICE_DT_GET(DT_ALIAS(COUNTER_DEVICE));

	if (!device_is_ready(g_counter_dev)) {
		printk("Counter device not ready\n");
		return;
	}
	/* PIT0 counts down, ctimer0 counts up and cannot be changed */
	g_midi1_clk_count_up_clk = counter_is_counting_up(g_counter_dev);

	g_midi1_dev = midi1_dev_arg;
#if MIDI_CLOCK_ON_PIN
	midi1_debug_gpio_init();
#endif
	return;
}

/*
 * Start periodic MIDI clock. ticks must be > 0.
 */
void midi1_clock_cntr_ticks_start(const struct device *dev, uint32_t ticks)
{
	int err = 0;
	if (ticks == 0u) {
		return;
	}
	atomic_set(&g_midi1_running_cntr, 1);
#if MIDI_CLOCK_ON_PIN
	//printk("Ticks requested: %u\n", ticks);
#endif
	struct counter_top_cfg top_cfg = {
		.callback = midi1_cntr_handler,
		.user_data = (void *)g_midi1_dev,
		.ticks = ticks,
		.flags = 0,
	};

	err = counter_set_top_value(g_counter_dev, &top_cfg);
	if (err != 0) {
		printk("Failed to set top value: %d\n", err);
		return;
	}

	/* Start the counter */
	err = counter_start(g_counter_dev);
	if (err != 0) {
		printk("Failed to start counter: %d\n", err);
		return;
	}
}

/* TODO: is not working with PIT!! BUG in my thinking */
void midi1_clock_cntr_update_ticks(const struct device *dev, uint32_t new_ticks)
{
	struct counter_top_cfg top_cfg = {
		.callback = midi1_cntr_handler,
		.user_data = (void *)g_midi1_dev,
		.ticks = new_ticks,
		.flags = COUNTER_TOP_CFG_DONT_RESET,    /* <-- KEY */
	};

	int err = counter_set_top_value(g_counter_dev, &top_cfg);
	if (err != 0) {
		printk("Failed to set top value: %d\n", err);
		return;
	}
	//printk("Updating ticks to: %u\n", new_ticks);
}

/*
 * Start periodic MIDI clock. interval_us must be > 0. 
 */
void midi1_clock_cntr_start(const struct device *dev, uint32_t interval_us)
{
	int err = 0;
	if (interval_us == 0u) {
		return;
	}
	atomic_set(&g_midi1_running_cntr, 1);

	uint32_t ticks = counter_us_to_ticks(g_counter_dev, interval_us);
	g_sbpm = us_interval_to_sbpm(interval_us);

	/*
	 * Configure top value when it overflows the midi1_cntr_handler is
	 * called as an ISR
	 */
	struct counter_top_cfg top_cfg = {
		.callback = midi1_cntr_handler,
		.user_data = (void *)g_midi1_dev,
		.ticks = ticks,
		.flags = 0,
	};

	err = counter_set_top_value(g_counter_dev, &top_cfg);
	if (err != 0) {
		printk("Failed to set top value: %d\n", err);
		return;
	}

	/* Start the counter */
	err = counter_start(g_counter_dev);
	if (err != 0) {
		printk("Failed to start counter: %d\n", err);
		return;
	}
}

/* Stop the clock */
void midi1_clock_cntr_stop(const struct device *dev)
{
	atomic_set(&g_midi1_running_cntr, 0);
}

/**
 * @brief Generate MIDI1.0 clock
 */
void midi1_clock_cntr_gen(const struct device *dev, uint16_t sbpm)
{
	midi1_clock_cntr_stop(dev);
	/* TODO: fixme when we are a proper device driver */
	midi1_clock_cntr_init(dev);
	uint32_t ticks = sbpm_to_ticks(sbpm,
	                               midi1_clock_cntr_cpu_frequency(dev));
	midi1_clock_cntr_ticks_start(dev, ticks);
}

void midi1_clock_cntr_gen_sbpm(const struct device *dev, uint16_t sbpm)
{
	uint32_t ticks = sbpm_to_ticks(sbpm,
	                               midi1_clock_cntr_cpu_frequency(dev));
	midi1_clock_cntr_ticks_start(dev, ticks);
}

uint16_t midi1_clock_cntr_get_sbpm(const struct device *dev)
{
	return g_sbpm;
}

/* EOF */
