#ifndef GSM_MSG__
#define GSM_MSG__

#include "gsm_msg_if.h"

void gsm_msg_init(void);
void gsm_msg_handshake_start(void);
void gsm_msg_receive(gsm_msg_t *p_msg);
void gsm_msg_reset(void);

#endif  /* _ GSM_MSG__ */
