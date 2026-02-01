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
//#include <zephyr/audio/midi.h>
#include <zephyr/device.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(midi1_clock_counter, CONFIG_LOG_DEFAULT_LEVEL);

#if MIDI_USB_CONFIGURED
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


#if LEGACY_CODE_BEING_PORTED
/* TODO: put this in a struct */
static atomic_t g_midi1_running_cntr = false;
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
		LOG_ERR("Error configing pin");
		return;
	}
}
#endif

/* 
 * This is the ISR/callback
 * TODO: check if usbd_midi_send is non-blocking
 * TODO: add uart send as well
 */
static void midi1_cntr_handler(const struct device *actual_counter_dev,
			       void *midi1_clk_cntr_dev)
{
	/*
	 * The ISR prototype for the user data is void so we _have_ to
	 * cast it
	 */
	const struct device *midi1_cntr_device =
		(const struct device *)midi1_clk_cntr_dev;
	const struct midi1_clock_cntr_config *cfg = midi1_cntr_device->config;
	struct midi1_clock_cntr_data *data = midi1_cntr_device->data;
	
#if MIDI_CLOCK_ON_PIN
	gpio_pin_toggle_dt(&clock_pin);
#endif
	if (!data->running_cntr) {
		return;
	}
	if (cfg->midi1_serial_dev) {
		// Call something maybe to this in a callback instead?
		LOG_INF("Sending MIDI1 serial clock");
	}
	return;
}

uint32_t midi1_clock_cntr_cpu_frequency(const struct device *dev)
{
	const struct midi1_clock_cntr_config *cfg = dev->config;
	[[maybe_unused]] struct midi1_clock_cntr_data *data = dev->data;
	
	return counter_get_frequency(cfg->counter_dev);
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
int midi1_clock_cntr_init(const struct device *dev)
{
	const struct midi1_clock_cntr_config *cfg = dev->config;
	struct midi1_clock_cntr_data *data = dev->data;
	
	data->running_cntr = false;
	data->sbpm = 12000;
	if (!device_is_ready(cfg->counter_dev)) {
		LOG_ERR("Counter device not ready");
		return -1;
	}
	/* PIT0 counts down, ctimer0 counts up and cannot be changed */
	data->count_up_clk = counter_is_counting_up(cfg->counter_dev);
#if MIDI_CLOCK_ON_PIN
	/*
	 * TODO: future implementation make an optional GPIO output
	 * TODO: for the clock
	 */
#endif
	LOG_INF("Counter device ready");
	return 0;
}


/*
 * Start periodic MIDI clock. ticks must be > 0.
 */
void midi1_clock_cntr_ticks_start(const struct device *dev, uint32_t ticks)
{
	const struct midi1_clock_cntr_config *cfg = dev->config;
	struct midi1_clock_cntr_data *data = dev->data;
	
	int err = 0;
	if (ticks == 0u) {
		return;
	}
	data->running_cntr = true;
	LOG_INF("Ticks requested: %u", ticks);
	struct counter_top_cfg top_cfg = {
		.callback = midi1_cntr_handler,
		.user_data = (void *)dev,
		.ticks = ticks,
		.flags = 0,
	};

	err = counter_set_top_value(cfg->counter_dev, &top_cfg);
	if (err != 0) {
		LOG_ERR("Failed to set top value: %d", err);
		return;
	}

	/* Start the counter */
	err = counter_start(cfg->counter_dev);
	if (err != 0) {
		LOG_ERR("Failed to start counter: %d", err);
		return;
	}
}

/* TODO: is not working with PIT!! BUG in my thinking */
void midi1_clock_cntr_update_ticks(const struct device *dev, uint32_t new_ticks)
{
	const struct midi1_clock_cntr_config *cfg = dev->config;
	struct midi1_clock_cntr_data *data = dev->data;
	
	data->interval_ticks = new_ticks;
	
	struct counter_top_cfg top_cfg = {
		.callback = midi1_cntr_handler,
		.user_data = (void *)dev,
		.ticks = new_ticks,
		.flags = COUNTER_TOP_CFG_DONT_RESET,    /* <-- KEY */
	};

	int err = counter_set_top_value(cfg->counter_dev, &top_cfg);
	if (err != 0) {
		LOG_ERR("Failed to set top value: %d", err);
		return;
	}
	LOG_INF("Updating ticks to: %u", new_ticks);
}

/*
 * Start periodic MIDI clock. interval_us must be > 0. 
 */
void midi1_clock_cntr_start(const struct device *dev, uint32_t interval_us)
{
	const struct midi1_clock_cntr_config *cfg = dev->config;
	struct midi1_clock_cntr_data *data = dev->data;
	
	int err = 0;
	if (interval_us == 0u || dev == NULL ) {
		return;
	}
	data->running_cntr = true;
	data->interval_us = interval_us;

	uint32_t ticks = counter_us_to_ticks(cfg->counter_dev,
					     data->interval_us);
	data->sbpm = us_interval_to_sbpm(data->interval_us);

	/*
	 * Configure top value when it overflows the midi1_cntr_handler is
	 * called as an ISR
	 */
	struct counter_top_cfg top_cfg = {
		.callback = midi1_cntr_handler,
		.user_data = (void *)dev,
		.ticks = ticks,
		.flags = 0,
	};

	err = counter_set_top_value(cfg->counter_dev, &top_cfg);
	if (err != 0) {
		LOG_ERR("Failed to set top value: %d", err);
		return;
	}

	/* Start the free running counter */
	err = counter_start(cfg->counter_dev);
	if (err != 0) {
		LOG_ERR("Failed to start counter: %d", err);
		return;
	}
}

/* Stop the clock */
void midi1_clock_cntr_stop(const struct device *dev)
{
	[[maybe_unused]] const struct midi1_clock_cntr_config *cfg = dev->config;
	struct midi1_clock_cntr_data *data = dev->data;
	
	data->running_cntr = false;
}

/**
 * @brief Generate MIDI1.0 clock
 */
void midi1_clock_cntr_gen(const struct device *dev, uint16_t sbpm)
{
	[[maybe_unused]] const struct midi1_clock_cntr_config *cfg = dev->config;
	[[maybe_unused]] struct midi1_clock_cntr_data *data = dev->data;
	
	//midi1_clock_cntr_stop(dev);
	uint32_t ticks = sbpm_to_ticks(sbpm,
	                               midi1_clock_cntr_cpu_frequency(dev));
	midi1_clock_cntr_ticks_start(dev, ticks);
}

void midi1_clock_cntr_gen_sbpm(const struct device *dev, uint16_t sbpm)
{
	[[maybe_unused]] const struct midi1_clock_cntr_config *cfg = dev->config;
	[[maybe_unused]] struct midi1_clock_cntr_data *data = dev->data;
	
	uint32_t ticks = sbpm_to_ticks(sbpm,
	                               midi1_clock_cntr_cpu_frequency(dev));
	midi1_clock_cntr_ticks_start(dev, ticks);
}

uint16_t midi1_clock_cntr_get_sbpm(const struct device *dev)
{
	[[maybe_unused]] const struct midi1_clock_cntr_config *cfg = dev->config;
	struct midi1_clock_cntr_data *data = dev->data;
	
	return data->sbpm;
}

/* Zephyr device driver API link to our actual implementation */
static const struct midi1_clock_cntr_api midi1_clock_counter_driver_api = {
	.cpu_frequency = midi1_clock_cntr_cpu_frequency,
	.start         = midi1_clock_cntr_start,
	.ticks_start   = midi1_clock_cntr_ticks_start,
	.update_ticks  = midi1_clock_cntr_update_ticks,
	.stop          = midi1_clock_cntr_stop,
	.gen           = midi1_clock_cntr_gen,
	.gen_sbpm      = midi1_clock_cntr_gen_sbpm,
	.get_sbpm      = midi1_clock_cntr_get_sbpm,
};

#define DT_DRV_COMPAT midi1_clock_counter

#define MIDI1_CLOCK_COUNTER_DEFINE(inst)                                      \
static struct midi1_clock_cntr_data midi1_clock_cntr_data_##inst;             \
static const struct midi1_clock_cntr_config midi1_clock_cntr_config_##inst = {\
.counter_dev = DEVICE_DT_GET(DT_INST_PROP(inst, counter)),                    \
.midi1_serial_dev = DEVICE_DT_GET(DT_INST_PROP(inst, midi1_serial)),          \
};                                                                            \
DEVICE_DT_INST_DEFINE(inst,                                                   \
midi1_clock_cntr_init,                                                        \
NULL,                                                                         \
&midi1_clock_cntr_data_##inst,                                                \
&midi1_clock_cntr_config_##inst,                                                  \
POST_KERNEL,                                                                  \
CONFIG_KERNEL_INIT_PRIORITY_DEVICE,                                           \
&midi1_clock_counter_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MIDI1_CLOCK_COUNTER_DEFINE)

/* EOF */
