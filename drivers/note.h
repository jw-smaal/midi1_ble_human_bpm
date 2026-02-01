/**
 * @file note.h 
 * 
 * @brief generic MIDI and harmony related functions 
 * implemented in support of embedded systems in c. 
 * tested on Zephyr RTOS.
 * TODO: update from C++ coding style back to C.
 * TODO: e.g. 'noteToText' should be renamed to 'note_to_text'
 *
 * @author Jan-Willem Smaal <usenet@gispen.org> 
 * @updated 20241224
 * @license SPDX-License-Identifier: Apache-2.0
 */
#ifndef NOTE_H
#define NOTE_H
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

/*
 * Pre computed values for the note frequencies
 */
#include "midi_freq_table.h"

/* 440 Hz for the A4 note */
#define BASE_A4_NOTE_FREQUENCY 440

/**
 * @brief converts a MIDI note using a lookup table to a string
 *
 * @param @midinote in MIDI format (limited to 0 -> 127)
 * @param flats represent not in flats or sharps (default)
 * @return char note in text format e.g. Db
 */
const char *noteToText(uint8_t midinote, bool flats);

/**
 * @brief convert note to text with octave
 * @param midinote in MIDI format (limited to 0 -> 127)
 * @return char note in text format e.g. Db3
 */
const char *noteToTextWithOctave(uint8_t midinote, bool flats);

/**
 * @brief note to octave
 * @param midinote in MIDI format (limited to 0 -> 127)
 * @return octave e.g. -3 or 4
 */
int noteToOct(uint8_t midinote);

#if TODO_USING_MATH
float noteToFreq(uint8_t midinote, int base_a4_note_freq);
#endif

/**
 * @brief converts note to frequency using lookup table
 * @param midinote in MIDI format (limited to 0 -> 127)
 * @return frequency referenceded to A440
 */
float noteToFreq(uint8_t midinote);

/**
 * @brief converts frequency to MIDI note approximate
 * @param frequency referenceded to A440
 * @return midinote in MIDI format (limited to 0 -> 127)
 */
uint8_t freqToMidiNote(float freq);

#endif                          /* NOTE_H */
/* EOF */
