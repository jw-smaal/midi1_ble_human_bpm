/**
 * LVGL LCD Graphical user interface
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

#include "midi1.h"
#include "common.h"
#include "model.h"


LOG_MODULE_REGISTER(lvgl_screen1, CONFIG_LOG_DEFAULT_LEVEL);
/*
 *  GLOBALS
 */
static lv_obj_t *label_title;
static lv_obj_t *label_bpm;
static lv_obj_t *label_pll;
static lv_obj_t *label_meas;
static lv_obj_t *ta_midi;

/* Strip chart */
static lv_obj_t *pll_chart;
static lv_chart_series_t *pll_ser;
static lv_chart_series_t *meas_ser;


/*
 *  GUI INITIALIZATION (480×320 landscape)
 */

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
	lv_obj_set_style_bg_color(lv_screen_active(),
	                          lv_color_white(), LV_PART_MAIN);
	lv_obj_set_style_bg_opa(lv_screen_active(), LV_OPA_COVER, LV_PART_MAIN);
	//lv_obj_set_style_text_color(lv_screen_active(),
	 //                           lv_color_black(), LV_PART_MAIN);
	lv_obj_set_style_text_font(lv_screen_active(),
	                           &lv_font_montserrat_14, LV_PART_MAIN);

	/* =====================================================
	 *  TOP BAR
	 * ===================================================== */
	/* Left: static title */
	label_title = lv_label_create(lv_screen_active());
	lv_label_set_text(label_title, "by J-W Smaal");
	lv_obj_align(label_title, LV_ALIGN_BOTTOM_RIGHT, 0, 0);

	/* Right: BPM in larger font */
	label_bpm = lv_label_create(lv_screen_active());
	lv_obj_set_style_text_font(label_bpm,
	                           &lv_font_montserrat_24, LV_PART_MAIN);
	//lv_obj_set_style_text_color(label_bpm,
	//			    lv_color_hex(0xA0ff00),   /* Blue */
	//			    LV_PART_MAIN);
	lv_label_set_text(label_bpm, "");
	lv_obj_align(label_bpm, LV_ALIGN_TOP_RIGHT, 0, 0);
	
	/* Left: PLL measurement on the left */
	label_pll = lv_label_create(lv_screen_active());
	lv_obj_set_style_text_font(label_pll,
				   &lv_font_montserrat_18, LV_PART_MAIN);
	//lv_obj_set_style_text_color(label_pll,
	//			    lv_color_hex(0x00ffA0),   /* green */
	//			    LV_PART_MAIN);
	lv_label_set_text(label_pll, "P");
	lv_obj_align(label_pll, LV_ALIGN_TOP_LEFT, 0, 0);
	
	/* Left: PLL measurement on the left */
	label_meas = lv_label_create(lv_screen_active());
	lv_obj_set_style_text_font(label_meas,
				   &lv_font_montserrat_18, LV_PART_MAIN);
	//lv_obj_set_style_text_color(label_meas,
	//			    lv_color_hex(0x00ff00),   /* green */
	//			    LV_PART_MAIN);
	lv_label_set_text(label_meas, "M");
	lv_obj_align(label_pll, LV_ALIGN_TOP_LEFT,0, 25);

	
	/* =====================================================
	 *  PLL LINE CHART (simple scrolling chart)
	 * ===================================================== */
	pll_chart = lv_chart_create(lv_screen_active());
	lv_obj_set_style_size(pll_chart, 0, 0, LV_PART_ITEMS);
	lv_obj_set_size(pll_chart, 390, 120);
	lv_obj_align(pll_chart, LV_ALIGN_BOTTOM_LEFT, 20, -20);
	
	lv_chart_set_type(pll_chart, LV_CHART_TYPE_LINE);
	/* history window */
	lv_chart_set_point_count(pll_chart, 100);
	lv_chart_set_update_mode(pll_chart, LV_CHART_UPDATE_MODE_SHIFT);
	/* PLL values range from 4000 --> 30000 */
	lv_chart_set_range(pll_chart,
			   LV_CHART_AXIS_PRIMARY_Y,
			   4000,
			   30000);
	pll_ser = lv_chart_add_series(pll_chart,
				      lv_palette_main(LV_PALETTE_BLUE),
				      LV_CHART_AXIS_PRIMARY_Y);
	meas_ser = lv_chart_add_series(pll_chart,
				       lv_palette_main(LV_PALETTE_RED),
				       LV_CHART_AXIS_PRIMARY_Y);
	
	/* =====================================================
	 *  CENTER: LARGE SCROLLABLE TEXT WINDOW
	 * ===================================================== */
	ta_midi = lv_textarea_create(lv_screen_active());
	lv_obj_set_style_text_font(ta_midi,
				   &lv_font_montserrat_12, LV_PART_MAIN);
	/* Size: nearly full screen minus top and bottom bar */
	lv_obj_set_size(ta_midi, 390, 120);
	lv_obj_align(ta_midi, LV_ALIGN_TOP_LEFT, 20, 60);
	/* No transparency */
	//lv_obj_set_style_bg_color(ta_midi, lv_color_black(), LV_PART_MAIN);
	lv_obj_set_style_bg_opa(ta_midi, LV_OPA_COVER, LV_PART_MAIN);
	/* Black text by default */
	lv_obj_set_style_text_color(ta_midi, lv_color_black(), LV_PART_MAIN);
	lv_textarea_set_text(ta_midi, "");
	lv_textarea_set_max_length(ta_midi, 4096);
	lv_textarea_set_cursor_click_pos(ta_midi, false);
	
}


#define MAX_MIDI_LINES 8
static int midi_line_count = 0;

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

/* We get this one from main */
extern uint8_t atom_bpm_get(void);

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
	//struct midi1_raw mid_raw;
	struct human_bpm_model mod;
	while (1) {
		int processed = 0;
		uint32_t sleep_ms = 0;
		char buf[MIDI_LINE_MAX];
		model_get(&mod);
		
		snprintf(buf, sizeof(buf), "BLE hr: %d BPM",
			 mod.hr_bpm);
		LOG_DBG("%s", buf);
		lv_label_set_text(label_bpm, buf);
		
		snprintf(buf, sizeof(buf), "Meas: %s",
			 sbpm_to_str(mod.meas_sbpm));
		LOG_DBG("%s", buf);
		lv_label_set_text(label_meas, buf);
		
		snprintf(buf, sizeof(buf), "PLL: %s",
			 sbpm_to_str(mod.pll_sbpm));
		LOG_DBG("%s", buf);
		lv_label_set_text(label_pll, buf);
		
		/* Add PLL value to chart */
		lv_chart_set_next_value(pll_chart, pll_ser, mod.pll_sbpm);
		lv_chart_set_next_value(pll_chart, meas_ser, mod.meas_sbpm);
		
		/* Process at most N messages per iteration */
		while (processed < MAX_MESSAGES_PER_TICK &&
		       k_msgq_get(&midi_msgq, line, K_NO_WAIT) == 0) {
			ui_add_line(line);
			//ui_set_line(line);
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
