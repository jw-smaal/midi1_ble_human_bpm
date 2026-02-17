/**
 * @brief MIDI 1.0 application "MODEL"
 *
 * @author Jan-Willem Smaal <usenet@gispen.org>
 * @date 20260107
 *
 * license SPDX-License-Identifier: Apache-2.0
 */
#ifndef MODEL_H
#define MODEL_H
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

/*
 * state --> Waiting for BLE HR service, Connected,
 * PLL, measured and generated BPM.
 * last update of the model
 */
typedef enum bpm_led_status {
	LED_UNDEF = 0,
	LED_ON	= 1,
	LED_OFF	= 2
}bpm_led_status_t;

typedef struct human_bpm_model {
	bool hr_connected;
	uint16_t hr_bpm;
	uint16_t meas_sbpm;
	uint16_t pll_sbpm;
	uint32_t last_update_ms;
	/* 1 = on, 0 = undefined, 2 = off*/
	bpm_led_status_t bpm_led_status;
	uint32_t bpm_led_interval;
}human_bpm_model_t;

void model_init(void);
void model_set(bool hr_connected,
	       uint16_t hr_bpm,
	       uint16_t meas_sbpm,
	       uint16_t pll_sbpm,
	       uint32_t bpm_led_interval);
void model_get(human_bpm_model_t *out);
bpm_led_status_t model_get_led_status(void);
void model_set_led_status(bpm_led_status_t led_stat);

#endif
