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

#include "gsm_msg_tx.h"
#include "gsm_msg_if.h"
#include "timestamp.h"
#include "util.h"
#include "app_timer.h"
#include "SEGGER_RTT.h"

#define LOCATORID_MARKER                7
#define UTC_TIME_MARKER                 16
#define UTC_DATE_MARKER                 60
#define STATUS_CODE_MARKER              67
/*
$AVSYS,80004680,18602,4063823670,64*7A
$EAVSYS,80004680,8901260113762815295,14049343719,Owner,0x52018602*23
$ECHK,80004680,0*37
$AVRMC,80004680,204730,r,4351.5177,N,07923.0563,W,0.27,0.00,110517,a,3299,626,1,0,0*7D
*/

static char m_pos_update[] = "$AVRMC,00000000,134755,a,4351.5177,N,07923.0563,W,0.27,0.00,250517,a,3299,626,1,0,0";
static char m_scratchpad[GSM_MSG_BUFF_SIZE];

// value of data* must be 1 <= data <= 31
static void build_avrmc_utc(char *p_dest, uint16_t offset,
	                uint32_t data1, uint32_t data2, uint32_t data3)
{
	p_dest += offset;
	
    if (data1 < TWO_DIGIT_VALUE)
    {
	    *p_dest = ZERO;
        p_dest++;
        longword_to_ascii(p_dest, data1);
        p_dest++;		
	}
    else
    {	
        longword_to_ascii(p_dest, data1);
		p_dest += 2;
	}
	
    if (data2 < TWO_DIGIT_VALUE)
    {
	    *p_dest = ZERO;
        p_dest++;
        longword_to_ascii(p_dest, data2);
        p_dest++;		
	}
    else
    {	
        longword_to_ascii(p_dest, data2);
		p_dest += 2;
	}
	
    if (data3 < TWO_DIGIT_VALUE)
    {
	    *p_dest = ZERO;
        p_dest++;
        longword_to_ascii(p_dest, data3);
        p_dest++;		
	}
    else
    {	
        longword_to_ascii(p_dest, data3);
		p_dest += 2;
	}
}

void gsm_msg_position_update(uint8_t *p_locatorid)
{					
	gsm_msg_status_update(GSM_MSG_STATUS_CODE_0, p_locatorid);
}

void gsm_msg_status_update(char status_code, uint8_t *p_locatorid)
{
	char *p_data;
	uint16_t len = 0;
    uint32_t cur_ticks;
    int year, month, day, hour, minute, second;
	
    m_pos_update[STATUS_CODE_MARKER] = status_code;
	
	app_timer_cnt_get(&cur_ticks);
	ts_tod_update(ts_check_and_set(cur_ticks)/1000);
	ts_tod_get(&year, &month, &day, &hour, &minute, &second);

    build_avrmc_utc(m_pos_update, UTC_TIME_MARKER,
	                (uint32_t)hour, (uint32_t)minute, (uint32_t)second);
    build_avrmc_utc(m_pos_update, UTC_DATE_MARKER,
	                (uint32_t)day, (uint32_t)month, (uint32_t)(year - Y2K));
	
	memcpy(&m_pos_update[LOCATORID_MARKER], p_locatorid, strlen(p_locatorid));
	
    memcpy(m_scratchpad, m_pos_update, strlen(m_pos_update));
    p_data = m_scratchpad + strlen(m_pos_update);
    *p_data++ = GSM_MSG_IF_SEPARATOR;
	len = strlen(m_pos_update);
	gsm_msg_if_send(m_scratchpad, len);	
}
/**
 * @}
 */
