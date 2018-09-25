

#ifndef __UTILS_H
#define __UTILS_H

#include "stm8l15x.h"

#ifdef __cplusplus
 extern "C" {
#endif

#ifndef NULL
#define NULL ((uint8_t) 0x00)
#endif

void str_addChar(uint8_t *str, uint8_t ch);
void str_addBeforeChar(uint8_t *str, uint8_t ch);
uint8_t *str_getLast(uint8_t *str, uint32_t maxSize);
uint16_t int2str(int16_t n, uint8_t *str, uint16_t max_len);


uint16_t copy_16bit(uint8_t *in, uint16_t *out, uint16_t out_start);
uint16_t str_concat16(uint16_t *str1, uint16_t *str2);
uint16_t str_concat(uint8_t *str1, uint8_t *str2);



#ifdef __cplusplus
}
#endif

#endif /* __UTILS_H */