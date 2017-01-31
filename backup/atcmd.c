#include <stdint.h>
#include <string.h>
#include "atcmd.h"
#include "app_uart.h"

#define n_array (sizeof (m_atcmds) / sizeof (const char *))

static char m_atcmd_cur[APP_ATCMD_LENGTH];
static char m_atcmd_param[APP_ATCMD_PARA_MAX][APP_ATCMD_PARA_LENGTH];
static char m_space = ' ';
static char m_ok_str[] ="OK";
static char m_nack_str[] ="AT cmd invalid";
static char m_in_str[] ="IN";
static char m_out_str[] ="OUT";
static char m_def_building_code[] = "BUL001";

static atcmd_data_t m_scanner;

static const char * m_atcmds[] = {
    "at$scan",
    "at$noscan",
    "at$mode0",
    "at$mode1",
    "at$scanint",
};

//static atcmd_param_desc_t m_scan[] = {{0, 1, 1}};  // scan status
//static atcmd_param_desc_t m_mode[] = {{0, 1, 1}};  // working mode
//static atcmd_param_desc_t m_scanint[] = {{0, 1, 2},   // scan interval
//										 {0, 1, 2}};  // scan window
										 
/**@brief Function to extract the command from the given raw data buffer.
 * 
 * @param[in] p_data  pointer to the first byte in the raw data buffer.
 */
static void atcmd_extract_cmd(char *p_data)
{
	uint8_t i = 0;
	while (*(p_data + i) != m_space)
		i++;
	memcpy(m_atcmd_cur, p_data, i);
	m_atcmd_cur[i] = '\0';
}

static uint8_t atcmd_run_cmd()
{	
    uint8_t i;

    for (i = 0; i < n_array; i++) {
        if (!strcmp(m_atcmds[i], m_atcmd_cur))
			break;
    }
	return (i);  	
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
	m_scanner.enable = 1;
	strcpy(m_scanner.building_code, m_def_building_code);
}

/**@brief Function to store the current at command in the at command table.
 * @details Use the built-in h/w encryption engine.
 * 
 * @param[in] p_data  pointer to the at command in the raw data buffer.
 * @param[in] cmd_len  size, in bytes, of the current at command.
 */
uint8_t atcmd_parse(char cmd_len, char *p_data)
{
	atcmd_extract_cmd(p_data);
	return (atcmd_run_cmd());
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