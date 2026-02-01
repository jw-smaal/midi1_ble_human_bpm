/**
 * @file midi_freq_table.h
 * Precomputed frequencies for MIDI notes 0–127
 * used for embedded systems.  Based op A4 = 440Hz.
 * used by "note.h  / note.c" for the noteToFreq() function 
 *
 * @author Jan-Willem Smaal <usenet@gispen.org>
 * license SPDX-License-Identifier: Apache-2.0
 */
#ifndef MIDI_FREQ_TABLE_H
#define MIDI_FREQ_TABLE_H

#include <stdint.h>

/* Base frequency for A4 (MIDI note 69) */
#define BASE_A4_NOTE_FREQ 440.0f

/* Precomputed frequencies for MIDI notes 0–127 (C-1 to G9) */
static const float midi_freq_table[128] = {
#include "midi_freq_table.inc"
};

#endif                          /* MIDI_FREQ_TABLE_H */
