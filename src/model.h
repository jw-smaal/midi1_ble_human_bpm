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
struct human_bpm_model {
	bool hr_connected;
	uint16_t hr_sbpm;
	uint16_t meas_sbpm;
	uint16_t pll_sbpm;
	uint32_t last_update_ms;
};

#endif
