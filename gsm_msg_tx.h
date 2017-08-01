#ifndef GSM_MSG_TX__
#define GSM_MSG_TX__

#define GSM_MSG_BUFF_SIZE               256
#define GSM_MSG_STATUS_CODE_0           '0'

void gsm_msg_position_update(uint8_t *p_locatorid);
void gsm_msg_status_update(char status_code, uint8_t *p_locatorid);

#endif  /* _ GSM_MSG_TX__ */
