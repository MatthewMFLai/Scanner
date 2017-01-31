#ifndef PSTORE_H__
#define PSTORE_H__
										
#define PSTORE_MAX_BLOCK       512  // 512 bytes to hold ascii config file

bool pstore_init(void);
uint16_t pstore_get(uint8_t *p_dest);
bool pstore_set(uint8_t *p_src, uint16_t len);
#endif  /* _ PSTORE_H__ */
