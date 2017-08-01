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

#include "gsm_msg_if.h"
#include "ringbuff.h"
#include "uart_reply.h"
#include "util.h"
#include "timestamp.h"
#include "SEGGER_RTT.h"

#define GSM_RBUFF_SIZE                      1024
#define GSM_SCRATCHPAD_SIZE                  256

static uint8_t m_gsm_msg_if_state;
static uint8_t m_gsm_rawbuff_rx[GSM_RBUFF_SIZE] = {0};
static rbuff_t m_gsm_rbuff_rx;
static uint8_t m_gsm_rawbuff_tx[GSM_RBUFF_SIZE] = {0};
static rbuff_t m_gsm_rbuff_tx;
static gsm_msg_t m_resp_data;
static uint8_t m_scratchpad[GSM_SCRATCHPAD_SIZE];

void gsm_msg_if_init(void)
{
	m_gsm_msg_if_state = GSM_MSG_IF_STATE_READY;
	m_gsm_rbuff_rx.elem_size = 1;
	m_gsm_rbuff_rx.array_len = GSM_RBUFF_SIZE;
	m_gsm_rbuff_rx.elem_array = m_gsm_rawbuff_rx;
	rbuff_init(&m_gsm_rbuff_rx);
	m_gsm_rbuff_tx.elem_size = 1;
	m_gsm_rbuff_tx.array_len = GSM_RBUFF_SIZE;
	m_gsm_rbuff_tx.elem_array = m_gsm_rawbuff_tx;
	rbuff_init(&m_gsm_rbuff_tx);	
}

gsm_msg_t *gsm_msg_if_receive(uint8_t *p_data, uint16_t len)
{
	uint8_t checksum;
	char msnibble, lsnibble;

	// Verify checksum.
	memcpy(m_scratchpad, p_data, len - GSM_MSG_IF_SUFFIX_LEN);
	m_scratchpad[len - GSM_MSG_IF_SUFFIX_LEN] = GSM_MSG_IF_SEPARATOR;
	checksum = calc_checksum(m_scratchpad);
	SEGGER_RTT_printf(0, "rx msg checksum: %x\n", checksum);
	// Convert checksum to hex bytes.
	byte_to_hex(&msnibble, &lsnibble, checksum);
	if (p_data[len - GSM_MSG_IF_SUFFIX_LEN] != msnibble ||
		p_data[len - GSM_MSG_IF_SUFFIX_LEN + 1] != lsnibble)
	{
		SEGGER_RTT_printf(0, "incorrect rx msg checksum: %x\n", checksum);
		m_resp_data.msg_type = GSM_MSG_IF_NAK;
		return (&m_resp_data);
	}
			
    switch (m_gsm_msg_if_state)
    {
		case GSM_MSG_IF_STATE_READY:

			m_resp_data.msg_array = p_data;
			m_resp_data.msg_size = len - GSM_MSG_IF_SUFFIX_LEN - 1;
			m_resp_data.msg_type = GSM_MSG_IF_DATA;				

		    break;
			
		case GSM_MSG_IF_STATE_WAITLN:

			m_gsm_msg_if_state = GSM_MSG_IF_STATE_READY;
			rbuff_flush(&m_gsm_rbuff_tx);
			m_resp_data.msg_array = p_data;
			m_resp_data.msg_size = len - GSM_MSG_IF_SUFFIX_LEN - 1;
			m_resp_data.msg_type = GSM_MSG_IF_ACK;
			SEGGER_RTT_printf(0, "rbuf cleared.\n");				
            ts_timer_clear(TIMER_GSM_MSG_IF_FSM);
			
		    break;
			
		default:
		    break;
	}
    return (&m_resp_data);	
}

// Support sending mulitple messages in one call. The separator must be
// the \0 byte.
// eg.  <msg>
//      <msg>\0<msg>\0<msg>
// The len parameter must exclude the in between \0 terminator bytes,
// but not the \0 after the very last message.
uint8_t gsm_msg_if_send(uint8_t *p_data, uint16_t len)
{
	char msgout[200];
	char msnibble, lsnibble;
	char star, suffix1, suffix2;
	uint8_t checksum, rc;
	uint16_t i = 0;
	uint16_t j = 0;
	uint16_t idx;
	
	rc = 1;
	star = GSM_MSG_IF_STAR;
	suffix1 = GSM_MSG_IF_SUFFIX1;
	suffix2 = GSM_MSG_IF_SUFFIX2;
	
    switch (m_gsm_msg_if_state)
    {
		case GSM_MSG_IF_STATE_READY:
            while (j <= len)
			{
				if (p_data[j] != GSM_MSG_IF_SEPARATOR)
					{
						j++;
						continue;
					}
				// message in the range p_data[i] to p_data[j-1]
				checksum = calc_checksum(p_data + i);
				
				// Convert checksum to hex bytes.
				byte_to_hex(&msnibble, &lsnibble, checksum);
				
				// Send message to Telit module.
				uart_reply_string2(p_data + i, j - i);
				
				memcpy(msgout, p_data + i, j - i);
				msgout[j - i] = '\0';
				SEGGER_RTT_printf(0, "sent: %s", msgout);
				
				// Save message in ring buffer.
				for (idx = i; idx < j; idx++)
					rbuff_push(&m_gsm_rbuff_tx, p_data + idx);
				
				uart_reply_byte(star);
				SEGGER_RTT_printf(0, "%c", star);
				rbuff_push(&m_gsm_rbuff_tx, &star);
				uart_reply_byte(msnibble);
				SEGGER_RTT_printf(0, "%c", msnibble);
				rbuff_push(&m_gsm_rbuff_tx, &msnibble);
				uart_reply_byte(lsnibble);
				SEGGER_RTT_printf(0, "%c", lsnibble);
				rbuff_push(&m_gsm_rbuff_tx, &lsnibble);
				uart_reply_byte(suffix1);
				SEGGER_RTT_printf(0, "%c", suffix1);
				rbuff_push(&m_gsm_rbuff_tx, &suffix1);
				uart_reply_byte(suffix2);
				SEGGER_RTT_printf(0, "%c", suffix2);
				rbuff_push(&m_gsm_rbuff_tx, &suffix2);
				
				j++;
				i = j;
			}
			m_gsm_msg_if_state = GSM_MSG_IF_STATE_WAITLN;
			ts_timer_set(TIMER_GSM_MSG_IF_FSM, TIMER_GSM_TIMEOUT_SECONDS);
			rc = 0;
			
            break;
        default:
            break;
    }
    return (rc);	
}

uint8_t gsm_msg_if_reply(uint8_t *p_data, uint16_t len)
{
	char msgout[200];
	char msnibble, lsnibble;
	char star, suffix1, suffix2;
	uint8_t checksum, rc;
	
	rc = 1;
	star = GSM_MSG_IF_STAR;
	suffix1 = GSM_MSG_IF_SUFFIX1;
	suffix2 = GSM_MSG_IF_SUFFIX2;
	
    switch (m_gsm_msg_if_state)
    {
		case GSM_MSG_IF_STATE_READY:

			checksum = calc_checksum(p_data);
			
			// Convert checksum to hex bytes.
			byte_to_hex(&msnibble, &lsnibble, checksum);
			
			// Send message to Telit module.
			uart_reply_string(p_data);
			
			memcpy(msgout, p_data, len);
			msgout[len] = '\0';
			SEGGER_RTT_printf(0, "sent: %s", msgout);
			
			uart_reply_byte(star);
			SEGGER_RTT_printf(0, "%c", star);

			uart_reply_byte(msnibble);
			SEGGER_RTT_printf(0, "%c", msnibble);

			uart_reply_byte(lsnibble);
			SEGGER_RTT_printf(0, "%c", lsnibble);

			uart_reply_byte(suffix1);
			SEGGER_RTT_printf(0, "%c", suffix1);

			uart_reply_byte(suffix2);
			SEGGER_RTT_printf(0, "%c", suffix2);

			rc = 0;
			
            break;
        default:
            break;
    }
    return (rc);	
}

uint8_t gsm_msg_if_get_state(void)
{
	
}

void gsm_msg_if_reset(void)
{
	m_gsm_msg_if_state = GSM_MSG_IF_STATE_READY;
	rbuff_flush(&m_gsm_rbuff_tx);	
}

void gsm_msg_if_disconnect(void)
{
	
}

/**
 * @}
 */
