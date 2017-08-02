#ifndef GSM_MSG_TX__
#define GSM_MSG_TX__

#define GSM_MSG_BUFF_SIZE               256
#define GSM_MSG_STATUS_CODE_0           '0'

#define GSM_MSG_GEOFENCE_LOCATOR_MARKER    7

void gsm_msg_position_update(uint8_t *p_locatorid);
void gsm_msg_multi_position_update(uint8_t *p_locatorid, uint8_t node_id, char status_code);
void gsm_msg_status_update(char status_code, uint8_t *p_locatorid);
void gsm_msg_raw_send(uint8_t *p_msg, uint16_t len);
void gsm_msg_geofence_delete_ack(uint8_t *p_locatorid);

#endif  /* _ GSM_MSG_TX__ */
