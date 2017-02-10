#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include "atcmd.h"
#include "pstore.h"
#include "SEGGER_RTT.h"

#define n_array (sizeof (m_atcmds) / sizeof (const char *))

static char m_atcmd_cur[APP_ATCMD_LENGTH];
static char m_atcmd_param[APP_ATCMD_PARA_MAX][APP_ATCMD_PARA_LENGTH];
static char m_space = ' ';
static char m_cr = '\r';
static char m_ok_str[] ="OK";
static char m_nack_str[] ="NACK";
static char m_in_str[] ="IN";
static char m_out_str[] ="OUT";
static char m_def_building_code[] = "BUL001";

static char m_configdata[PSTORE_MAX_BLOCK];

static atcmd_data_t m_scanner;

static const char * m_atcmds[] = {
    "at$scan",
    "at$mode",
    "at$scanint",
	"at$scan?",
	"at$mode?",
	"at$scanint?",
	"at$cfgget?",
	"at$cfgset",
	"at$cfggetv?",
	"at$cfgupd",
	"at$curts?"
};

static atcmd_param_desc_t m_scan[] = {{0, 1}};  // scan status
static atcmd_param_desc_t m_mode[] = {{0, 1}};  // working mode
static atcmd_param_desc_t m_scanint[] = {{0, 2},   // scan interval
										 {0, 2}};  // scan window
static atcmd_param_desc_t m_configdat[] = {{1, 0},   // version string
										   {0, 2},  // config data size
										   {1, 0}};   // config data, no white space support
										   
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
static bool atcmd_extract_cmd(uint16_t buffer_len, char *p_data)
{
	uint8_t bytedata;
	uint8_t worddata[5] = {0};
	uint16_t i = 0;
	uint16_t j = 0;
	uint16_t scan_interval;
	uint16_t scan_window;
	while (*(p_data + i) != m_space &&
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
			       *(p_data + j) != m_cr)
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
			       *(p_data + j) != m_cr)
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
		case APP_ATCMD_ACT_CONFIG_GET :
		case APP_ATCMD_ACT_CONFIG_GET_VER :
			break;
			
		case APP_ATCMD_ACT_CONFIG_SET :
			// Get the next parameter, the version string
			while (*(p_data + i) == m_space &&
			       i < buffer_len)
				i++;
				
			if (i == buffer_len)
				return false;

			j = i;
			while (*(p_data + j) != m_space &&
			       *(p_data + j) != m_cr
				   )
				j++;			

			if (j == buffer_len)
				return false;

			if (m_configdat[0].is_str)
			{
				memcpy(m_scanner.version_str, p_data + i, j - i);
			}
			
			// Get the next parameter, the 2-byte config data size
			i = j;
			while (*(p_data + i) == m_space &&
			       i < buffer_len)
				i++;
				
			if (i == buffer_len)
				return false;

			j = i;
			while (*(p_data + j) != m_space &&
			       *(p_data + j) != m_cr)
				j++;			

			if (j == buffer_len)
				return false;

			memcpy(worddata, p_data + i, j - i);
			if (!check_ascii_word(worddata, j - i))
				return false;
			
			if (!m_configdat[1].is_str)
			{
				memcpy(m_scanner.config_size_str, worddata, j - i);
				m_scanner.config_size_str[j-i] = '\0';
				// Convert ascii to data.
				scan_window = ascii_to_word(worddata, j - i);
			}
			
			if (scan_window)
			{
				m_scanner.config_size = scan_window;
			}
			else
				return false;

			// Get the next parameter, the config data content
			i = j;
			while (*(p_data + i) == m_space &&
			       i < buffer_len)
				i++;
				
			if (i == buffer_len)
				return false;

			j = i;
			while (*(p_data + j) != m_space &&
			       *(p_data + j) != m_cr
				   )
				j++;			

			if (j == buffer_len)
				return false;

			if (m_configdat[2].is_str)
			{
				memcpy(m_configdata, p_data + i, j - i);
			}			
			
			break;
			
		case APP_ATCMD_ACT_CONFIG_UPD :
		case APP_ATCMD_ACT_CURRENT_TS :
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

		case APP_ATCMD_ACT_CONFIG_GET :
			rc = APP_ATCMD_ACT_CONFIG_GET;
			break;
			
		case APP_ATCMD_ACT_CONFIG_SET :
			pstore_set((uint8_t *)m_configdata, m_scanner.config_size);
			rc = APP_ATCMD_ACT_CONFIG_SET;
			break;

		case APP_ATCMD_ACT_CONFIG_GET_VER :
			rc = APP_ATCMD_ACT_CONFIG_GET_VER;
			break;
			
		case APP_ATCMD_ACT_CONFIG_UPD :
			rc = APP_ATCMD_ACT_CONFIG_UPD;
			break;

		case APP_ATCMD_ACT_CURRENT_TS :
			rc = APP_ATCMD_ACT_CURRENT_TS;
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
	m_scanner.config_size = 0;
	memset(m_configdata, 0, PSTORE_MAX_BLOCK);
	memset(m_scanner.version_str, 0, APP_VERSION_STR_MAX);
	memset(m_scanner.config_size_str, 0, APP_WORD_STR_LEN);
}

/**@brief Function to store the current at command in the at command table.
 * @details Use the built-in h/w encryption engine.
 * 
 * @param[in] p_data  pointer to the at command in the raw data buffer.
 * @param[in] cmd_len  size, in bytes, of the current at command.
 */
uint8_t atcmd_parse(uint16_t buffer_len, char *p_data)
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

char atcmd_get_enable(void)
{
	return (m_scanner.enable_byte);
}

char atcmd_get_mode(void)
{
	return (m_scanner.mode_byte);
}

char *atcmd_get_interval(void)
{
	return (m_scanner.scan_interval_str);
}

char *atcmd_get_window(void)
{
	return (m_scanner.scan_window_str);
}

/**@brief Function to store the current at command in the at command table.
 * @details Use the built-in h/w encryption engine.
 * 
 * @param[in] p_data  pointer to the at command in the raw data buffer.
 * @param[in] cmd_len  size, in bytes, of the current at command.
 *
 * @Warning This routine may cause the watchdog timer to reset if the config data
 *          is more than 128 bytes. For a huge config data use the scheduler to
 *          send the data instead.
 */
char *atcmd_reply_config(void)
{
	uint16_t config_size;
	
	config_size = pstore_get((uint8_t *)m_configdata);
	m_configdata[config_size] = 0x00;
    return (m_configdata);
}

/**@brief Function to store the current at command in the at command table.
 * @details Use the built-in h/w encryption engine.
 * 
 * @param[in] p_data  pointer to the at command in the raw data buffer.
 * @param[in] cmd_len  size, in bytes, of the current at command.
 */
char *atcmd_get_ok(void)
{
    return (m_ok_str);
}

/**@brief Function to store the current at command in the at command table.
 * @details Use the built-in h/w encryption engine.
 * 
 * @param[in] p_data  pointer to the at command in the raw data buffer.
 * @param[in] cmd_len  size, in bytes, of the current at command.
 */
char *atcmd_get_nack(void)
{
	return (m_nack_str);
}

/**@brief Function to store the current at command in the at command table.
 * @details Use the built-in h/w encryption engine.
 * 
 * @param[in] p_data  pointer to the at command in the raw data buffer.
 * @param[in] cmd_len  size, in bytes, of the current at command.
 */
char *atcmd_get_in(void)
{
	return (m_in_str);
}

/**@brief Function to store the current at command in the at command table.
 * @details Use the built-in h/w encryption engine.
 * 
 * @param[in] p_data  pointer to the at command in the raw data buffer.
 * @param[in] cmd_len  size, in bytes, of the current at command.
 */
char *atcmd_get_out(void)
{
	return (m_out_str);
}