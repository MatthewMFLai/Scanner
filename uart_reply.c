#include <stdint.h>
#include <string.h>
#include "uart_reply.h"
#include "app_uart.h"


void uart_reply_byte(uint8_t data)
{
	while(app_uart_put(data) != NRF_SUCCESS);
}

/**@brief Function to store the current at command in the at command table.
 * @details Use the built-in h/w encryption engine.
 * 
 * @param[in] p_data  pointer to the at command in the raw data buffer.
 * @param[in] cmd_len  size, in bytes, of the current at command.
 */
void uart_reply_string(char *p_str)
{
	for (uint16_t i = 0; i < strlen(p_str); i++)
	{
		while(app_uart_put(p_str[i]) != NRF_SUCCESS);
	}
}