#include <zephyr/ztest.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <midi1_serial.h>

#define MIDI_DEV DT_NODELABEL(midi0)
static const struct device *midi_dev = DEVICE_DT_GET(MIDI_DEV);

ZTEST_USER(midi_driver_tests, test_baseline)
{
	zassert_true(device_is_ready(midi_dev), "MIDI device not ready");
}

static void *midi_driver_tests_setup(void)
{
	return NULL;
}

ZTEST_SUITE(midi_driver_tests, NULL, midi_driver_tests_setup, NULL, NULL, NULL);
