/**
 * @brief MIDI 1.0 human clock
 *
 * @author Jan-Willem Smaal <usenet@gispen.org>
 * @date 20260107
 *
 * license SPDX-License-Identifier: Apache-2.0
 */

/*
 * Bluetooth hr central example used is:
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stddef.h>
#include <errno.h>
#include <lvgl.h>
#include <string.h>
#include <zephyr/types.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/display.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/printk.h>

/*
 * This is part of the MIDI2 library prj.conf
 * CONFIG_MIDI2_UMP_STREAM_RESPONDER=y
 * /zephyr/lib/midi2/ump_stream_responder.h
 * it gets linked in and is required for the USB MIDI support.
 */
#include <sample_usbd.h>
#include <zephyr/usb/class/usbd_midi2.h>
#include <ump_stream_responder.h>

/* Moved to ../drivers */
#include "midi1_serial.h"
#include "midi1_clock_cntr.h"
#include "midi1_clock_meas_cntr.h"
#include "midi1_blockavg.h"

/* Some MIDI1 helpers that are not drivers */
#include "midi1_pll.h"
#include "note.h"

/* My application logic */
#include "common.h"
#include "model.h"

LOG_MODULE_REGISTER(midi1_human_clock, CONFIG_LOG_DEFAULT_LEVEL);

#define USB_MIDI_DT_NODE DT_NODELABEL(usb_midi)
static const struct device *const midi_usb = DEVICE_DT_GET(USB_MIDI_DT_NODE);

static void start_scan(void);
static struct bt_conn *default_conn;
static struct bt_uuid_16 discover_uuid = BT_UUID_INIT_16(0);
static struct bt_gatt_discover_params discover_params;
static struct bt_gatt_subscribe_params subscribe_params;
uint64_t total_rx_count;        /* This value is exposed to test code */

/* Global BPM set by the bluetooth HR service */
static uint8_t g_bpm = 0;
static atomic_t atom_bpm = ATOMIC_INIT(0);

uint8_t atom_bpm_get(void)
{
        return (uint8_t) atomic_get(&atom_bpm);
}

static uint8_t notify_func(struct bt_conn *conn,
                           struct bt_gatt_subscribe_params *params,
                           const void *data, uint16_t length)
{
        if (!data) {
                LOG_ERR("[UNSUBSCRIBED]");
                params->value_handle = 0U;
                return BT_GATT_ITER_STOP;
        }

        const uint8_t *d = data;

        /* HRM format: d[0] = flags, d[1] = BPM */
        if (length >= 2) {
		uint8_t flags = d[0];
                uint8_t bpm = d[1];

                LOG_INF("HR Notification: BPM=%u flags=0x%02x len=%u",
                        bpm, flags, length);
                g_bpm = bpm;
                atomic_set(&atom_bpm, bpm);
        }
        total_rx_count++;

        return BT_GATT_ITER_CONTINUE;
}

static uint8_t discover_func(struct bt_conn *conn,
                             const struct bt_gatt_attr *attr,
                             struct bt_gatt_discover_params *params)
{
        int err;

        if (!attr) {
                LOG_INF("Discover complete");
                (void)memset(params, 0, sizeof(*params));
                return BT_GATT_ITER_STOP;
        }

        LOG_INF("[ATTRIBUTE] handle %u", attr->handle);

        if (!bt_uuid_cmp(discover_params.uuid, BT_UUID_HRS)) {
                memcpy(&discover_uuid, BT_UUID_HRS_MEASUREMENT,
                       sizeof(discover_uuid));
                discover_params.uuid = &discover_uuid.uuid;
                discover_params.start_handle = attr->handle + 1;
                discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;

                err = bt_gatt_discover(conn, &discover_params);
                if (err) {
                        LOG_ERR("Discover failed (err %d)", err);
                }
        } else if (!bt_uuid_cmp(discover_params.uuid, BT_UUID_HRS_MEASUREMENT)) {
                memcpy(&discover_uuid, BT_UUID_GATT_CCC, sizeof(discover_uuid));
                discover_params.uuid = &discover_uuid.uuid;
                discover_params.start_handle = attr->handle + 2;
                discover_params.type = BT_GATT_DISCOVER_DESCRIPTOR;
                subscribe_params.value_handle = bt_gatt_attr_value_handle(attr);

                err = bt_gatt_discover(conn, &discover_params);
                if (err) {
                        LOG_ERR("Discover failed (err %d)", err);
                }
        } else {
                subscribe_params.notify = notify_func;
                subscribe_params.value = BT_GATT_CCC_NOTIFY;
                subscribe_params.ccc_handle = attr->handle;

                err = bt_gatt_subscribe(conn, &subscribe_params);
                if (err && err != -EALREADY) {
                        LOG_ERR("Subscribe failed (err %d)", err);
                } else {
                        LOG_INF("[SUBSCRIBED]");
                }

                return BT_GATT_ITER_STOP;
        }

        return BT_GATT_ITER_STOP;
}

static bool eir_found(struct bt_data *data, void *user_data)
{
        bt_addr_le_t *addr = user_data;
        int i;

        LOG_INF("[AD]: %u data_len %u", data->type, data->data_len);

        switch (data->type) {
        case BT_DATA_UUID16_SOME:
        case BT_DATA_UUID16_ALL:
                if (data->data_len % sizeof(uint16_t) != 0U) {
                        LOG_INF("AD malformed");
                        return true;
                }

                for (i = 0; i < data->data_len; i += sizeof(uint16_t)) {
                        struct bt_conn_le_create_param *create_param;
                        struct bt_le_conn_param *param;
                        const struct bt_uuid *uuid;
                        uint16_t u16;
                        int err;

                        memcpy(&u16, &data->data[i], sizeof(u16));
                        uuid = BT_UUID_DECLARE_16(sys_le16_to_cpu(u16));
                        if (bt_uuid_cmp(uuid, BT_UUID_HRS)) {
                                continue;
                        }

                        err = bt_le_scan_stop();
                        if (err) {
                                LOG_ERR("Stop LE scan failed (err %d)", err);
                                continue;
                        }

                        LOG_INF("Creating connection with Coded PHY support");
                        param = BT_LE_CONN_PARAM_DEFAULT;
                        create_param = BT_CONN_LE_CREATE_CONN;
                        create_param->options |= BT_CONN_LE_OPT_CODED;
                        err = bt_conn_le_create(addr, create_param, param,
                                                &default_conn);
                        if (err) {
                                LOG_INF
                                    ("Coded PHY connection failed (err %d), trying non-Coded",
                                     err);

                                create_param->options &= ~BT_CONN_LE_OPT_CODED;
                                err = bt_conn_le_create(addr, create_param,
                                                        param, &default_conn);
                                if (err) {
                                        LOG_ERR
                                            ("Create connection failed (err %d)",
                                             err);
                                        start_scan();
                                }
                        }

                        return false;
                }
        }

        return true;
}

static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
                         struct net_buf_simple *ad)
{
        char dev[BT_ADDR_LE_STR_LEN];

        bt_addr_le_to_str(addr, dev, sizeof(dev));
        LOG_INF("[DEVICE]: %s, AD evt type %u, AD data len %u, RSSI %i",
                dev, type, ad->len, rssi);

        /* We're only interested in legacy connectable events or
         * possible extended advertising that are connectable.
         */
        if (type == BT_GAP_ADV_TYPE_ADV_IND ||
            type == BT_GAP_ADV_TYPE_ADV_DIRECT_IND ||
            type == BT_GAP_ADV_TYPE_EXT_ADV) {
                bt_data_parse(ad, eir_found, (void *)addr);
        }
}

static void start_scan(void)
{
        int err;

        /* Use active scanning and disable duplicate filtering to handle any
         * devices that might update their advertising data at runtime. */
        struct bt_le_scan_param scan_param = {
                .type = BT_LE_SCAN_TYPE_ACTIVE,
                .options = BT_LE_SCAN_OPT_CODED,
                .interval = BT_GAP_SCAN_FAST_INTERVAL,
                .window = BT_GAP_SCAN_FAST_WINDOW,
        };

        err = bt_le_scan_start(&scan_param, device_found);
        if (err) {
                LOG_INF
                    ("Scanning with Coded PHY failed (err %d), trying without",
                     err);

                scan_param.options &= ~BT_LE_SCAN_OPT_CODED;
                err = bt_le_scan_start(&scan_param, device_found);
                if (err) {
                        LOG_ERR("Scanning failed to start (err %d)", err);
                        return;
                }
        }

        LOG_INF("Scanning successfully started");
}

static void connected(struct bt_conn *conn, uint8_t conn_err)
{
        char addr[BT_ADDR_LE_STR_LEN];
        int err;

        bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

        if (conn_err) {
                LOG_ERR("Failed to connect to %s (%u)", addr, conn_err);

                bt_conn_unref(default_conn);
                default_conn = NULL;

                start_scan();
                return;
        }

        LOG_INF("Connected: %s", addr);

        total_rx_count = 0U;

        if (conn == default_conn) {
                memcpy(&discover_uuid, BT_UUID_HRS, sizeof(discover_uuid));
                discover_params.uuid = &discover_uuid.uuid;
                discover_params.func = discover_func;
                discover_params.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
                discover_params.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
                discover_params.type = BT_GATT_DISCOVER_PRIMARY;

                err = bt_gatt_discover(default_conn, &discover_params);
                if (err) {
                        LOG_ERR("Discover failed (err %d)", err);
                        return;
                }
        }
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
        char addr[BT_ADDR_LE_STR_LEN];

        bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

        LOG_INF("Disconnected: %s, reason 0x%02x %s", addr, reason,
                bt_hci_err_to_str(reason));

        if (default_conn != conn) {
                return;
        }

        bt_conn_unref(default_conn);
        default_conn = NULL;

        start_scan();
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
        .connected = connected,
        .disconnected = disconnected,
};

static void on_ump_packet(const struct device *dev, const struct midi_ump ump)
{
        if (UMP_MT(ump) == UMP_MT_SYS_RT_COMMON) {
                if (UMP_MIDI_STATUS(ump) == RT_TIMING_CLOCK) {
                        /* MIDI Clock handled elsewhere */
                }
        }
}

/**
 * @brief Light up the LED (if any) when USB-MIDI2.0 is active towards the PC
 */
static void on_device_ready(const struct device *dev, const bool ready)
{
        LOG_INF("MIDI USBD device ready!");
}

/* rx callback struct for the clock tests we use 'on_ump_packet' */
static const struct usbd_midi_ops ump_ops = {
        .rx_packet_cb = on_ump_packet,
        .ready_cb = on_device_ready,
};

/*
 * This get's called every 24PQN from the driver.
 * because it's timing sensitive call anything that runs for
 * a longer time in a workqueue.
 */
void midi1_clock_cntr_callback(void)
{
        static uint8_t i;

        /* USBD MIDI */
        if (midi_usb) {
                usbd_midi_send(midi_usb, midi1_timing_clock());
        }

        /* 0 --> 23 = 24 pulses */
        if (i < 23) {
                i++;
        } else {
                LOG_DBG("[P]");
                i = 0;
        }
}

int main(void)
{
        int err;

        /* Initialize Bluetooth */
        err = bt_enable(NULL);
        if (err) {
                LOG_ERR("Bluetooth init failed (err %d)", err);
                return err;
        }

        LOG_INF("Bluetooth initialized");
        start_scan();

        /* Initialize Serial MIDI1 */
        const struct device *midi_serial = DEVICE_DT_GET(DT_NODELABEL(midi0));
        if (!device_is_ready(midi_serial)) {
                LOG_ERR("Serial MIDI1 device not ready");
                return -ENODEV;
        }

        const struct midi1_serial_api *mid_api = midi_serial->api;

        LOG_INF("MIDI1 sending initial note...");
        mid_api->note_on(midi_serial, CH16, 1, 60);
        k_sleep(K_MSEC(290));
        mid_api->note_off(midi_serial, CH16, 1, 60);
        k_sleep(K_MSEC(290));

        /* Initialize MIDI clock driver */
        const struct device *clk =
            DEVICE_DT_GET(DT_NODELABEL(midi1_clock_cntr));
        if (!device_is_ready(clk)) {
                LOG_ERR("MIDI1 clock counter device not ready");
                return -ENODEV;
        }

        const struct midi1_clock_cntr_api *mid_clk = clk->api;
        mid_clk->gen_sbpm(clk, 12000);  /* Initial 120.00 BPM */
        mid_clk->register_callback(clk, midi1_clock_cntr_callback);

        /* Initialize MIDI clock measurement driver */
        const struct device *meas =
            DEVICE_DT_GET(DT_NODELABEL(midi1_clock_meas_cntr));
        if (!device_is_ready(meas)) {
                LOG_ERR("MIDI1 clock measurement device not ready");
                return -ENODEV;
        }

        const struct midi1_clock_meas_cntr_api *mid_meas = meas->api;

        /* Initialize USBD MIDI2.0 */
        if (midi_usb && device_is_ready(midi_usb)) {
                struct usbd_context *sample_usbd;

                usbd_midi_set_ops(midi_usb, &ump_ops);
                sample_usbd = sample_usbd_init_device(NULL);
                if (sample_usbd == NULL) {
                        LOG_ERR("Failed to initialize USB device");
                } else {
                        err = usbd_enable(sample_usbd);
                        if (err) {
                                LOG_ERR("Failed to enable USBD (err %d)", err);
                        } else {
                                LOG_INF("USB device support enabled");
                        }
                }
        }

        /* My application model */
        model_init();

        while (1) {
                uint16_t gen_sbpm = (uint16_t) atom_bpm_get() * 100U;

                LOG_DBG("Measured incoming SBPM %d, Target %d",
                        mid_meas->get_sbpm(meas), gen_sbpm);

                if (gen_sbpm > 0) {
                        mid_clk->gen_sbpm(clk, gen_sbpm);
                }

                model_set(true, atom_bpm_get(), mid_meas->get_sbpm(meas), 0, 0);

                k_sleep(K_MSEC(4000));

#define MIDI_TEST_PATTERN 1
#if MIDI_TEST_PATTERN
                /* Test Pattern Logic */
                for (uint8_t value = 0; value < 16; value++) {
                        mid_api->control_change(midi_serial, CH16, 1, value);
                        k_sleep(K_MSEC(290));
                }
                for (uint8_t value = 60; value < 66; value++) {
                        mid_api->note_on(midi_serial, CH7, value, 100);
                        k_sleep(K_MSEC(310));
                }
                for (uint8_t value = 60; value < 66; value++) {
                        mid_api->note_off(midi_serial, CH7, value, 100);
                }
#endif
        }

        return 0;
}
