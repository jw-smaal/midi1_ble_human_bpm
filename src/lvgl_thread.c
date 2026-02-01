/**
 * LVGL stuff
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

#include "common.h"

LOG_MODULE_REGISTER(lvgl_screen1, CONFIG_LOG_DEFAULT_LEVEL);
/*
 *  GLOBALS
 */
static lv_obj_t *label_title;
static lv_obj_t *label_bpm;
static lv_obj_t *label_channel;
static lv_obj_t *ta_midi;
static lv_obj_t *level_bar;
#define LED_COUNT 16
static lv_obj_t *leds[LED_COUNT];

/*
 *  GUI INITIALIZATION (480×320 landscape)
 */
void ui_set_led_level(uint8_t value)
{
	int lit = (value * LED_COUNT) / 128;
	
	/* Should check if led != 0 */
	for (int i = 0; i < LED_COUNT; i++) {
		if (i < lit) {
			lv_led_on(leds[LED_COUNT - 1 - i]);

		} else {
			lv_led_off(leds[LED_COUNT - 1 - i]);
		}
	}
}


[[maybe_unused]] static void initialize_gui2(void)
{
	/* Screen background (solid black, no transparency) */
	lv_obj_set_style_bg_color(lv_screen_active(),
				  lv_color_black(), LV_PART_MAIN);
	lv_obj_set_style_bg_opa(lv_screen_active(), LV_OPA_COVER, LV_PART_MAIN);
	
	/* Default text: white, size 14 */
	lv_obj_set_style_text_color(lv_screen_active(),
				    lv_color_white(), LV_PART_MAIN);
	lv_obj_set_style_text_font(lv_screen_active(),
				   &lv_font_montserrat_14, LV_PART_MAIN);
	/* =====================================================
	 *  TOP BAR
	 * ===================================================== */
	/* Left: static title */
	label_title = lv_label_create(lv_screen_active());
	lv_label_set_text(label_title, "MIDI Monitor ");
	lv_obj_align(label_title, LV_ALIGN_TOP_LEFT, 6, 4);
}

static void initialize_gui(void)
{
	/* Screen background (solid black, no transparency) */
	lv_obj_set_style_bg_color(lv_screen_active(),
	                          lv_color_black(), LV_PART_MAIN);
	lv_obj_set_style_bg_opa(lv_screen_active(), LV_OPA_COVER, LV_PART_MAIN);

	/* Default text: white, size 14 */
	lv_obj_set_style_text_color(lv_screen_active(),
	                            lv_color_white(), LV_PART_MAIN);
	lv_obj_set_style_text_font(lv_screen_active(),
	                           &lv_font_montserrat_14, LV_PART_MAIN);

	/* =====================================================
	 *  TOP BAR
	 * ===================================================== */
	/* Left: static title */
	label_title = lv_label_create(lv_screen_active());
	lv_label_set_text(label_title, "MIDI Monitor by J-W Smaal v0.1");
	lv_obj_align(label_title, LV_ALIGN_TOP_LEFT, 6, 4);

	/* Right: BPM in larger font */
	label_bpm = lv_label_create(lv_screen_active());
	lv_obj_set_style_text_font(label_bpm,
	                           &lv_font_montserrat_18, LV_PART_MAIN);
	lv_obj_set_style_text_color(label_bpm,
				    lv_color_hex(0xff0000),   /* Red */
				    LV_PART_MAIN);
	lv_label_set_text(label_bpm, " 123.89 BPM");
	lv_obj_align(label_bpm, LV_ALIGN_TOP_RIGHT, 0, 0);
	
	
	/* Left: Channel on the left */
	label_channel = lv_label_create(lv_screen_active());
	lv_obj_set_style_text_font(label_channel,
				   &lv_font_montserrat_18, LV_PART_MAIN);
	lv_obj_set_style_text_color(label_channel,
				    lv_color_hex(0x006400),   /* dark green */
				    LV_PART_MAIN);
	lv_label_set_text(label_channel, "CHxx");
	lv_obj_align(label_channel, LV_ALIGN_BOTTOM_LEFT, 0, 0);

	/* =====================================================
	 *  CENTER: LARGE SCROLLABLE TEXT WINDOW
	 * ===================================================== */
	ta_midi = lv_textarea_create(lv_screen_active());
	//lv_label_set_recolor(ta_midi, true);
	lv_obj_set_style_text_font(ta_midi,
				   &lv_font_montserrat_18, LV_PART_MAIN);

	/* Size: nearly full screen minus top and bottom bar */
	lv_obj_set_size(ta_midi, 390, 220);
	lv_obj_align(ta_midi, LV_ALIGN_TOP_LEFT, 6, 40);

	/* No transparency */
	lv_obj_set_style_bg_color(ta_midi, lv_color_black(), LV_PART_MAIN);
	lv_obj_set_style_bg_opa(ta_midi, LV_OPA_COVER, LV_PART_MAIN);

	/* Whitetext by default */
	lv_obj_set_style_text_color(ta_midi, lv_color_white(), LV_PART_MAIN);

	lv_textarea_set_text(ta_midi, "");
	lv_textarea_set_max_length(ta_midi, 4096);
	lv_textarea_set_cursor_click_pos(ta_midi, false);
	
	
	/*
	 * Level bar
	 */
	level_bar = lv_bar_create(lv_screen_active());
	
	/* MIDI range: 0–127 */
	lv_bar_set_range(level_bar, 0, 127);
	
	/* Size and position */
	lv_obj_set_size(level_bar, 240, 20);   /* width, height */
	lv_obj_align(level_bar, LV_ALIGN_BOTTOM_MID, 0, 0);
	
#if 0
	lv_obj_set_style_bg_color(level_bar,
				  lv_color_hex(0x00A000),
				  LV_PART_INDICATOR);
#endif
		
#define LED_COUNT 16
	for (int i = 0; i < LED_COUNT; i++) {
		leds[i] = lv_led_create(lv_screen_active());
		
		/* LED size */
		lv_obj_set_size(leds[i], 10, 10);
		
		/* Vertical placement: top LED at top, bottom LED at bottom */
		lv_obj_align(leds[i],
			     LV_ALIGN_RIGHT_MID,
			     -4,     /* X offset from right edge */
			     -((LED_COUNT - 1) * 12) / 2 + i * 12);
		
	}
}


static void ui_set_bar(const struct midi1_raw mid_raw)
{
	if (!level_bar) {
		return;
	}
	uint8_t v = mid_raw.p2;
	lv_bar_set_value(level_bar, v, LV_ANIM_OFF);
}

[[maybe_unused]] static void ui_set_line(const char *msg)
{
	if (!label_title) {
		return;
	}

	char buf[MIDI_LINE_MAX];
	snprintf(buf, sizeof(buf), "%s", msg);
	lv_textarea_set_text(label_title, buf);
}

#define MAX_MIDI_LINES 9
static int midi_line_count = 0;
[[maybe_unused]] static void ui_add_line2(const char *msg) {
	if (!ta_midi) {
		return;
	}

	lv_textarea_set_text(ta_midi, "");
}

static void ui_add_line(const char *msg)
{
	if (!ta_midi) {
		return;
	}
	
	/* If full, clear everything */
	if (midi_line_count >= MAX_MIDI_LINES) {
		lv_textarea_set_text(ta_midi, "");
		midi_line_count = 0;
	}
	
	/* Format new line */
	char buf[MIDI_LINE_MAX];
	snprintf(buf, sizeof(buf), "%s\n", msg);
	
	/* Append */
	lv_textarea_add_text(ta_midi, buf);
	midi_line_count++;
}

#define MAX_MESSAGES_PER_TICK 3
void lvgl_thread(void)
{
	const struct device *display_dev;
	int ret;

	display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
	if (!device_is_ready(display_dev)) {
		LOG_ERR("Device not ready, aborting test");
		return;
	}

	initialize_gui();
	lv_timer_handler();
	ret = display_blanking_off(display_dev);
	if (ret < 0 && ret != -ENOSYS) {
		LOG_ERR("Failed to turn blanking off (error %d)", ret);
		return;
	}
	ui_add_line("...");
	initialize_gui();
	
	char line[MIDI_LINE_MAX];
	struct midi1_raw mid_raw;
	while (1) {
		int processed = 0;
		uint32_t sleep_ms = 0;

		/* Process at most N messages per iteration */
		while (processed < MAX_MESSAGES_PER_TICK &&
		       k_msgq_get(&midi_msgq, line, K_NO_WAIT) == 0) {
			ui_add_line(line);
			//ui_set_line(line);
			processed++;
		}
		sleep_ms = lv_timer_handler();
		k_msleep(sleep_ms);
		
		/* The raw midi stuff  that updates the bar */
		while (processed < MAX_MESSAGES_PER_TICK &&
			k_msgq_get(&midi_raw_msgq, &mid_raw, K_NO_WAIT) == 0) {
			ui_set_bar(mid_raw);
			ui_set_led_level(mid_raw.p1);
			processed++;
		}
		sleep_ms = lv_timer_handler();
		k_msleep(sleep_ms);
	}
	return;
}


/* For LVGL had to increase the stack size */
K_THREAD_DEFINE(lvgl_thread_tid, 8192, lvgl_thread, NULL, NULL, NULL, 5, 0, 0);

/* EOF */
