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

static struct human_bpm_model g_model;
static struct k_mutex g_model_lock;

void model_init(void)
{
	k_mutex_init(&g_model_lock);
}

void model_set(bool hr_connected,
	       uint16_t hr_sbpm,
	       uint16_t meas_sbpm,
	       uint16_t pll_sbpm)
{
	k_mutex_lock(&g_model_lock, K_FOREVER);
	
	g_model.hr_connected = hr_connected;
	if(g_model.hr_sbpm) g_model.hr_sbpm = hr_sbpm;
	if(g_model.meas_sbpm) g_model.meas_sbpm = meas_sbpm;
	if(g_model.pll_sbpm) g_model.pll_sbpm = pll_sbpm ;
	g_model.last_update_ms = k_uptime_get_32();
	
	k_mutex_unlock(&g_model_lock);
}

void model_get(struct human_bpm_model *out)
{
	k_mutex_lock(&g_model_lock, K_FOREVER);
	*out = g_model;
	k_mutex_unlock(&g_model_lock);
}

/* EOF */
