
#ifndef ATCMD_H__
#define ATCMD_H__
										
#define APP_ATCMD_LENGTH        0x10
#define APP_ATCMD_PARA_LENGTH	0X10
#define APP_ATCMD_PARA_MAX   	4

#define APP_ATCMD_ACT_ENABLE_SCAN 		0
#define APP_ATCMD_ACT_MODE_0       		1
#define APP_ATCMD_ACT_SCAN_INT     		2
#define APP_ATCMD_ACT_ENABLE_SCAN_READ 	3
#define APP_ATCMD_ACT_MODE_0_READ       4
#define APP_ATCMD_ACT_SCAN_INT_READ     5
#define APP_ATCMD_ACT_CONFIG_GET        6
#define APP_ATCMD_ACT_CONFIG_SET        7
#define APP_ATCMD_ACT_CONFIG_GET_VER    8
#define APP_ATCMD_NOT_SUPPORTED     0xff

#define APP_BUILDING_CODE_LENGTH	0X10
#define APP_WORD_STR_LEN			6  // 5 characters + the \0
#define APP_VERSION_STR_MAX         32
#define APP_ATCMD_MAX_DATA_LEN      1024

typedef struct
{
	char		building_code[APP_BUILDING_CODE_LENGTH];
	char		scan_interval_str[APP_WORD_STR_LEN];
	char		scan_window_str[APP_WORD_STR_LEN];
	char		mode_byte;
	char		enable_byte;
	uint16_t	scan_interval;
	uint16_t 	scan_window;
	uint8_t		mode;
	uint8_t		enable;
	char        version_str[APP_VERSION_STR_MAX];
	uint16_t    config_size;
	char		config_size_str[APP_WORD_STR_LEN];
} atcmd_data_t;

typedef struct
{
	uint8_t		is_str; // 1 => treat as string, 0 => treat as unsigned number
	uint8_t		size; // parameter size in number of bytes
} atcmd_param_desc_t;

void atcmd_init(void);
uint8_t atcmd_parse(uint16_t buffer_len, char *p_data);
void atcmd_get_scan_param(uint16_t *p_interval, uint16_t *p_window);
uint8_t atcmd_scan_enabled(void);
void atcmd_reply_scan(void);
void atcmd_reply_mode(void);
void atcmd_reply_scanint(void);
void atcmd_reply_config(void);
void atcmd_reply_config_ver(void);
void atcmd_reply_ok(void);
void atcmd_reply_nack(void);
void atcmd_report_in(void);
void atcmd_report_out(void);
#endif  /* _ ATCMD_H__ */
