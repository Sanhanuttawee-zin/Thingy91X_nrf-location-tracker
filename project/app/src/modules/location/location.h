/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _LOCATION_H_
#define _LOCATION_H_

#include <zephyr/kernel.h>
#include <zephyr/zbus/zbus.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Channels provided by this module */
ZBUS_CHAN_DECLARE(
	LOCATION_CHAN
);

enum location_msg_type {
	LOCATION_SEARCH_STARTED = 0x1,
	LOCATION_SEARCH_DONE,
	LOCATION_SEARCH_TRIGGER,
};

#define MSG_TO_LOCATION_TYPE(_msg)	(*(const enum location_msg_type *)_msg)

#ifdef __cplusplus
}
#endif

#endif /* _LOCATION_H_ */
