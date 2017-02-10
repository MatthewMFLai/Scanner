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

#include <stdint.h>
#include "timestamp.h"

static ts_data_t m_ts_data = {0, 0};

/**@brief Function for the Power manager.
 */
uint32_t ts_check_and_set(uint32_t ts_cur)
{
	if (ts_cur < m_ts_data.ts_prev)
		m_ts_data.ovrflw_counter++;

	m_ts_data.ts_prev = ts_cur;
	return (BIT24_CLICKS * m_ts_data.ovrflw_counter + ts_cur);
}