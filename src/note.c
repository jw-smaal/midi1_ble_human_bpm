/** 
 * @file note.c 
 * 
 * @brief Generic MIDI and harmony related functions 
 * implemented in support of embedded systems in c. 
 * tested on Zephyr RTOS. 
 * 
 * @author Jan-Willem Smaal <usenet@gispen.org> 
 * @updated 20241224
 * @license SPDX-License-Identifier: Apache-2.0
 */
#include "note.h"

/*
 * Return the note with the octave included.
 * not thread safe. 
 */
const char *noteToTextWithOctave(uint8_t midinote, bool flats)
{
	static char notestring[5];
	snprintf(notestring,
	         sizeof(notestring),
	         "%s%d", noteToText(midinote, flats), noteToOct(midinote));
	return &notestring[0];
}

/* This function converts a MIDI note using a lookup table to a string */
const char *noteToText(uint8_t midinote, bool flats)
{
	int octave = noteToOct(midinote);
	uint8_t note = midinote - ((octave + 2) * 12);
	static const char *flatNotes[] =
	    { "C ", "Db", "D ", "Eb", "E ", "F ", "Gb", "G ", "Ab", "A ", "Bb",
		"B "
	};
	static const char *sharpNotes[] =
	    { "C ", "C#", "D ", "D#", "E ", "F ", "F#", "G ", "G#", "A ", "A#",
		"B "
	};

	const char *noteString;
	if (flats) {
		noteString = flatNotes[note];
	} else {
		noteString = sharpNotes[note];
	}
	/*  Null-terminated string is returned from the static char array */
	return noteString;
}

/*
 * This function converts a MIDI note to an octave number 
 */
int noteToOct(uint8_t midinote)
{
	return (midinote / 12) - 2;
}

#if TODO_USING_MATH
/*
 * This function converts a MIDI note to a frequency
 * it can use a custom pitch for the A4 note.
 * TODO: implement in MCU friendly way....
 */
float noteToFreqCustomA4(uint8_t midinote, int base_a4_note_freq)
{
	return base_a4_note_freq * pow(2, (midinote - 69) / 12.0);
}
#endif

/**
 * In this case on my embedded system I don't want to run the
 * pow() match functions I use a precomputed lookup table.
 * A=440Hz is a given then.
 * TODO: possibly create multiple tables with some
 * TODO: variation in pitch of A4.  Flash size tradeoff.   
 */
#include "midi_freq_table.h"
float noteToFreq(uint8_t midinote)
{
	return midi_freq_table[midinote];
}

#if TODO_USING_MATH
uint8_t freqToMidiNote(float freq)
{
	/* Fast inverse using log2f */
	float n = 69.0f + 12.0f * log2f(freq / BASE_A4_NOTE_FREQ);
	if (n < 0)
		n = 0;
	if (n > 127)
		n = 127;
	return (uint8_t) (n + 0.5f);
}
#endif

/**
 * @brief Convert a frequency value to the nearest MIDI note number.
 *
 * This function uses a binary search over the precomputed MIDI note
 * frequency table (0–127).
 *
 * @param freq  Input frequency in Hz
 * @return      MIDI note number (0–127) that best matches the frequency
 *
 * TODO update for MIDI 2.0 to include per note offsets.
 */
uint8_t freqToMidiNote(float freq)
{
	int min = 0;
	int max = 127;

	while (min <= max) {
		int mid = (min + max) / 2;

		if (freq > midi_freq_table[mid]) {
			min = mid + 1;
		} else if (freq < midi_freq_table[mid]) {
			max = mid - 1;
		} else {
			/* exact match */
			return mid;
		}
	}

	/*  min is now the first note with freq_table[min] > freq */
	if (min == 0) {
		return 0;
	}
	if (min > 127) {
		return 127;
	}

	/* pick closest of min and min-1 */
	float low = midi_freq_table[min - 1];
	float high = midi_freq_table[min];

	return (freq - low < high - freq) ? (min - 1) : min;
}

/* EOF */
