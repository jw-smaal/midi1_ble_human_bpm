/**
 * @file midi1_blockavg.c
 * @brief average the BPM measurement samples
 * @note used by the midi_clock_measure_counter.c and .h
 *
 * @author Jan-Willem Smaal <usenet@gispen.org>
 * @date 20260102
 * license SPDX-License-Identifier: Apache-2.0
 */
#include <stdint.h>
#include "midi1_blockavg.h"

void midi1_blockavg_init(struct midi1_blockavg *blkavg)
{
	/* Make sure the memory is empty when starting */
	for (uint32_t i = 0; i < MIDI1_BLOCKAVG_SIZE; i++) {
		blkavg->buf[i] = 0;
	}
	blkavg->sum = 0;
	blkavg->index = 0;
	blkavg->count = 0;
}

void midi1_blockavg_add(struct midi1_blockavg *blkavg, uint32_t sample)
{
	if (blkavg->count < MIDI1_BLOCKAVG_SIZE) {
		/* Still filling the buffer */
		blkavg->buf[blkavg->count] = sample;
		blkavg->sum += sample;
		blkavg->count++;
	} else {
		/* Buffer full: overwrite oldest */
		blkavg->sum -= blkavg->buf[blkavg->index];
		blkavg->buf[blkavg->index] = sample;
		blkavg->sum += sample;

		blkavg->index++;
		if (blkavg->index >= MIDI1_BLOCKAVG_SIZE) {
			blkavg->index = 0;
		}
	}
}

uint32_t midi1_blockavg_average(struct midi1_blockavg *blkavg)
{
	if (blkavg->count == 0) {
		return 0;
	}
	return blkavg->sum / blkavg->count;
}

uint32_t midi1_blockavg_count(struct midi1_blockavg *blkavg)
{
	return blkavg->count;
}

/* EOF */
