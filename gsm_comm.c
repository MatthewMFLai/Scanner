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

#include "gsm_comm.h"
#include "gsm_msg_if.h"
#include "gsm_msg.h"
#include "ringbuff.h"
#include "uart_reply.h"
#include "timestamp.h"
#include "nrf_gpio.h"
#include "nrf_drv_saadc.h"
#include "nrf_delay.h"
#include "SEGGER_RTT.h"

#define GPIO_TELLIT_ON                      12
#define GPIO_TELLIT_RESET                   13
#define GPIO_TELLIT_DTR                     11
#define AIN_TELLIT_MON                      0
#define TELLIT_ON_PULSE_MS                  1500
#define TELLIT_AT_GAP                       5000

#define ADC_REF_VOLTAGE_IN_MILLIVOLTS       600                                          /**< Reference voltage (in milli volts) used by ADC while doing conversion. */
#define ADC_PRE_SCALING_COMPENSATION        6                                            /**< The ADC is configured to use VDD with 1/3 prescaling as input. And hence the result of conversion is to be multiplied by 3 to get the actual value of the battery voltage.*/
#define DIODE_FWD_VOLT_DROP_MILLIVOLTS      270                                          /**< Typical forward voltage drop of the diode (Part no: SD103ATW-7-F) that is connected in series with the voltage supply. This is the voltage drop when the forward current is 1mA. Source: Data sheet of 'SURFACE MOUNT SCHOTTKY BARRIER DIODE ARRAY' available at www.diodes.com. */
#define ADC_RES_10BIT                       1024                                         /**< Maximum digital value for 10-bit ADC conversion. */

#define GSM_RBUFF_SIZE                      256

/**@brief Macro to convert the result of ADC conversion in millivolts.
 *
 * @param[in]  ADC_VALUE   ADC result.
 *
 * @retval     Result converted to millivolts.
 */
#define ADC_RESULT_IN_MILLI_VOLTS(ADC_VALUE)\
        ((((ADC_VALUE) * ADC_REF_VOLTAGE_IN_MILLIVOLTS) / ADC_RES_10BIT) * ADC_PRE_SCALING_COMPENSATION)

static nrf_saadc_value_t adc_buf[2];
static uint8_t m_gsm_comm_state;
static uint8_t m_gsm_rawbuff[GSM_RBUFF_SIZE] = {0};
static rbuff_t m_gsm_rbuff;

static char m_at[] = "AT";
static char m_cgdcont[] = "AT+CGDCONT = 1,\"IP\",\"nmrx.ca.apn\",\"0.0.0.0\"";
static char m_scfg[] = "AT#SCFG=1,1,1500,0,280,5";
static char m_sgact[] = "AT#SGACT=1,1";
static char m_sd[] = "AT#SD=1,0,17903,\"tcp.ngrok.io\"";
//static char m_sd[] = "AT#SD=1,0,1688,\"laipgw1.com\"";
static char m_csq[] = "AT+CSQ";
static char m_cops[] = "AT+COPS?";
static char m_creg[] = "AT+CREG?";
static char m_ccid[] = "AT#CCID";
static char m_cpin[] = "AT+CPIN=1228";

static char m_match_creg1[] = "+CREG: 1";
static char m_match_creg5[] = "+CREG: 5";
static char m_match_ok[] = "OK";
static char m_match_connect[] = "CONNECT";
static char m_match_nitz[] = "#NITZ:";

/**@brief Function for handling the ADC interrupt.
 *
 * @details  This function will fetch the conversion result from the ADC, convert the value into
 *           percentage and send it to peer.
 */
void saadc_event_handler(nrf_drv_saadc_evt_t const * p_event)
{
    if (p_event->type == NRF_DRV_SAADC_EVT_DONE)
    {
        nrf_saadc_value_t adc_result;
        uint16_t          batt_lvl_in_milli_volts;
        uint32_t          err_code;

        adc_result = p_event->data.done.p_buffer[0];

        err_code = nrf_drv_saadc_buffer_convert(p_event->data.done.p_buffer, 1);
        APP_ERROR_CHECK(err_code);

        batt_lvl_in_milli_volts = ADC_RESULT_IN_MILLI_VOLTS(adc_result) +
                                  DIODE_FWD_VOLT_DROP_MILLIVOLTS;
    }
}

/**@brief Function for configuring the GPIO.
 *
 * @details  This function will configure GPIO P0.12 and P0.13 as input lines.
 */
void gpio_configure(void)
{
	nrf_gpio_cfg_output(GPIO_TELLIT_ON);
	nrf_gpio_cfg_output(GPIO_TELLIT_RESET);
}

/**@brief Function for configuring ADC to do battery level conversion.
 */
static void adc_configure(void)
{
    ret_code_t err_code = nrf_drv_saadc_init(NULL, saadc_event_handler);
    APP_ERROR_CHECK(err_code);

    nrf_saadc_channel_config_t config = NRF_DRV_SAADC_DEFAULT_CHANNEL_CONFIG_SE(NRF_SAADC_INPUT_VDD);
    err_code = nrf_drv_saadc_channel_init(AIN_TELLIT_MON,&config);
    APP_ERROR_CHECK(err_code);

    err_code = nrf_drv_saadc_buffer_convert(&adc_buf[0], 1);
    APP_ERROR_CHECK(err_code);

    err_code = nrf_drv_saadc_buffer_convert(&adc_buf[1], 1);
    APP_ERROR_CHECK(err_code);
}

void gsm_comm_init(void)
{
	gpio_configure();
    adc_configure();
	m_gsm_comm_state = GSM_COMM_STATE_CREG;
	m_gsm_rbuff.elem_size = 1;
	m_gsm_rbuff.array_len = GSM_RBUFF_SIZE;
	m_gsm_rbuff.elem_array = m_gsm_rawbuff;
	rbuff_init(&m_gsm_rbuff);
}

void gsm_comm_poweron(void)
{
	uint32_t err_code;
	// Pulse the power on pin of the Tellit module for 1.5 seconds.
    nrf_gpio_pin_set(GPIO_TELLIT_ON);
    nrf_delay_ms(TELLIT_ON_PULSE_MS);
    nrf_gpio_pin_clear(GPIO_TELLIT_ON);
			
	// Check PWRMON (P0.02) from Tellit module
	err_code = nrf_drv_saadc_sample();
    APP_ERROR_CHECK(err_code);
}

void gsm_comm_fsm_start(void)
{
	SEGGER_RTT_printf(0, "Wait for registration\n");
	nrf_delay_ms(TELLIT_AT_GAP);
	nrf_delay_ms(TELLIT_AT_GAP);
	nrf_delay_ms(TELLIT_AT_GAP);
	nrf_delay_ms(TELLIT_AT_GAP);
	uart_reply_string(m_at);
	uart_reply_byte('\r');
	SEGGER_RTT_printf(0, "%s", m_at);
    nrf_delay_ms(TELLIT_AT_GAP);
	uart_reply_string(m_cgdcont);
	uart_reply_byte('\r');
	SEGGER_RTT_printf(0, "%s", m_cgdcont);
    nrf_delay_ms(TELLIT_AT_GAP);
	uart_reply_string(m_sgact);
	uart_reply_byte('\r');
	SEGGER_RTT_printf(0, "%s", m_sgact);
    nrf_delay_ms(TELLIT_AT_GAP);
	uart_reply_string(m_sd);
	uart_reply_byte('\r');
	SEGGER_RTT_printf(0, "%s", m_sd);
    nrf_delay_ms(TELLIT_AT_GAP);
    m_gsm_comm_state = GSM_COMM_STATE_CONNECTED;	
}

void gsm_comm_fsm_run(uint8_t *p_data, uint16_t len)
{
    uint16_t i;
	uint16_t match_len;
	gsm_msg_t *p_resp;
	
	// Get raw data into ringbuffer first.
/*    for (i = 0; i < len; i++)
        rbuff_push(&m_gsm_rbuff, p_data + i);
*/
    switch (m_gsm_comm_state)
    {
		case GSM_COMM_STATE_CREG:
		    match_len = strlen(m_match_creg1);
            if (len >= match_len)
            {
				if (!strncmp(m_match_creg1, p_data, match_len) ||
                    !strncmp(m_match_creg5, p_data, match_len))
				{
					uart_reply_string(m_at);
	                uart_reply_byte('\r');
					SEGGER_RTT_printf(0, "%s", m_at);
					m_gsm_comm_state = GSM_COMM_STATE_AT;
				}
			}				
		    break;
		case GSM_COMM_STATE_AT:
		    match_len = strlen(m_match_ok);
            if (len >= match_len)
            {
				if (!strncmp(m_match_ok, p_data, match_len))
				{
					uart_reply_string(m_cgdcont);
					uart_reply_byte('\r');
					SEGGER_RTT_printf(0, "%s", m_cgdcont);
					m_gsm_comm_state = GSM_COMM_STATE_CONTEXT;
			    }
			}
		    break;
		case GSM_COMM_STATE_CONTEXT:
		    match_len = strlen(m_match_ok);
            if (len >= match_len)
            {
				if (!strncmp(m_match_ok, p_data, match_len))
					m_gsm_comm_state = GSM_COMM_STATE_DELAY1;
			}
		    break;
		case GSM_COMM_STATE_DELAY1:
		    match_len = strlen(m_match_nitz);
            if (len >= match_len)
            {
				if (!strncmp(m_match_nitz, p_data, match_len))
				{
                    int year, month, day, hour, minute, second, timezone;
	
                    if (sscanf(p_data, "#NITZ: %d/%d/%d,%d:%d:%d-%d",
	                           &year, &month, &day, &hour, &minute, &second, &timezone) == 7)
			            ts_tod_set(year + Y2K, month, day, hour, minute, second, QUARTER_HOUR_SECONDS * timezone);
                    else if (sscanf(p_data, "#NITZ: %d/%d/%d,%d:%d:%d+%d",
                                    &year, &month, &day, &hour, &minute, &second, &timezone) == 7)
                        ts_tod_set(year + Y2K, month, day, hour, minute, second, QUARTER_HOUR_SECONDS * timezone * (-1));
				    else
					{
                        sscanf(p_data, "#NITZ: %d/%d/%d,%d:%d:%d",
	                           &year, &month, &day, &hour, &minute, &second);
                        ts_tod_set(year + Y2K, month, day, hour, minute, second, 0);							   
					}
				    uart_reply_string(m_sgact);
	                uart_reply_byte('\r');
					SEGGER_RTT_printf(0, "%s", m_sgact);
					m_gsm_comm_state = GSM_COMM_STATE_CONNECTION;
			    }
			}
		    break;
		case GSM_COMM_STATE_CONNECTION:
		    match_len = strlen(m_match_ok);
            if (len >= match_len)
            {
				if (!strncmp(m_match_ok, p_data, match_len))
				{
					uart_reply_string(m_sd);
	                uart_reply_byte('\r');
					SEGGER_RTT_printf(0, "%s", m_sd);
					m_gsm_comm_state = GSM_COMM_STATE_SOCKET;
			    }
			}
		    break;
		case GSM_COMM_STATE_SOCKET:
		    match_len = strlen(m_match_connect);
            if (len >= match_len)
            {
				if (!strncmp(m_match_connect, p_data, match_len))
				{
					m_gsm_comm_state = GSM_COMM_STATE_CONNECTED;
					gsm_msg_if_init();
					//gsm_msg_handshake_start();
				}
			}
		    break;
		case GSM_COMM_STATE_CONNECTED:
		    p_resp = gsm_msg_if_receive(p_data, len);
			//if (p_resp->msg_type == GSM_MSG_IF_DATA)
			gsm_msg_receive(p_resp);
		    break;
        default:
            break;
    }			
}
uint8_t gsm_comm_fsm_get_state(void)
{
	return (m_gsm_comm_state);
}
void gsm_comm_fsm_reset(void)
{
	m_gsm_comm_state = GSM_COMM_STATE_CREG;	
}
/**
 * @}
 */
