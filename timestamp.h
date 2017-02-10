#ifndef TIMESTAMP_H__
#define TIMESTAMP_H__
			
#define BIT24_CLICKS 16777216			
typedef struct
{
	uint32_t	ts_prev;
	uint32_t 	ovrflw_counter;
} ts_data_t;

uint32_t ts_check_and_set(uint32_t ts_cur);

#endif  /* _ TIMESTAMP_H__ */
