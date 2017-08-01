#ifndef GSM_COMM__
#define GSM_COMM__

#define GSM_COMM_STATE_CREG              0
#define GSM_COMM_STATE_AT                1
#define GSM_COMM_STATE_CONTEXT           2
#define GSM_COMM_STATE_CONNECTION        3
#define GSM_COMM_STATE_SOCKET            4
#define GSM_COMM_STATE_CONNECTED         5
#define GSM_COMM_STATE_DELAY1            6

void gsm_comm_init(void);
void gsm_comm_poweron(void);
void gsm_comm_fsm_start(void);
void gsm_comm_fsm_run(uint8_t *p_data, uint16_t len);
uint8_t gsm_comm_fsm_get_state(void);
void gsm_comm_fsm_reset(void);
#endif  /* _ GSM_COMM__ */
