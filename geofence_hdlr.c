/* Copyright (c) 2014 Nordic Semiconductor. All Rights Reserved.
 *
 * The information contained herein is property of Nordic Semiconductor ASA.
 * Terms and conditions of usage are described in detail in NORDIC
 * SEMICONDUCTOR STANDARD SOFTWARE LICENSE AGREEMENT.
 *
 * Licensees are granted free, non-transferable use of the information. NO
 * WARRANTY of ANY KIND is provided. This heading must NOT be removed from
 * the file.
 *
 */

/** @file
 *
 * @defgroup ble_sdk_app_template_main main.c
 * @{
 * @ingroup ble_sdk_app_template
 * @brief Template project main file.
 *
 * This file contains a template for creating a new application. It has the code necessary to wakeup
 * from button, advertise, get a connection restart advertising on disconnect and if no new
 * connection created go back to system-off mode.
 * It can easily be used as a starting point for creating a new application, the comments identified
 * with 'YOUR_JOB' indicates where and how you can customize.
 */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "geofence_hdlr.h"
#include "timestamp.h"
#include "SEGGER_RTT.h"

static bool m_geofence_set;
static bool m_geofence_first_in;

void geofence_hdlr_init(void)
{
	m_geofence_set = false;
	m_geofence_first_in = false;
}

void geofence_hdlr_fence_set(void)
{
	m_geofence_set = true;
	m_geofence_first_in = false;
}

void geofence_hdlr_fence_clr(void)
{
	m_geofence_set = false;
	m_geofence_first_in = false;
}

uint8_t geofence_hdlr_device_update(void)
{
	if (!m_geofence_set)
		return (GEOFENCE_HDLR_RC_NO_GEOFENCE);
	
	ts_marker_update(2);
	
	if (!m_geofence_first_in)
	{
		SEGGER_RTT_printf(0, "Geofence In\n");
		m_geofence_first_in = true;
		return (GEOFENCE_HDLR_RC_FIRST_IN);
	}	
    return (GEOFENCE_HDLR_RC_IN);	
}

uint8_t geofence_hdlr_timer_update(void)
{
	if (!m_geofence_set)
		return (GEOFENCE_HDLR_RC_NO_GEOFENCE);

    if (ts_marker_exceed(2, 10, TS_MARKER_SECOND))
    {
		if (m_geofence_first_in)
		{
			SEGGER_RTT_printf(0, "Geofence Out\n");
			m_geofence_first_in = false;
			return (GEOFENCE_HDLR_RC_FIRST_OUT);
		}
		else
			return (GEOFENCE_HDLR_RC_OUT);
	}		
	return (GEOFENCE_HDLR_RC_IN);
}