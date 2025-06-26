/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/types.h>

/**
 * @brief Get the update interval from a CBOR buffer.
 *
 * @param[in]  cbor         The CBOR buffer.
 * @param[in]  len          The length of the CBOR buffer.
 * @param[out] interval_sec The update interval in seconds.
 *
 * @returns 0 If the operation was successful.
 *	    Otherwise, a (negative) error code is returned.
 * @retval -EFAULT if the CBOR buffer is invalid.
 *
 */
int get_led_command_from_cbor_response(const uint8_t *cbor,
                                       size_t len,
                                       void *led_msg);

int get_update_interval_from_cbor_response(const uint8_t *cbor,
					   size_t len,
					   uint32_t *interval_sec);

/**
 * @brief Log CBOR data structure in readable format.
 *
 * @param[in]  cbor      The CBOR buffer.
 * @param[in]  cbor_len  The length of the CBOR buffer.
 */
void log_cbor_data(const uint8_t *cbor, size_t cbor_len);
