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

#include "gsm_msg.h"
#include "gsm_msg_tx.h"
#include "gsm_msg_if.h"
#include "util.h"
#include "relay_fsm.h"
#include "SEGGER_RTT.h"

/*
$AVSYS,80004680,18602,4063823670,64*7A
$EAVSYS,80004680,8901260113762815295,14049343719,Owner,0x52018602*23
$ECHK,80004680,0*37
$AVRMC,80004680,204730,r,4351.5177,N,07923.0563,W,0.27,0.00,110517,a,3299,626,1,0,0*7D
*/
static char m_handshake1[] = "$AVSYS,80006444,18602,4063823670,64";
static char m_handshake2[] = "$EAVSYS,80006444,89302720399930508089,14167799583,Owner,0x52018602";
static char m_handshake3[] = "$ECHK,80006444,0";
static char m_req_sys_info[] = "$AVREQ,?";
static char m_req_loc_info[] = "$AVREQ,00000000,1";
static char m_geofence_config[] = "$LPCFG,00000000";
static char m_geofence_delete[] = "$EAVGOF,00000000";
static char m_geofence_ack[] = "$AVCFG,00000000";
static char m_ack[] = "$EAVACK";
static char m_scratchpad[GSM_MSG_BUFF_SIZE];

void gsm_msg_init(void)
{
	
}

void gsm_msg_handshake_start(void)
{
	char *p_data;
	uint16_t len = 0;
	
    // Send the 3 handshake messages.
    memcpy(m_scratchpad, m_handshake1, strlen(m_handshake1));
    p_data = m_scratchpad + strlen(m_handshake1);
    *p_data++ = GSM_MSG_IF_SEPARATOR;
    memcpy(p_data, m_handshake2, strlen(m_handshake2));
	p_data += strlen(m_handshake2);
    *p_data++ = GSM_MSG_IF_SEPARATOR;
    memcpy(p_data, m_handshake3, strlen(m_handshake3));	
	len = strlen(m_handshake1) + strlen(m_handshake2) + strlen(m_handshake3) + 2;
	
	gsm_msg_if_send(m_scratchpad, len);
}

void gsm_msg_receive(gsm_msg_t *p_msg)
{
	char *p_data = p_msg->msg_array;
	uint16_t len = p_msg->msg_size;
	
    if (!memcmp(m_req_sys_info, p_data, len))
	{
		//memcpy(m_scratchpad, m_handshake1, strlen(m_handshake1));
		//p_data = m_scratchpad + strlen(m_handshake1);
		//*p_data++ = GSM_MSG_IF_SEPARATOR;
		//len = strlen(m_handshake1);	
		//gsm_msg_if_reply(m_scratchpad, len);	
		gsm_msg_handshake_start();	
    }
	else if (!memcmp(m_req_loc_info, p_data, len))
	{
		//gsm_msg_position_update();
		relay_fsm_process_gsm(p_data, len);
    }
	else if (!memcmp(m_ack, p_data, strlen(m_ack)))
	{
		relay_fsm_process_gsm(p_data, len);
    }
	else if (!memcmp(m_geofence_config, p_data, strlen(m_geofence_config)))
	{
		relay_fsm_process_gsm(p_data, len);
    }
	else if (!memcmp(m_geofence_delete, p_data, strlen(m_geofence_delete)))
	{
		relay_fsm_process_gsm(p_data, len);
    }
	else if (!memcmp(m_geofence_ack, p_data, strlen(m_geofence_ack)))
	{
		relay_fsm_process_gsm(p_data, len);
    }
}

void gsm_msg_reset(void)
{
	
}

/**
 * @}
 */
