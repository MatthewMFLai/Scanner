#ifndef RSSI_FILTER_H__
#define RSSI_FILTER_H__

#define RSSI_FILTER_MAX_NODES       9
#define RSSI_FILTER_MAX_SAMPLES     16
#define RSSI_FILTER_MIN_RSSI_VALUE  -127

void rssi_filter_init(void);
void rssi_filter_update(uint16_t node_id, int8_t rssi, uint32_t ts);
uint16_t rssi_filter_report_best(void);

#endif  /* _ RSSI_FILTER_H__ */
