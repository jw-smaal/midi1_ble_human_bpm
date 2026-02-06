#ifndef MIDI1_BLOCKAVG_H
#define MIDI1_BLOCKAVG_H
/**
 * @file midi1_blockavg.h
 * @brief average the BPM measurement samples
 * @note used by the midi_clock_measure_counter.c and .h
 *
 * @author Jan-Willem Smaal <usenet@gispen.org>
 * @date 20260102
 * license SPDX-License-Identifier: Apache-2.0
 */
#include <stdint.h>

/**
 * @note over which amount of samples do we take the
 * measurement.
 */
#define MIDI1_BLOCKAVG_SIZE 48

struct midi1_blockavg {
	uint32_t buf[MIDI1_BLOCKAVG_SIZE];
	uint32_t sum;
	uint32_t index;
	uint32_t count;
};

/**
 * @brief init the block average system
 */
void midi1_blockavg_init(struct midi1_blockavg *blkavg);

/**
 * @brief add current sample
 */
void midi1_blockavg_add(struct midi1_blockavg *blkavg, uint32_t sample);

/**
 * @brief get average of current block
 * @return block average
 */
uint32_t midi1_blockavg_average(struct midi1_blockavg *blkavg);

/**
 * @brief getter for the current sample
 * @return count number the sample
 */
uint32_t midi1_blockavg_count(struct midi1_blockavg *blkavg);

#endif
/* EOF */
