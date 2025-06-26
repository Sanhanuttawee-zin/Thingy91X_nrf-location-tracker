/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <errno.h>
#include <zephyr/zbus/zbus.h>
#include <zephyr/logging/log.h>
#include "device_shadow_decode.h"
#include "modules/led/led.h"
#include <zcbor_decode.h>
#include <zcbor_common.h>

LOG_MODULE_REGISTER(cbor_helper, CONFIG_APP_LOG_LEVEL);

void log_cbor_data(const uint8_t *cbor, size_t cbor_len)
{
    zcbor_state_t state[3];
    zcbor_new_state(state, ARRAY_SIZE(state), cbor, cbor_len, 1, NULL, 0);
    
    LOG_INF("=== CBOR Data Structure ===");
    
    if (zcbor_map_start_decode(state)) {
        struct zcbor_string key;
        
        while (!zcbor_map_end_decode(state)) {
            if (zcbor_tstr_decode(state, &key)) {
                char key_buf[32] = {0};
                if (key.len < sizeof(key_buf)) {
                    memcpy(key_buf, key.value, key.len);
                    
                    if (strcmp(key_buf, "config") == 0) {
                        LOG_INF("Found config section:");
                        if (zcbor_map_start_decode(state)) {
                            while (!zcbor_map_end_decode(state)) {
                                struct zcbor_string config_key;
                                if (zcbor_tstr_decode(state, &config_key)) {
                                    char config_key_buf[32] = {0};
                                    if (config_key.len < sizeof(config_key_buf)) {
                                        memcpy(config_key_buf, config_key.value, config_key.len);
                                        
                                        if (strcmp(config_key_buf, "update_interval") == 0) {
                                            uint32_t interval;
                                            if (zcbor_uint32_decode(state, &interval)) {
                                                LOG_INF("  update_interval: %u", interval);
                                            }
                                        } else if (strcmp(config_key_buf, "led") == 0) {
                                            LOG_INF("  led config:");
                                            if (zcbor_map_start_decode(state)) {
                                                while (!zcbor_map_end_decode(state)) {
                                                    struct zcbor_string led_key;
                                                    if (zcbor_tstr_decode(state, &led_key)) {
                                                        char led_key_buf[32] = {0};
                                                        if (led_key.len < sizeof(led_key_buf)) {
                                                            memcpy(led_key_buf, led_key.value, led_key.len);
                                                            uint32_t val;
                                                            if (zcbor_uint32_decode(state, &val)) {
                                                                LOG_INF("    %s: %u", led_key_buf, val);
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        } else {
                                            zcbor_any_skip(state, NULL);
                                        }
                                    }
                                }
                            }
                        }
                    } else if (strcmp(key_buf, "led") == 0) {
                        LOG_INF("Found top-level led:");
                        if (zcbor_map_start_decode(state)) {
                            while (!zcbor_map_end_decode(state)) {
                                struct zcbor_string led_key;
                                if (zcbor_tstr_decode(state, &led_key)) {
                                    char led_key_buf[32] = {0};
                                    if (led_key.len < sizeof(led_key_buf)) {
                                        memcpy(led_key_buf, led_key.value, led_key.len);
                                        uint32_t val;
                                        if (zcbor_uint32_decode(state, &val)) {
                                            LOG_INF("  %s: %u", led_key_buf, val);
                                        }
                                    }
                                }
                            }
                        }
                    } else {
                        LOG_INF("Skipping key: %s", key_buf);
                        zcbor_any_skip(state, NULL);
                    }
                }
            }
        }
    }
    LOG_INF("=== End CBOR Data ===");
}



int get_update_interval_from_cbor_response(const uint8_t *cbor,
					   size_t len,
					   uint32_t *interval_sec)
{
	struct config_object object = { 0 };
	size_t not_used;

	int err = cbor_decode_config_object(cbor, len, &object, &not_used);

	if (err) {
		return -EFAULT;
	}

	// Check if update_interval section exists (in config)
	if (!object.config_present || !object.config.update_interval_present) {
		return -ENOENT;
	}

	*interval_sec = object.config.update_interval.update_interval;

	return 0;
}

int get_led_command_from_cbor_response(const uint8_t *cbor,
                                       size_t len,
                                       void *led_msg)
{
    struct config_object object = { 0 };
    size_t not_used;
    struct led_msg *led_cmd = (struct led_msg *)led_msg;

    int err = cbor_decode_config_object(cbor, len, &object, &not_used);
    if (err) {
        // CBOR decoding failed - likely old format shadow data
        // This is expected when shadow contains only update_interval
        LOG_ERR("CBOR decoding failed with error: %d", err);
        LOG_HEXDUMP_ERR(cbor, len, "Failed CBOR data:");
        return -ENOENT;
    }
    
    LOG_INF("CBOR decode successful - config_present=%d, led_present=%d", 
            object.config_present, object.led_present);
    
    if (object.config_present) {
        LOG_INF("Config section: update_interval_present=%d, led_present=%d",
                object.config.update_interval_present, object.config.led_present);
        if (object.config.led_present) {
            LOG_INF("Config LED: R=%d G=%d B=%d", 
                    object.config.led.red, object.config.led.green, object.config.led.blue);
        }
    }

    // Check if LED configuration exists (prefer config.led for complete params, fallback to top-level)
    bool has_top_level_led = object.led_present;
    bool has_config_led = object.config_present && object.config.led_present;
    
    if (!has_top_level_led && !has_config_led) {
        LOG_INF("No LED found in shadow");
        return -ENOENT;
    }

    // Extract LED parameters from CBOR (prefer config.led for full params, fallback to top-level)
    led_cmd->type = LED_RGB_SET;
    
    if (has_config_led) {
        // Use config LED (has full timing parameters)
        led_cmd->red = object.config.led.red;
        led_cmd->green = object.config.led.green;
        led_cmd->blue = object.config.led.blue;
        led_cmd->duration_on_msec = object.config.led.duration_on_msec_present ? 
                                    object.config.led.duration_on_msec.duration_on_msec : 250;
        led_cmd->duration_off_msec = object.config.led.duration_off_msec_present ? 
                                     object.config.led.duration_off_msec.duration_off_msec : 2000;
        led_cmd->repetitions = object.config.led.repetitions_present ? 
                               object.config.led.repetitions.repetitions : 10;
        LOG_INF("Using config.led: R=%d G=%d B=%d, on=%dms, off=%dms, rep=%d", 
                led_cmd->red, led_cmd->green, led_cmd->blue,
                led_cmd->duration_on_msec, led_cmd->duration_off_msec, led_cmd->repetitions);
    } else {
        // Use top-level LED (basic colors only)
        led_cmd->red = object.led.red;
        led_cmd->green = object.led.green;
        led_cmd->blue = object.led.blue;
        led_cmd->duration_on_msec = object.led.duration_on_msec_present ? 
                                    object.led.duration_on_msec.duration_on_msec : 250;
        led_cmd->duration_off_msec = object.led.duration_off_msec_present ? 
                                     object.led.duration_off_msec.duration_off_msec : 2000;
        led_cmd->repetitions = object.led.repetitions_present ? 
                               object.led.repetitions.repetitions : 10;
        LOG_INF("Using top-level LED: R=%d G=%d B=%d, on=%dms, off=%dms, rep=%d", 
                led_cmd->red, led_cmd->green, led_cmd->blue,
                led_cmd->duration_on_msec, led_cmd->duration_off_msec, led_cmd->repetitions);
    }

    return 0;
}


