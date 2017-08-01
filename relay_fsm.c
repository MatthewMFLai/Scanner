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

#include "relay_fsm.h"
#include "gsm_comm.h"
#include "gsm_msg_tx.h"
#include "app_msg_tx_hdlr.h"
#include "app_msg_hdlr.h"
#include "app_msg_encrypt.h"
#include "msg_process.h"
#include "db_access.h"
#include "timestamp.h"
#include "geofence_hdlr.h"
#include "rssi_filter.h"
#include "SEGGER_RTT.h"

static uint8_t m_relay_fsm_state;
static char m_req_loc_info[] = "$AVREQ,00000000,1";
static char m_geofence_config[] = "$LPCFG,00000000";
static char m_geofence_delete[] = "$EAVGOF,00000000";

void relay_fsm_init(void)
{
    m_relay_fsm_state = RELAY_FSM_STATE_READY;	
}

void relay_fsm_reset(void)
{
    m_relay_fsm_state = RELAY_FSM_STATE_READY;	
}
					 
uint8_t relay_fsm_process_rbc(uint8_t *s, uint16_t len, uint16_t node_id)
{
	msg_desc_t *p_desc;
	
	if (gsm_comm_fsm_get_state() != GSM_COMM_STATE_CONNECTED)
		return (RELAY_FSM_NO_GSM);

	switch (m_relay_fsm_state) {
        case RELAY_FSM_STATE_READY:
		    SEGGER_RTT_printf(0, "relay_fsm_process_rbc RELAY_FSM_STATE_READY\n");
		    p_desc = msg_process_parse (s, len, MSG_FIELD_TYPE);
			if (msg_process_is_type(p_desc, MSG_STATUS_CODE))
			{
				uint16_t len;
				char status_code[1];
				
			    // Parse status report to get 1-byte status code
		        msg_process_get(s,
						&len,
						p_desc,
						MSG_FIELD_STATUS_CODE,
						status_code);
						
				// Kludge: set to default device node id 0.
                //gsm_msg_status_update(status_code[0], db_access_get_locatorid_with_nodeid(0));
				gsm_msg_multi_position_update(db_access_get_locatorid_with_nodeid(0),
				                              rssi_filter_report_best(), status_code[0]);
                if (status_code[0] == '4' ||
				    status_code[0] == 'X')				
			        m_relay_fsm_state = RELAY_FSM_STATE_WAIT_LN_ACK;
				else
				    m_relay_fsm_state = RELAY_FSM_STATE_WAIT_LN_RELAY_ACK;	
                ts_timer_set(TIMER_RELAY_MSG_FSM, TIMER_GSM_TIMEOUT_SECONDS);				
			}
			else if (msg_process_is_type(p_desc, MSG_PING_RQST))
			{
				char msg_data[16] = {0};
				uint16_t from_handle, to_handle;
				char *p_msg_type_str = msg_process_get_type_str(MSG_PING_RESP);
				msg_build_desc_t *p_build_desc = msg_process_pre_build(MSG_PING_RESP);

				p_build_desc->field[MSG_FIELD_TYPE] = (void *)p_msg_type_str;
				len = msg_process_build(msg_data, p_build_desc);
				db_access_get_handles(node_id, &from_handle, &to_handle);
				app_msg_encrypt_key_set((uint8_t *)db_access_get_key_with_handle(from_handle));
				// Encrypt message
				len = app_msg_encrypt(len, (uint8_t *)msg_data,
									  app_msg_encrypt_key_get(), MSG_ENCRYPT_COUNTER);					
				app_msg_hdlr_send(from_handle, len, msg_data);
			}
			else if (msg_process_is_type(p_desc, MSG_SCAN_ACK))
			{
				SEGGER_RTT_printf(0, "scan ack node_id: %d\n", node_id);
			}
			else if (msg_process_is_type(p_desc, MSG_FLUSH))
			{
				SEGGER_RTT_printf(0, "flush node_id: %d\n", node_id);
			}
			else
			{
				// Kludge: set to default device node id 0.
				gsm_msg_multi_position_update(db_access_get_locatorid_with_nodeid(0), len, GSM_MSG_STATUS_CODE_0);
				m_relay_fsm_state = RELAY_FSM_STATE_WAIT_LN_ACK;
				ts_timer_set(TIMER_RELAY_MSG_FSM, TIMER_GSM_TIMEOUT_SECONDS);
	        }
		    break;
			
        case RELAY_FSM_STATE_WAIT_LN_RELAY_ACK:
		    SEGGER_RTT_printf(0, "relay_fsm_process_rbc RELAY_FSM_STATE_WAIT_LN_RELAY_ACK\n");
            break;
			
        case RELAY_FSM_STATE_WAIT_LN_ACK:
		    SEGGER_RTT_printf(0, "relay_fsm_process_rbc RELAY_FSM_STATE_WAIT_LN_ACK\n");
            break;
			
        case RELAY_FSM_STATE_WAIT_MESH_RELAY_ACK:
		    SEGGER_RTT_printf(0, "relay_fsm_process_rbc RELAY_FSM_STATE_WAIT_MESH_RELAY_ACK\n");
		    p_desc = msg_process_parse (s, len, MSG_FIELD_TYPE);
			if (msg_process_is_type(p_desc, MSG_PING_RESP))
			{
			    gsm_msg_multi_position_update(db_access_get_locatorid_with_nodeid(node_id),
				                              rssi_filter_report_best(), GSM_MSG_STATUS_CODE_0);	
			    m_relay_fsm_state = RELAY_FSM_STATE_WAIT_LN_ACK;
				ts_timer_set(TIMER_RELAY_MSG_FSM, TIMER_GSM_TIMEOUT_SECONDS);
			}
            break;
			
        case RELAY_FSM_STATE_WAIT_MESH_ACK:
		    SEGGER_RTT_printf(0, "relay_fsm_process_rbc RELAY_FSM_STATE_WAIT_MESH_ACK\n");
            break;	
	}
}

uint8_t relay_fsm_process_gsm(uint8_t *s, uint16_t len)
{
	uint8_t *p_deviceaddr;
	uint16_t node_id, from_handle, to_handle;
	
	switch (m_relay_fsm_state) {
        case RELAY_FSM_STATE_READY:
		    SEGGER_RTT_printf(0, "relay_fsm_process_gsm RELAY_FSM_STATE_READY\n");
			if (!memcmp(m_req_loc_info, s, strlen(m_req_loc_info)))
			{
				char msg_data[16] = {0};
				char *p_msg_type_str = msg_process_get_type_str(MSG_PING_RQST);
				msg_build_desc_t *p_build_desc = msg_process_pre_build(MSG_PING_RQST);

				p_build_desc->field[MSG_FIELD_TYPE] = (void *)p_msg_type_str;
				len = msg_process_build(msg_data, p_build_desc);

                // Find out the from gateway to device handle for the default device.
                p_deviceaddr = db_access_get_default_deviceaddr();
				if (p_deviceaddr != NULL)
				{
					db_access_get(p_deviceaddr, &node_id, &from_handle, &to_handle);
                    app_msg_encrypt_key_set((uint8_t *)db_access_get_key_with_handle(from_handle));
					// Encrypt message
					len = app_msg_encrypt(len, (uint8_t *)msg_data,
										  app_msg_encrypt_key_get(), MSG_ENCRYPT_COUNTER);					
					app_msg_hdlr_send(from_handle, len, msg_data);

					m_relay_fsm_state = RELAY_FSM_STATE_WAIT_MESH_RELAY_ACK;
					ts_timer_set(TIMER_RELAY_MSG_FSM, TIMER_TIMEOUT_SECONDS);
				}
			}
			else if (!memcmp(m_geofence_config, s, strlen(m_geofence_config)))
			{
				// Just send the same message back with the locator id filled in.
				// $LPCFG,00000000,AG00=1,GF00=3|GF00|4351.5575|-7923.1368|4351.4958|-7923.0107*02
				memcpy(s + GSM_MSG_GEOFENCE_LOCATOR_MARKER, db_access_get_locatorid_with_nodeid(0),
				       strlen(db_access_get_locatorid_with_nodeid(0)));
				gsm_msg_raw_send(s, len);
				geofence_hdlr_fence_set();
			}
			else if (!memcmp(m_geofence_delete, s, strlen(m_geofence_delete)))
			{
				gsm_msg_geofence_delete_ack(db_access_get_locatorid_with_nodeid(0));
				geofence_hdlr_fence_clr();
			}
		    break;
			
        case RELAY_FSM_STATE_WAIT_LN_RELAY_ACK:
		    SEGGER_RTT_printf(0, "relay_fsm_process_gsm RELAY_FSM_STATE_WAIT_LN_RELAY_ACK\n");
		    // Send ack back to device.
			
			// Find out the from gateway to device handle for the default device.
			p_deviceaddr = db_access_get_default_deviceaddr();
			if (p_deviceaddr != NULL)
			{
				db_access_get(p_deviceaddr, &node_id, &from_handle, &to_handle);
                app_msg_encrypt_key_set((uint8_t *)db_access_get_key_with_handle(from_handle));
				// Encrypt message
				len = app_msg_encrypt(len, (uint8_t *)s,
									  app_msg_encrypt_key_get(), MSG_ENCRYPT_COUNTER);				
				app_msg_hdlr_send(from_handle, len, s);

				m_relay_fsm_state = RELAY_FSM_STATE_READY;
				ts_timer_clear(TIMER_RELAY_MSG_FSM);
			}
            break;
			
        case RELAY_FSM_STATE_WAIT_LN_ACK:
		    SEGGER_RTT_printf(0, "relay_fsm_process_gsm RELAY_FSM_STATE_WAIT_LN_ACK\n");
			m_relay_fsm_state = RELAY_FSM_STATE_READY;
			ts_timer_clear(TIMER_RELAY_MSG_FSM);
            break;
			
        case RELAY_FSM_STATE_WAIT_MESH_RELAY_ACK:
		    SEGGER_RTT_printf(0, "relay_fsm_process_gsm RELAY_FSM_STATE_WAIT_MESH_RELAY_ACK\n");
            break;
			
        case RELAY_FSM_STATE_WAIT_MESH_ACK:
		    SEGGER_RTT_printf(0, "relay_fsm_process_gsm RELAY_FSM_STATE_WAIT_MESH_ACK\n");
            break;	
	}
	return (RELAY_FSM_STATE_READY);
}