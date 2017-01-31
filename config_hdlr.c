#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "config_hdlr.h"

static config_hdlr_t config_data[CONFIG_MAX_PARAMS];
static uint16_t m_param_max;

static uint8_t byte_to_ascii (uint8_t *p_dest, uint8_t value)
{
    uint8_t quotient;
    uint8_t rc = 0;

    for (uint8_t i = 0, divisor = 100; i < CONFIG_BYTE_DIGITS_MAX; i++)
    {
	quotient = value / divisor;
	value = value % divisor;
	*p_dest++ = quotient + 0x30;
	rc++;
	divisor = divisor / 10;
    }
    return (rc);
}

static uint8_t word_to_ascii (uint8_t *p_dest, uint16_t value)
{
    uint16_t divisor = 10000;
    uint8_t quotient;
    uint8_t rc = 0;

    for (uint8_t i = 0; i < CONFIG_WORD_DIGITS_MAX; i++)
    {
	quotient = value / divisor;
	value = value % divisor;
	*p_dest++ = quotient + 0x30;
	rc++;
	divisor = divisor / 10;
    }
    return (rc);
}

static uint8_t longword_to_ascii (uint8_t *p_dest, uint32_t value)
{
    uint32_t divisor = 1000000000;
    uint8_t quotient;
    uint8_t rc = 0;

    for (uint8_t i = 0; i < CONFIG_LONGWORD_DIGITS_MAX; i++)
    {
	quotient = value / divisor;
	value = value % divisor;
	*p_dest++ = quotient + 0x30;
	rc++;
	divisor = divisor / 10;
    }
    return (rc);
}

static uint8_t ascii_to_byte (uint8_t *p_data, uint8_t len)
{
    uint8_t i;
    uint8_t rc = 0;

    for (i = 0; i < len; i++)
    {
	rc = (rc * 10) + *(p_data + i) - 0x30;
    }
    return (rc);
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

static uint32_t ascii_to_longword (uint8_t *p_data, uint8_t len)
{
    uint8_t i;
    uint32_t rc = 0;

    for (i = 0; i < len; i++)
    {
	rc = (rc * 10) + *(p_data + i) - 0x30;
    }
    return (rc);
}

static uint16_t getnext_char(char token, uint16_t cur_idx, uint8_t *p_data)
{
    uint16_t idx;
    uint16_t idx_stop = cur_idx + sizeof(config_hdlr_t);

    for (idx = cur_idx; idx < idx_stop; idx++)
    {
	if (*(p_data + idx) == token)
	   break;
    }
    if (idx != idx_stop)
	return (idx);
    else
	return 0;
}

void config_hdlr_init(void)
{
    uint8_t i;
    for (i = 0; i < CONFIG_MAX_PARAMS; i++)
	memset(&config_data[i], 0, sizeof(config_hdlr_t));
    m_param_max = 0;
}

// Data file must be ascii file with the format
// <key>=<value><0x0A><key>=<value><0x0A>....
// The len should include the 0x0A limiter bytes
void config_hdlr_parse(uint16_t len, uint8_t *p_data)
{
    uint8_t toloop = 1;
    uint16_t cur_idx = 0;
    uint16_t limit_idx = 0;
    uint16_t equal_idx = 0;
    uint16_t param_idx = 0;

    while (toloop && cur_idx < len)
    {
	// Get the next limiter index.
	limit_idx = getnext_char(CONFIG_LIMITER, cur_idx, p_data);
	equal_idx = getnext_char(CONFIG_EQUAL, cur_idx, p_data);
	memcpy(config_data[param_idx].key, p_data + cur_idx, equal_idx - cur_idx);
	memcpy(config_data[param_idx].value, p_data + equal_idx + 1, limit_idx - equal_idx - 1);
	config_data[param_idx].len = limit_idx - equal_idx - 1;
	param_idx++;
	cur_idx = limit_idx + 1;
    }
    m_param_max = param_idx;
}

// The value string, once found, will be copied into the passed in buffer, with the
// terminating /0. The length of the string (exclude terminating \0) is set in *p_len. 
bool config_hdlr_get_string(char *p_key, uint16_t *p_len, char *p_dest)
{
    uint16_t idx = 0;
    while (idx < CONFIG_MAX_PARAMS)
    {
	if (memcmp(config_data[idx].key, p_key, CONFIG_KEY_LEN) != 0)
	    continue;
	
	memcpy(p_dest, config_data[idx].value, config_data[idx].len);
	*(p_dest + config_data[idx].len) = '\0';
	*p_len = config_data[idx].len;
	return true;
    }
    return false;
}

bool config_hdlr_get_byte(char *p_key, uint8_t *p_dest)
{
    uint16_t idx = 0;
    while (idx < CONFIG_MAX_PARAMS)
    {
	if (memcmp(config_data[idx].key, p_key, CONFIG_KEY_LEN) != 0)
	    continue;
	if (config_data[idx].len > CONFIG_BYTE_DIGITS_MAX)	
	    return false;

	*p_dest = ascii_to_byte(config_data[idx].value, config_data[idx].len);
	return true;
    }
    return false;
}

bool config_hdlr_get_word(char *p_key, uint16_t *p_dest)
{
    uint16_t idx = 0;
    while (idx < CONFIG_MAX_PARAMS)
    {
	if (memcmp(config_data[idx].key, p_key, CONFIG_KEY_LEN) != 0)
	    continue;
	
	if (config_data[idx].len > CONFIG_WORD_DIGITS_MAX)	
	    return false;

	*p_dest = ascii_to_word(config_data[idx].value, config_data[idx].len);
	return true;
    }
    return false;
}

bool config_hdlr_get_longword(char *p_key, uint32_t *p_dest)
{
    uint16_t idx = 0;
    while (idx < CONFIG_MAX_PARAMS)
    {
	if (memcmp(config_data[idx].key, p_key, CONFIG_KEY_LEN) != 0)
	    continue;
	
	if (config_data[idx].len > CONFIG_LONGWORD_DIGITS_MAX)	
	    return false;

	*p_dest = ascii_to_longword(config_data[idx].value, config_data[idx].len);
	return true;
    }
    return false;
}

uint16_t config_hdlr_build(uint8_t *p_dest)
{
    uint16_t idx;
    uint16_t rc = 0;
    for (idx = 0; idx < m_param_max; idx++)
    {
	memcpy(p_dest, config_data[idx].key, CONFIG_KEY_LEN);
	p_dest += CONFIG_KEY_LEN;
	*p_dest++ = CONFIG_EQUAL;
	memcpy(p_dest, config_data[idx].value, config_data[idx].len);
        p_dest += config_data[idx].len;
	*p_dest++ = CONFIG_LIMITER;
	rc += (CONFIG_KEY_LEN + config_data[idx].len + 2);
    }
    return (rc);
}

// The value string, once found, will be copied into the passed in buffer, with the
// terminating /0. The length of the string (exclude terminating \0) is set in *p_len. 
bool config_hdlr_set_string(char *p_key, uint16_t len, char *p_src)
{
    uint16_t idx = 0;

    if (len > CONFIG_VALUE_LEN)
	return false;

    while (idx < CONFIG_MAX_PARAMS)
    {
	if (memcmp(config_data[idx].key, p_key, CONFIG_KEY_LEN) != 0)
	    continue;

	memset(config_data[idx].value, 0, CONFIG_KEY_LEN);	
	memcpy(config_data[idx].value, p_src, len);
	config_data[idx].len = len;
	return true;
    }
    return false;
}
bool config_hdlr_set_byte(char *p_key, uint8_t value)
{
    uint16_t idx = 0;
    while (idx < CONFIG_MAX_PARAMS)
    {
	if (memcmp(config_data[idx].key, p_key, CONFIG_KEY_LEN) != 0)
	    continue;

	config_data[idx].len = byte_to_ascii(config_data[idx].value, value); 
	return true;
    }
    return false;
}

bool config_hdlr_set_word(char *p_key, uint16_t value)
{
    uint16_t idx = 0;
    while (idx < CONFIG_MAX_PARAMS)
    {
	if (memcmp(config_data[idx].key, p_key, CONFIG_KEY_LEN) != 0)
	    continue;

	config_data[idx].len = word_to_ascii(config_data[idx].value, value); 
	return true;
    }
    return false;
}

bool config_hdlr_set_longword(char *p_key, uint32_t value)
{
    uint16_t idx = 0;
    while (idx < CONFIG_MAX_PARAMS)
    {
	if (memcmp(config_data[idx].key, p_key, CONFIG_KEY_LEN) != 0)
	    continue;

	config_data[idx].len = longword_to_ascii(config_data[idx].value, value); 
	return true;
    }
    return false;
}
