#ifndef COMMON_H
#define COMMON_H
/**
 * Common stuff
 *
 * @author Jan-Willem Smaal <usenet@gispen.org>
 * @date 20260107
 *
 * license SPDX-License-Identifier: Apache-2.0
 */
#include <stdio.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/sys/printk.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/display.h>
#include <zephyr/display/cfb.h>

#include <lvgl.h>
#include <string.h>
#include <zephyr/logging/log.h>

#define MIDI_LINE_MAX 128

#define MIDI_MSGQ_MAX 32
/* Defined in midi1_receive_thread */
extern struct k_msgq midi_msgq;
extern struct k_msgq midi_raw_msgq;

/* just a struct we pass to be able to draw a bar */
struct midi1_raw {
	uint8_t channel;
	uint8_t p1;
	uint8_t p2;
};


#endif
