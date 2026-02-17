/**
 * @brief MIDI 1.0 application "MODEL"
 *
 * @author Jan-Willem Smaal <usenet@gispen.org>
 * @date 20260107
 *
 * license SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/types.h>
#include <zephyr/kernel.h>
#include "model.h"

//static struct human_bpm_model g_model;
static human_bpm_model_t g_model;
static struct k_mutex g_model_lock;

void model_init(void)
{
	k_mutex_init(&g_model_lock);
}

void model_set(bool hr_connected,
	       uint16_t hr_bpm,
	       uint16_t meas_sbpm,
	       uint16_t pll_sbpm,
	       uint32_t bpm_led_interval)
{
	k_mutex_lock(&g_model_lock, K_FOREVER);
	g_model.hr_connected = hr_connected;
	/* Only change things in the model when they have a value */
	if(hr_bpm) g_model.hr_bpm = hr_bpm;
	if(meas_sbpm) g_model.meas_sbpm = meas_sbpm;
	if(pll_sbpm) g_model.pll_sbpm = pll_sbpm;
	if(bpm_led_interval) g_model.bpm_led_interval = bpm_led_interval;
	g_model.last_update_ms = k_uptime_get_32();
	k_mutex_unlock(&g_model_lock);
}

void model_get(human_bpm_model_t *out)
{
	k_mutex_lock(&g_model_lock, K_FOREVER);
	*out = g_model;
	k_mutex_unlock(&g_model_lock);
}

human_bpm_model_t model_get2(void)
{
	struct human_bpm_model ret;
	k_mutex_lock(&g_model_lock, K_FOREVER);
	ret = g_model;
	k_mutex_unlock(&g_model_lock);
	return ret;
}

bpm_led_status_t model_get_led_status(void) {
	bpm_led_status_t ret = LED_UNDEF;
	k_mutex_lock(&g_model_lock, K_FOREVER);
	ret = g_model.bpm_led_status;
	k_mutex_unlock(&g_model_lock);
	return ret;
}

void model_set_led_status(bpm_led_status_t led_stat) {
	k_mutex_lock(&g_model_lock, K_FOREVER);
	g_model.bpm_led_status = led_stat;
	k_mutex_unlock(&g_model_lock);
	return;
}

/* EOF */
