#include <stdint.h>
#include <string.h>
#include "atcmd.h"
#include "app_uart.h"
#include "SEGGER_RTT.h"

#define n_array (sizeof (m_atcmds) / sizeof (const char *))

static char m_atcmd_cur[APP_ATCMD_LENGTH];
static char m_atcmd_param[APP_ATCMD_PARA_MAX][APP_ATCMD_PARA_LENGTH];
static char m_space = ' ';
static char m_lf = '\n';
static char m_cr = '\r';
static char m_ok_str[] ="OK";
static char m_nack_str[] ="NACK";
static char m_in_str[] ="IN";
static char m_out_str[] ="OUT";
static char m_def_building_code[] = "BUL001";

static atcmd_data_t m_scanner;

static const char * m_atcmds[] = {
    "at$scan",
    "at$mode",
    "at$scanint",
	"at$scan?",
	"at$mode?",
	"at$scanint?",
};

static atcmd_param_desc_t m_scan[] = {{0, 1}};  // scan status
static atcmd_param_desc_t m_mode[] = {{0, 1}};  // working mode
static atcmd_param_desc_t m_scanint[] = {{0, 2},   // scan interval
										 {0, 2}};  // scan window


static bool check_ascii_word (uint8_t *p_data, uint8_t len)
{
	uint8_t i;
	
	if (len > 5)
		return false;
	
	for (i = 0; i < len; i++)
	{
		if (*(p_data + i) < 0x30 ||
			*(p_data + i) > 0x39)
			return false;
	}
	return true;
}
										 
static uint16_t ascii_to_word (uint8_t *p_data, uint8_t len)
{
	uint8_t i;
	uint16_t rc = 0;
	
	for (i = 0; i < len; i++)
	{
		rc = (rc * 10) + *(p_data + i) - 0x30;
	}
	return (rc);
}

static uint8_t atcmd_match_cmd()
{	
    uint8_t i;

    for (i = 0; i < n_array; i++) {
        if (!strcmp(m_atcmds[i], m_atcmd_cur))
			break;
    }
	return (i);  	
}
										 
/**@brief Function to extract the command from the given raw data buffer.
 * 
 * @param[in] p_data  pointer to the first byte in the raw data buffer.
 */
static bool atcmd_extract_cmd(char buffer_len, char *p_data)
{
	uint8_t bytedata;
	uint8_t worddata[5] = {0};
	uint8_t i = 0;
	uint8_t j = 0;
	uint16_t scan_interval;
	uint16_t scan_window;
	while (*(p_data + i) != m_space &&
		*(p_data + i) != m_lf &&
		*(p_data + i) != m_cr &&
		i < buffer_len)
		i++;
		
	if (i == buffer_len)
		return false;
	
	memcpy(m_atcmd_cur, p_data, i);
	m_atcmd_cur[i] = '\0';
	
	switch (atcmd_match_cmd()) {
		case APP_ATCMD_ACT_ENABLE_SCAN :
			// Get the next parameter, the single byte on/off status
			while (*(p_data + i) == m_space &&
			       i < buffer_len)
				i++;
				
			if (i == buffer_len)
				return false;
	
			bytedata = *(p_data + i);
			if (!m_scan[0].is_str)
			{
				m_scanner.enable_byte = bytedata;
				// Convert ascii to data.
				bytedata -= 0x30;
				m_scanner.enable = bytedata;
			}
			break;
			
		case APP_ATCMD_ACT_MODE_0 :
			// Get the next parameter, the working mode value
			while (*(p_data + i) == m_space &&
			       i < buffer_len)
				i++;
				
			if (i == buffer_len)
				return false;
			
			bytedata = *(p_data + i);
			if (!m_mode[0].is_str)
			{
				m_scanner.mode_byte = bytedata;
				// Convert ascii to data.
				bytedata -= 0x30;
				m_scanner.mode = bytedata;
			}
			break;

		case APP_ATCMD_ACT_SCAN_INT :
			// Get the next parameter, the 2-byte interval value
			while (*(p_data + i) == m_space &&
			       i < buffer_len)
				i++;
				
			if (i == buffer_len)
				return false;

			j = i;
			while (*(p_data + j) != m_space &&
			       *(p_data + j) != m_cr &&
				   *(p_data + j) != m_lf)
				j++;			

			if (j == buffer_len)
				return false;

			memcpy(worddata, p_data + i, j - i);
			if (!check_ascii_word(worddata, j - i))
				return false;

			if (!m_scanint[0].is_str)
			{
				memcpy(m_scanner.scan_interval_str, worddata, j - i);
				m_scanner.scan_interval_str[j-i] = '\0';
				// Convert ascii to data.
				scan_interval = ascii_to_word(worddata, j - i);
			}
			
			// Get the next parameter, the 2-byte window value
			i = j;
			while (*(p_data + i) == m_space &&
			       i < buffer_len)
				i++;
				
			if (i == buffer_len)
				return false;

			j = i;
			while (*(p_data + j) != m_space &&
			       *(p_data + j) != m_cr &&
				   *(p_data + j) != m_lf)
				j++;			

			if (j == buffer_len)
				return false;

			memcpy(worddata, p_data + i, j - i);
			if (!check_ascii_word(worddata, j - i))
				return false;
			
			if (!m_scanint[1].is_str)
			{
				memcpy(m_scanner.scan_window_str, worddata, j - i);
				m_scanner.scan_window_str[j-i] = '\0';
				// Convert ascii to data.
				scan_window = ascii_to_word(worddata, j - i);
			}
			
			if (scan_interval && scan_window)
			{
				m_scanner.scan_interval = scan_interval;
				m_scanner.scan_window = scan_window;
			}
			else
				return false;

			break;
			
		case APP_ATCMD_ACT_ENABLE_SCAN_READ :
		case APP_ATCMD_ACT_MODE_0_READ :
		case APP_ATCMD_ACT_SCAN_INT_READ :
			break;
			
		default :
			break;
	}
	return true;
}

static uint8_t atcmd_run_cmd()
{	
	uint8_t rc = APP_ATCMD_NOT_SUPPORTED;
	
	switch (atcmd_match_cmd()) {
		case APP_ATCMD_ACT_ENABLE_SCAN :
			rc = APP_ATCMD_ACT_ENABLE_SCAN;
			break;

		case APP_ATCMD_ACT_MODE_0 :
			rc = APP_ATCMD_ACT_MODE_0;
			break;

		case APP_ATCMD_ACT_SCAN_INT :
			rc = APP_ATCMD_ACT_SCAN_INT;
			break;
			
		case APP_ATCMD_ACT_ENABLE_SCAN_READ :
			rc = APP_ATCMD_ACT_ENABLE_SCAN_READ;
			break;
			
		case APP_ATCMD_ACT_MODE_0_READ :
			rc = APP_ATCMD_ACT_MODE_0_READ;
			break;
			
		case APP_ATCMD_ACT_SCAN_INT_READ :
			rc = APP_ATCMD_ACT_SCAN_INT_READ;
			break;
			
		default :
			break;
	}
	return (rc);
}

/**@brief Function to initialize the at command tables.
 */
void atcmd_init(void)
{
	memset(m_atcmd_cur, 0, APP_ATCMD_LENGTH);
	memset(m_atcmd_param, 0, APP_ATCMD_PARA_MAX * APP_ATCMD_PARA_LENGTH);
	m_scanner.scan_interval = 0x00A0;
	m_scanner.scan_window = 0x0050;
	m_scanner.mode = 0;
	m_scanner.mode_byte = '0';
	m_scanner.enable = 1;
	m_scanner.enable_byte = '0';
	strcpy(m_scanner.building_code, m_def_building_code);
}

/**@brief Function to store the current at command in the at command table.
 * @details Use the built-in h/w encryption engine.
 * 
 * @param[in] p_data  pointer to the at command in the raw data buffer.
 * @param[in] cmd_len  size, in bytes, of the current at command.
 */
uint8_t atcmd_parse(char buffer_len, char *p_data)
{
	if (atcmd_extract_cmd(buffer_len, p_data))
		return (atcmd_run_cmd());
	else
		return APP_ATCMD_NOT_SUPPORTED;
}

void atcmd_get_scan_param(uint16_t *p_interval, uint16_t *p_window)
{
	*p_interval = m_scanner.scan_interval;
	*p_window = m_scanner.scan_window;
}

uint8_t atcmd_scan_enabled(void)
{
	return (m_scanner.enable);
}

void atcmd_reply_scan(void)
{
	while(app_uart_put(m_scanner.enable_byte) != NRF_SUCCESS);
	while(app_uart_put('\n') != NRF_SUCCESS);
}

void atcmd_reply_mode(void)
{
	while(app_uart_put(m_scanner.mode_byte) != NRF_SUCCESS);
	while(app_uart_put('\n') != NRF_SUCCESS);
}

void atcmd_reply_scanint(void)
{
	for (uint8_t i = 0; i < strlen(m_scanner.scan_interval_str); i++)
	{
		while(app_uart_put(m_scanner.scan_interval_str[i]) != NRF_SUCCESS);
	}
	while(app_uart_put(' ') != NRF_SUCCESS);
	for (uint8_t i = 0; i < strlen(m_scanner.scan_window_str); i++)
	{
		while(app_uart_put(m_scanner.scan_window_str[i]) != NRF_SUCCESS);
	}
	while(app_uart_put('\n') != NRF_SUCCESS);
}

/**@brief Function to store the current at command in the at command table.
 * @details Use the built-in h/w encryption engine.
 * 
 * @param[in] p_data  pointer to the at command in the raw data buffer.
 * @param[in] cmd_len  size, in bytes, of the current at command.
 */
void atcmd_reply_ok(void)
{
	for (uint8_t i = 0; i < strlen(m_ok_str); i++)
	{
		while(app_uart_put(m_ok_str[i]) != NRF_SUCCESS);
	}
	while(app_uart_put('\n') != NRF_SUCCESS);
}

/**@brief Function to store the current at command in the at command table.
 * @details Use the built-in h/w encryption engine.
 * 
 * @param[in] p_data  pointer to the at command in the raw data buffer.
 * @param[in] cmd_len  size, in bytes, of the current at command.
 */
void atcmd_reply_nack(void)
{
	for (uint8_t i = 0; i < strlen(m_nack_str); i++)
	{
		while(app_uart_put(m_nack_str[i]) != NRF_SUCCESS);
	}
	while(app_uart_put('\n') != NRF_SUCCESS);
}

/**@brief Function to store the current at command in the at command table.
 * @details Use the built-in h/w encryption engine.
 * 
 * @param[in] p_data  pointer to the at command in the raw data buffer.
 * @param[in] cmd_len  size, in bytes, of the current at command.
 */
void atcmd_report_in(void)
{
	for (uint8_t i = 0; i < strlen(m_in_str); i++)
	{
		while(app_uart_put(m_in_str[i]) != NRF_SUCCESS);
	}
	while(app_uart_put('\n') != NRF_SUCCESS);
}

/**@brief Function to store the current at command in the at command table.
 * @details Use the built-in h/w encryption engine.
 * 
 * @param[in] p_data  pointer to the at command in the raw data buffer.
 * @param[in] cmd_len  size, in bytes, of the current at command.
 */
void atcmd_report_out(void)
{
	for (uint8_t i = 0; i < strlen(m_out_str); i++)
	{
		while(app_uart_put(m_out_str[i]) != NRF_SUCCESS);
	}
	while(app_uart_put('\n') != NRF_SUCCESS);
}