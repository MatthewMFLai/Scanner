#ifndef CONFIG_HDLR_H__
#define CONFIG_HDLR_H__
										
#define CONFIG_MAX_PARAMS       60      // 12 for scanner, 12 for each beacon
#define CONFIG_KEY_LEN          4 
#define CONFIG_SIZE_LEN         4 
#define CONFIG_VALUE_LEN        32 
#define CONFIG_LIMITER          0x0A
#define CONFIG_EQUAL            '='
#define CONFIG_BYTE_DIGITS_MAX  3
#define CONFIG_WORD_DIGITS_MAX  5 
#define CONFIG_LONGWORD_DIGITS_MAX 10 

typedef struct
{
    uint8_t     	key[CONFIG_KEY_LEN];
    uint32_t     	len;
    uint8_t      	value[CONFIG_VALUE_LEN];
} config_hdlr_t;

void config_hdlr_init(void);

// Parse the ascii config file supplied by the calling routine.
void config_hdlr_parse(uint16_t len, uint8_t *p_data);
bool config_hdlr_get_string(char *p_key, uint16_t *p_len, char *p_dest);
bool config_hdlr_get_byte(char *p_key, uint8_t *p_dest);
bool config_hdlr_get_word(char *p_key, uint16_t *p_dest);
bool config_hdlr_get_longword(char *p_key, uint32_t *p_dest);
uint16_t config_hdlr_build(uint8_t *p_dest);
bool config_hdlr_set_string(char *p_key, uint16_t len, char *p_src);
bool config_hdlr_set_byte(char *p_key, uint8_t value);
bool config_hdlr_set_word(char *p_key, uint16_t value);
bool config_hdlr_set_longword(char *p_key, uint32_t value);
#endif  /* _ CONFIG_HDLR_H__ */
