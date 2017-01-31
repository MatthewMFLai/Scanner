
#ifndef ATCMD_H__
#define ATCMD_H__
										
#define APP_ATCMD_LENGTH        0x10
#define APP_ATCMD_PARA_LENGTH	0X10
#define APP_ATCMD_PARA_MAX   	4

#define APP_ATCMD_ACT_ENABLE_SCAN 	0
#define APP_ATCMD_ACT_DISABLE_SCAN 	1
#define APP_ATCMD_ACT_MODE_0       	2
#define APP_ATCMD_ACT_MODE_1       	3
#define APP_ATCMD_ACT_SCAN_INT     	4

#define APP_BUILDING_CODE_LENGTH	0X10

typedef struct
{
	char		building_code[APP_BUILDING_CODE_LENGTH];
	uint16_t	scan_interval;
	uint16_t 	scan_window;
	uint8_t		mode;
	uint8_t		enable;
} atcmd_data_t;

typedef struct
{
	uint8_t		size; // parameter size in number of bytes
	uint8_t		is_str; // 1 => treat as string, 0 => treat as unsigned number
	uint8_t		ascii_rep; // 1 => number represented with ascii, 0 => binary number content
} atcmd_param_desc_t;

void atcmd_init(void);
uint8_t atcmd_parse(char cmd_len, char *p_data);
void atcmd_reply_ok(void);
void atcmd_reply_nack(void);
void atcmd_report_in(void);
void atcmd_report_out(void);
#endif  /* _ ATCMD_H__ */
