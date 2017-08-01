#ifndef GSM_MSG_IF__
#define GSM_MSG_IF__

#define GSM_MSG_IF_STATE_READY             0
#define GSM_MSG_IF_STATE_WAITLN            1
#define GSM_MSG_IF_STATE_OOS               2

#define GSM_MSG_IF_ACK                     0
#define GSM_MSG_IF_NAK                     1
#define GSM_MSG_IF_TIMEOUT                 2
#define GSM_MSG_IF_DATA                    3
#define GSM_MSG_IF_SEPARATOR               '\0'
#define GSM_MSG_IF_STAR                    '*'
#define GSM_MSG_IF_PREFIX                  '$'
#define GSM_MSG_IF_SUFFIX1                 '\r'
#define GSM_MSG_IF_SUFFIX2                 '\n'
#define GSM_MSG_IF_SUFFIX_LEN              4
typedef struct
{
  void* msg_array;
  uint32_t msg_size;
  uint8_t msg_type;
} gsm_msg_t;

void gsm_msg_if_init(void);
gsm_msg_t *gsm_msg_if_receive(uint8_t *p_data, uint16_t len);

// Support sending mulitple messages in one call. The separator must be
// the \0 byte.
// eg.  <msg>
//      <msg>\0<msg>\0<msg>
// The len parameter must exclude the in between \0 terminator bytes,
// but not the \0 after the very last message.
// Return: 0 message sent, 1 message not sent, busy
uint8_t gsm_msg_if_send(uint8_t *p_data, uint16_t len);
uint8_t gsm_msg_if_reply(uint8_t *p_data, uint16_t len);
uint8_t gsm_msg_if_get_state(void);
void gsm_msg_if_reset(void);
void gsm_msg_if_disconnect(void);

#endif  /* _ GSM_MSG_IF__ */
