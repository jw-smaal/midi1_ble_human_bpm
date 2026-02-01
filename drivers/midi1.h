/**
 * @file midi1.h
 * @brief MIDI1 helpers
 *
 * @note Functions to create UMP packets and do calculations
 * on MIDI timings.   Also convert to frequency and from frequency
 * to note.
 *
 * Created in 2014 for ATMEL AVR MCU's ported
 * to Zephyr RTOS in 2024 mainly for ARM based MCU's.
 *
 * @author Jan-Willem Smaal <usenet@gispen.org>
 * updated 20241224
 * @date 20252810  -> for Zephyr use with USB.
 *
 * license SPDX-License-Identifier: Apache-2.0
 */
#ifndef MIDI1_H
#define MIDI1_H
/*-----------------------------------------------------------------------*/
#include <stdint.h>
#include <string.h>
#include <zephyr/audio/midi.h>

/*-----------------------------------------------------------------------*/
/*  Define's specific for the MIDI protocol */
#define PITCHWHEEL_CENTER 8192
/* 16384 is the 2^14 which is the maximum pitch bend value */
#define PITCHWHEEL_MAX 16384

/* MIDI channel/mode masks */
#define CHANNEL_VOICE_MASK      0x80    /*  Bit 7 == 1 */
#define CHANNEL_MODE_MASK       0xB0
#define SYSTEM_EXCLUSIVE_MASK   0xF0
#define SYSTEM_REALTIME_MASK    0XF8
#define SYSTEM_COMMON_MASK      0XF0
#define MIDI_DATA               0x7F    /*  Bit 7 == 0 */

/* System messages */
enum midi_sysex {
	SYSTEM_EXCLUSIVE_START = 0xF0,
	SYSTEM_TUNE_REQUEST = 0xF6,
	SYSTEM_EXCLUSIVE_END = 0xF7
};

/* MIDI channel voice commands */
enum midi_channel_commands {
	C_NOTE_ON = 0x90,
	C_NOTE_OFF = 0x80,
	C_POLYPHONIC_AFTERTOUCH = 0xA0,
	C_CHANNEL_AFTERTOUCH = 0xD0,
	C_PITCH_WHEEL = 0xE0,
	C_CONTROL_CHANGE = 0xB0,
	C_PROGRAM_CHANGE = 0xC0
};

/* Channel mode messages defined here in decimals as the
 * spec also list them as such on page 20
 */
#define C_CHANNEL_MODE 0xB0
enum midi_channel_mode {
	C_MODE_ALL_SOUNDS_OFF = 120U,
	C_MODE_RESET_ALL_CONTROLLERS = 121U,
	C_MODE_LOCAL_CONTROl = 122U,
	C_MODE_ALL_NOTES_OFF = 123U,
	C_MODE_OMNI_OFF = 124U,
	C_MODE_OMNI_ON = 125U,
	C_MODE_MONO_ON = 126U,
	C_MODE_POLY_ON = 127U
};

enum midi_channel {
	CH1 = 0x00,
	CH2 = 0x01,
	CH3 = 0x02,
	CH4 = 0x03,
	CH5 = 0x04,
	CH6 = 0x05,
	CH7 = 0x06,
	CH8 = 0x07,
	CH9 = 0x08,
	CH10 = 0x09,
	CH11 = 0x0A,
	CH12 = 0x0B,
	CH13 = 0x0C,
	CH14 = 0x0D,
	CH15 = 0x0E,
	CH16 = 0x0F
};

enum midi_control_change {
	CTL_MSB_BANK = 0x00,
	CTL_MSB_MODWHEEL = 0x01,
	CTL_MSB_BREATH = 0x02,
	CTL_MSB_FOOT = 0x04,
	CTL_MSB_PORTAMENTO_TIME = 0x05,
	CTL_MSB_DATA_ENTRY = 0x06,
	CTL_MSB_MAIN_VOLUME = 0x07,
	CTL_MSB_BALANCE = 0x08,
	CTL_MSB_PAN = 0x0A,
	CTL_MSB_EXPRESSION = 0x0B,
	CTL_MSB_EFFECT1 = 0x0C,
	CTL_MSB_EFFECT2 = 0x0D,
	CTL_MSB_GENERAL_PURPOSE1 = 0x10,
	CTL_MSB_GENERAL_PURPOSE2 = 0x11,
	CTL_MSB_GENERAL_PURPOSE3 = 0x12,
	CTL_MSB_GENERAL_PURPOSE4 = 0x13,
	CTL_LSB_BANK = 0x20,
	CTL_LSB_MODWHEEL = 0x21,
	CTL_LSB_BREATH = 0x22,
	CTL_LSB_FOOT = 0x24,
	CTL_LSB_PORTAMENTO_TIME = 0x25,
	CTL_LSB_DATA_ENTRY = 0x26,
	CTL_LSB_MAIN_VOLUME = 0x27,
	CTL_LSB_BALANCE = 0x28,
	CTL_LSB_PAN = 0x2A,
	CTL_LSB_EXPRESSION = 0x2B,
	CTL_LSB_EFFECT1 = 0x2C,
	CTL_LSB_EFFECT2 = 0x2D,
	CTL_LSB_GENERAL_PURPOSE1 = 0x30,
	CTL_LSB_GENERAL_PURPOSE2 = 0x31,
	CTL_LSB_GENERAL_PURPOSE3 = 0x32,
	CTL_LSB_GENERAL_PURPOSE4 = 0x33,
	CTL_SUSTAIN = 0x40,
	CTL_PORTAMENTO = 0x41,
	CTL_SOSTENUTO = 0x42,
	CTL_SOFT_PEDAL = 0x43,
	CTL_LEGATO_FOOTSWITCH = 0x44,
	CTL_HOLD2 = 0x45,
	CTL_SC1_SOUND_VARIATION = 0x46,
	CTL_SC2_TIMBRE = 0x47,
	CTL_SC3_RELEASE_TIME = 0x48,
	CTL_SC4_ATTACK_TIME = 0x49,
	CTL_SC5_BRIGHTNESS = 0x4A,
	CTL_SC6 = 0x4B,
	CTL_SC7 = 0x4C,
	CTL_SC8 = 0x4D,
	CTL_SC9 = 0x4E,
	CTL_SC10 = 0x4F,
	CTL_GENERAL_PURPOSE5 = 0x50,
	CTL_GENERAL_PURPOSE6 = 0x51,
	CTL_GENERAL_PURPOSE7 = 0x52,
	CTL_GENERAL_PURPOSE8 = 0x53,
	CTL_PORTAMENTO_CONTROL = 0x54,
	CTL_E1_REVERB_DEPTH = 0x5B,
	CTL_E2_TREMOLO_DEPTH = 0x5C,
	CTL_E3_CHORUS_DEPTH = 0x5D,
	CTL_E4_DETUNE_DEPTH = 0x5E,
	CTL_E5_PHASER_DEPTH = 0x5F,
	CTL_DATA_INCREMENT = 0x60,
	CTL_DATA_DECREMENT = 0x61,
	CTL_NRPN_LSB = 0x62,
	CTL_NRPN_MSB = 0x63,
	CTL_RPN_LSB = 0x64,
	CTL_RPN_MSB = 0x65,
	CTL_ALL_SOUNDS_OFF = 0x78,
	CTL_RESET_CONTROLLERS = 0x79,
	CTL_LOCAL_CONTROL_SWITCH = 0x7A,
	CTL_ALL_NOTES_OFF = 0x7B,
	CTL_OMNI_OFF = 0x7C,
	CTL_OMNI_ON = 0x7D,
	CTL_MONO1 = 0x7E,
	CTL_MONO2 = 0x7F
};

/* System Real Time commands */
enum midi_real_time {
	RT_TIMING_CLOCK = 0xF8,
	RT_START = 0xFA,
	RT_CONTINUE = 0xFB,
	RT_STOP = 0xFC,
	RT_ACTIVE_SENSING = 0xFE,
	RT_RESET = 0xFF
};

/*
 * TODO: For now assume UMP channel group = 0 when using MIDI1.0
 */
#define UMP_CHANNEL_GROUP 0

/**
 * -- == Channel messages == --
 */
/**
 * @brief create a MIDI1.0 UMP message for a NOTE_ON
 * @param channel MIDI channel 1 = 0 you may also use 'CH1', 'CH16' etc..
 * @param key MIDI key
 * @param velocity NOTE ON velocity
 * @return midi_ump universal MIDI1.0 packet
 */
struct midi_ump midi1_note_on(uint8_t channel, uint8_t key, uint8_t velocity);

/**
 * @brief create a MIDI1.0 UMP message for a NOTE_OFF
 * @param channel MIDI channel 1 = 0 you may also use 'CH1', 'CH16' etc..
 * @param key MIDI key
 * @param velocity NOTE OFF velocity
 * @return midi_ump universal MIDI1.0 packet
 */
struct midi_ump midi1_note_off(uint8_t channel, uint8_t key, uint8_t velocity);

/**
 * @brief create a MIDI1.0 UMP message for a CONTROL CHANGE
 * @param channel MIDI channel 1 = 0 you may also use 'CH1', 'CH16' etc..
 * @param controller MIDI controller number
 * @param val controller value
 * @return midi_ump universal MIDI1.0 packet
 */
struct midi_ump midi1_controlchange(uint8_t channel,
                                    uint8_t controller, uint8_t val);

/**
 * @brief create a MIDI1.0 UMP message for a PITCH WHEEL
 * @param channel MIDI channel 1 = 0 you may also use 'CH1', 'CH16' etc..
 * @param val controller value as a 14 bit value.
 * @return midi_ump universal MIDI1.0 packet
 */
struct midi_ump midi1_pitchwheel(uint8_t channel, uint16_t val);

/**
 * @brief create a MIDI1.0 UMP message for a MODULATION WHEEL
 * @param channel MIDI channel 1 = 0 you may also use 'CH1', 'CH16' etc..
 * @param val controller value as a 14 bit value.
 * @return midi_ump universal MIDI1.0 packet
 */
struct midi_ump midi1_modwheel(uint8_t channel, uint8_t val);

/**
 * @brief create a MIDI1.0 UMP message for a POLYPHONIC AFTERTOUCH
 * @param channel MIDI channel 1 = 0 you may also use 'CH1', 'CH16' etc..
 * @param key MIDI key
 * @param val controller value as a 7 bit value.
 * @return midi_ump universal MIDI1.0 packet
 */
struct midi_ump midi1_polyaftertouch(uint8_t channel, uint8_t key, uint8_t val);

/**
 * @brief create a MIDI1.0 UMP message for a CHANNEL AFTERTOUCH
 * @param channel MIDI channel 1 = 0 you may also use 'CH1', 'CH16' etc..
 * @param val controller value as a 7 bit value.
 * @return midi_ump universal MIDI1.0 packet
 */
struct midi_ump midi1_channelaftertouch(uint8_t channel, uint8_t val);

/**
 * -- == System realtime messages == --
 */

/**
 * @brief create MIDI timing clock Universal MIDI packet (UMP)
 * @return midi_ump universal MIDI1.0 packet
 */
struct midi_ump midi1_timing_clock(void);

/**
 * @brief create MIDI realtime START Universal MIDI packet (UMP)
 * @return midi_ump universal MIDI1.0 packet
 */
struct midi_ump midi1_start(void);

/**
 * @brief create MIDI realtime CONTINUE Universal MIDI packet (UMP)
 * @return midi_ump universal MIDI1.0 packet
 */
struct midi_ump midi1_continue(void);

/**
 * @brief create MIDI realtime STOP Universal MIDI packet (UMP)
 * @return midi_ump universal MIDI1.0 packet
 */
struct midi_ump midi1_stop(void);

/**
 * @brief create MIDI realtime ACTIVE SENSING Universal MIDI packet (UMP)
 * @return midi_ump universal MIDI1.0 packet
 */
struct midi_ump midi1_active_sensing(void);

/**
 * @brief create MIDI realtime RESET Universal MIDI packet (UMP)
 * @return midi_ump universal MIDI1.0 packet
 */
struct midi_ump midi1_reset(void);

/*
 *------------------------------------------------------------------------------
 * MIDI tempo helpers.
 *
 * upscaled implementation (s)bpm given
 * 1.00 bpm is 100
 * 123.10 bpm is 123100 max 65535 == 655.35 bpm
 * period returned is in microseconds as a uint32_t
 * 0.003814755 3.814 ms --> 3814 us (655,35 bpm)
 * 2.500000000 s/(1/24 qn) --> 2500000 us
 * i.e. multiplied by 1000000
 * Done so we can run it on a ARM M0+ without a FPU and the need
 * to compile in single precision math.
 *
 */
#define BPM_SCALE      100u
#define US_PER_SECOND  1000000u

/**
 * @brief returns the interval in microseconds (us) for a given sbpm
 * @param  sbpm   Scaled BPM value (e.g. 12000 for 120.00 BPM)
 * @return interval in microseconds (us)
 */
uint32_t sbpm_to_us_interval(uint16_t sbpm);

/**
 * @brief returns the interval in clock ticks for a given sbpm
 * @param sbpm   Scaled BPM value (e.g. 12000 for 120.00 BPM)
 * @param clock_hz clock speed of the current processor
 * @return interval clock ticks
 */
uint32_t sbpm_to_ticks(uint16_t sbpm, uint32_t clock_hz);

/**
 * @brief Convert a measured interval in microseconds to scaled BPM (sbpm).
 *
 * @param interval  Interval duration in microseconds (us)
 * @return Scaled BPM value (e.g. 12000 for 120.00 BPM)
 */
uint16_t us_interval_to_sbpm(uint32_t interval);

/**
 * @brief Convert a measured interval in microseconds to 24‑PPQN period units.
 *
 * @param interval  Interval duration in microseconds (us)
 * @return 24‑PPQN period value corresponding to the interval
 */
uint32_t us_interval_to_24pqn(uint32_t interval);

/**
 * @brief Convert a 24‑PPQN period value to an interval in microseconds.
 *
 * @param pqn24  24‑PPQN period value
 * @return Interval duration in microseconds (us)
 */
uint32_t pqn24_to_us_interval(uint32_t pqn24);

/**
 * @brief Convert scaled BPM (sbpm) to a 24‑PPQN period value.
 *
 * @param sbpm  Scaled BPM value (e.g. 12000 for 120.00 BPM)
 * @return 24‑PPQN period value corresponding to the BPM
 */
uint32_t sbpm_to_24pqn(uint16_t sbpm);

/**
 * @brief convert pulses per quater note to scaled bpm
 *
 * @param 24‑PPQN period value
 * @return pqn24 pulses per quater note in us
 */
uint16_t pqn24_to_sbpm(uint32_t pqn24);

/**
 * @brief  Returns static string with the BPM formattted like 123.45
 *
 * @param  sbpm   Scaled BPM value (e.g. 12000 for 120.00 BPM)
 * @return Pointer to a static buffer containing the formatted string "xxx.yy"
 */
const char *sbpm_to_str(uint16_t sbpm);

/* -------------------------------------------------------------------------- */
#endif
/* EOF */
