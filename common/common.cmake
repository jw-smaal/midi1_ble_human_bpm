# Copyright (c) 2023 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0
set(SAMPLE_USBD_DIR common)

target_include_directories(app PRIVATE ${SAMPLE_USBD_DIR})
target_sources_ifdef(CONFIG_USB_DEVICE_STACK_NEXT app PRIVATE
  ${SAMPLE_USBD_DIR}/sample_usbd_init.c
)
