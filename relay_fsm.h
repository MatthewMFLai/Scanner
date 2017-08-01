#ifndef RELAY_FSM_H__
#define RELAY_FSM_H__

#define RELAY_FSM_STATE_READY                        0
#define RELAY_FSM_STATE_WAIT_LN_RELAY_ACK            1
#define RELAY_FSM_STATE_WAIT_LN_ACK                  2
#define RELAY_FSM_STATE_WAIT_MESH_RELAY_ACK          3
#define RELAY_FSM_STATE_WAIT_MESH_ACK                4

#define RELAY_FSM_RC_OK                              0
#define RELAY_FSM_NO_GSM                             1

void relay_fsm_init(void);
void relay_fsm_reset(void);
uint8_t relay_fsm_process_rbc(uint8_t *s, uint16_t len, uint16_t node_id);
uint8_t relay_fsm_process_gsm(uint8_t *s, uint16_t len);

#endif  /* _ RELAY_FSM_H__ */
