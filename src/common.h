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


/* Global pll */
#include "midi1_pll.h"
extern struct midi1_pll_data g_pll;

#endif
