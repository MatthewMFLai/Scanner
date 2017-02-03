#ifndef UTIL_H__
#define UTIL_H__

#define CONFIG_BYTE_DIGITS_MAX  3
#define CONFIG_WORD_DIGITS_MAX  5 
#define CONFIG_LONGWORD_DIGITS_MAX 10
#define BCD_MULTIPLIER  16
									
uint8_t byte_to_ascii (uint8_t *p_dest, uint8_t value);
uint8_t word_to_ascii (uint8_t *p_dest, uint16_t value);
uint8_t longword_to_ascii (uint8_t *p_dest, uint32_t value);
uint8_t ascii_to_byte (uint8_t *p_data, uint8_t len);
uint16_t ascii_to_word (uint8_t *p_data, uint8_t len);
uint32_t ascii_to_longword (uint8_t *p_data, uint8_t len);
uint8_t ascii_to_bcd (char msn, char lsn);
void big_to_small_endian(uint8_t *p_data, uint8_t len);

#endif  /* _ UTIL_H__ */
