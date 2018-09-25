

#include "utils.h"


/**
  * @brief  Add char to the end of string
  * @note   
  * @param  str: pointer to the string
  * @param  ch: char to add
  * @retval None
  */
void str_addChar(uint8_t *str, uint8_t ch)
{
    uint8_t *p = NULL;
    for(p = str; *p != NULL; ++p) {}
    *p = ch; ++p;
    *p = NULL;
}

/**
  * @brief  Add char to the start of string
  * @note   
  * @param  str: pointer to the string
  * @param  ch: char to add at begin
  * @retval None
  */
void str_addBeforeChar(uint8_t *str, uint8_t ch)
{
    uint8_t *p = NULL;
    for(p = str; *p != NULL; ++p) {}
    ++p;
    do {
      *p = *(p-1);
      --p;
    } while (p > str);// {
    *p = ch;
}

/**
  * @brief  Return pointer to the char before NULL in string string
  * @note   
  * @param  str: pointer to the string
  * @param  maxSize: maximum size of string
  * @retval None
  */
uint8_t *str_getLast(uint8_t *str, uint32_t maxSize)
{
    uint8_t *p = NULL;
    for (p = str; (*p != NULL) && maxSize; ++p, --maxSize) {}
    return (--p);
}

uint16_t copy_16bit(uint8_t *in, uint16_t *out, uint16_t out_start) {
    uint16_t len = 0u;
    for (; *(in + len) != 0u; ++len) {
        *(out + out_start + len) = (uint16_t) *(in + len);
    }
    *(out + len) = 0u;
    return len;
}

uint16_t str_concat16(uint16_t *str1, uint16_t *str2) {
    uint16_t len1, len2;
    for (len1 = 0u; *(str1 + len1) != 0u; ++len1) { }
    for (len2 = 0u; *(str2 + len2) != 0u; ++len2) {
        *(str1 + len1 + len2) = *(str2 + len2);
    }
    return (len1 + len2);
}

uint16_t str_concat(uint8_t *str1, uint8_t *str2) {
    uint16_t len1, len2;
    for (len1 = 0u; *(str1 + len1) != 0u; ++len1) { }
    for (len2 = 0u; *(str2 + len2) != 0u; ++len2) {
        *(str1 + len1 + len2) = *(str2 + len2);
    }
    return (len1 + len2);
}

/**
  * @brief  Convert signed integer to string
  * @note   Private
  * @param  n: in value
  * @param  str: pointer to output string buffer
  * @param  max_len: maximum length of string
  * @retval Out string length (0x00 if string too long)
  */
uint16_t int2str(int16_t n, uint8_t *str, uint16_t max_len) {
    uint8_t* p;
    int16_t copyn = 0;
    uint16_t len = 0u;
    
    if (n < 0) {
        *(str++) = '-';
        ++len;
        n *= -1;
    }
    p = str;
    copyn = n;
    do { //Move to where representation ends
        ++p;
        copyn /= 10u;
        ++len;
    } while(copyn);
    if (len > max_len) { return 0u; }
    *p = NULL;
    do { //Move back, inserting digits as u go
        *(--p) = n%10u + '0';
        n /= 10u;
    } while (n && (p >= str));
    return len;
}


///**
//  * @brief  Convert 'float' variable to string
//  * @note   Max. float value is +/- 6553.5, min. +/- 0.1
//  *         If less, out string is "0.0"
//  * @param  infl: pointer to float value
//  * @param  acc: float conver aacuracy
//  *          This parameter can be one of the following values:
//  *            @arg ACCYRACY_ONE: 
//  *            @arg ACCYRACY_INT: 
//  * @param  str: pointer to output string buffer
//  * @param  dec_pos: output position of decimal point
//  *         in out string
//  * @retval Out string length
//  */
//static uint16_t float2str(float *infl, DecimalPointPos acc,
//                          uint8_t *str, uint16_t *dec_pos) {
//    bool neg = FALSE, round = FALSE;
//    uint16_t uint_dg = 0u, len = 0u;
//    
//    if (*(infl) < 0.0) {
//        neg = TRUE;
//        *(str++) = '-';
//        uint_dg = (uint16_t) (-*(infl)*10);
//    }
//    else {
//        uint_dg = (uint16_t) (*(infl)*10);
//    }
//    if (NO_DECIMAL == acc) {
//        if (IS_NEED_ROUNDED(uint_dg)) {
//            uint_dg += 10u;
//        }
//        uint_dg /= 10u;
//    }
//    if (uint_dg < 10u) {
//        *dec_pos = (neg ? 1u : 0u);
//        *(str++) = '0';
//        len = uint2str(uint_dg, str);
//    }
//    else {
//        len = uint2str(uint_dg, str);
//        *dec_pos = len - (neg ? 1u : 2u);
//    }
//    return (len + (neg ? 1u : 0u));
//}

///**
//  * @brief  Convert unsigned integer to string
//  * @note   Private
//  * @param  n: in value
//  * @param  str: pointer to output string buffer
//  * @retval Out string length
//  */
//static uint16_t uint2str(uint16_t n, uint8_t *str) {
//    uint8_t* p = b;
//    uint16_t copyn = n;
//    uint16_t len = 0u;
//    do { //Move to where representation ends
//        ++p;
//        copyn = copyn/10u;
//        ++len;
//    } while(copyn);
//    *p = NULL;
//    do { //Move back, inserting digits as u go
//        *(--p) = n%10u + '0';
//        n /= 10u;
//    } while (n);
//    return len;
//}