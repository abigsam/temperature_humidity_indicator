/**
  ******************************************************************************
  * @file    delay.h
  * @author  Microcontroller Division
  * @version V1.2.0
  * @date    09/2010
  * @brief   delay functions header
  ******************************************************************************
  * @copy
  *
  * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
  * TIME. AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY
  * DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
  * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
  * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  * <h2><center>&copy; COPYRIGHT 2010 STMicroelectronics</center></h2>
  */

#ifndef _DELAY_H_
#define _DELAY_H_
    
#include "stm8l15x.h"
#include "stm8l15x_clk.h"

#define HSI_HZ                          (16000000UL)
#define SYSCLK_DIV                      (16)//8)
#define DELAYMS_TIM_PREDIV_POV          ((uint8_t) 2)//4)   //6 for SYSCLK_DIV = 1
#define DELAYMS_TIM_RELOAD_VAL          ((uint8_t) (((HSI_HZ/1000U)/SYSCLK_DIV) >> DELAYMS_TIM_PREDIV_POV))     //250 for SYSCLK_DIV = 1
#define DELAY10US_TIM_PREDIV_POV        ((uint8_t) 0)
#define DELAY10US_TIM_RELOAD_VAL        ((uint8_t) (((HSI_HZ/100000UL)/SYSCLK_DIV) >> DELAY10US_TIM_PREDIV_POV)) //160 for SYSCLK_DIV = 1

#define TRH_LSI_FREQ_HZ         (33000U) //LSI frequency, Hz
#define WAIT_WCNT_WRITE_READY() { while (SET != RTC_GetFlagStatus(RTC_FLAG_WUTWF)) { } }
#define WCNT_MAX_S              ((uint16_t) 20)
#define WCNT_MAX_MS             ((uint16_t) 3900) // WKUP_MAX(65535) / [LSI(33000)/RTC_DIV(2)] ~= 3900
#define GET_WCNT_SEC(v)         ( (uint16_t) ( ((uint32_t)v * (uint32_t)TRH_LSI_FREQ_HZ) / 16 ) )
#define GET_WCNT_MS(v)          ( (uint16_t) ( (v * (TRH_LSI_FREQ_HZ / 1000)) / 2) )
    
void initRTCwakeup(void);
static void sleep_wcnt(uint16_t wcnt_val);
    
//------------------------------------------------------------------------------
// Function Name : delay_ms
// Description   : delay for some time in ms unit
// Input         : n_ms is how many ms of time to delay
//------------------------------------------------------------------------------
void delay_ms(u16 n_ms) ;


//------------------------------------------------------------------------------
// Function Name : delay_10us
// Description   : delay for some time in 10us unit(partial accurate)
// Input         : n_10us is how many 10us of time to delay
//------------------------------------------------------------------------------
void delay_10us(u16 n_10us);

void delay_lowp_ms(u16 n_ms);
void delay_lowp_intr(void);

void sleep_s(uint32_t time_s);
void sleep_ms(uint32_t time_ms);

/******************* (C) COPYRIGHT 2010 STMicroelectronics *****END OF FILE****/
#endif //_DELAY_H_