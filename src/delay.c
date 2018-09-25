/**
  ******************************************************************************
  * @file    delay.c
  * @author  Microcontroller Division
  * @version V1.2.0
  * @date    09/2010
  * @brief   delay functions
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

/* Includes ------------------------------------------------------------------*/
#include "delay.h"

    
/**
  * @brief  Init RTC for work with LSI
  * @note   Private
  * @param  None
  * @retval None
  */
void initRTCwakeup(void) {
    RTC_InitTypeDef RTC_InitStruct;
    RTC_TimeTypeDef RTC_TimeStruct;
    /* RTC Domain Access */
    CLK_RTCClockConfig(CLK_RTCCLKSource_LSI, CLK_RTCCLKDiv_1);
    CLK_PeripheralClockConfig(CLK_Peripheral_RTC, ENABLE);
    /* Init RTC */
    RTC_StructInit(&RTC_InitStruct);
    RTC_Init(&RTC_InitStruct);
    RTC_TimeStructInit(&RTC_TimeStruct);
    RTC_SetTime(RTC_Format_BCD, &RTC_TimeStruct);
    /* Configure Wakeup counter */
    RTC_WakeUpCmd(DISABLE);
    WAIT_WCNT_WRITE_READY();
    RTC_WakeUpClockConfig(RTC_WakeUpClock_RTCCLK_Div16); // RTC_clock / 16
}

/**
  * @brief  Sleep for specified Wake up counter ticks
  * @param  wcnt_val: value that loaded to Wake up counter
  * @retval None 
  */
static void sleep_wcnt(uint16_t wcnt_val)
{
  RTC_WakeUpCmd(DISABLE);           //Disable wakeup counter
  WAIT_WCNT_WRITE_READY();          //Wait for write ready
  RTC_SetWakeUpCounter(wcnt_val);  //Update wakeup counter
  RTC_ITConfig(RTC_IT_WUT, ENABLE); //Enable wakeup counter interrupt
  RTC_WakeUpCmd(ENABLE);            //Enable wakeup counter
  enableInterrupts();
  halt();
  disableInterrupts();
  RTC_ITConfig(RTC_IT_WUT, DISABLE); //Disable wakeup counter interrupt
  RTC_WakeUpCmd(DISABLE);            //Disable wakeup counter
}


/**
  * @brief  Activate sleep mode for specified time in s
  * @note   Public
  * @param  time_s: Time in s for sleep
  * @retval None
  */
void sleep_s(uint32_t time_s) {
    uint16_t time_wcnt = 0;
    //
    RTC_WakeUpCmd(DISABLE);
    WAIT_WCNT_WRITE_READY();
    RTC_WakeUpClockConfig(RTC_WakeUpClock_RTCCLK_Div16); // RTC_clock / 16
    //
    do {
        if (time_s > WCNT_MAX_S) {
          //time_wcnt = WCNT_ONE_SECOND_DIV16 * WCNT_MAX_S;
          time_wcnt = GET_WCNT_SEC(WCNT_MAX_S); // (20 * 33000)/16
          time_s -= WCNT_MAX_S;
        }
        else {
          //time_wcnt = WCNT_ONE_SECOND_DIV16 * time_s;
          time_wcnt = GET_WCNT_SEC(time_s);
        }
        //
        sleep_wcnt(time_wcnt);
    } while (time_s > WCNT_MAX_S);
}

/**
  * @brief  Activate sleep mode for specified time in ms
  * @note   Public
  * @param  time_ms: Time in ms for sleep
  * @retval None
  */
void sleep_ms(uint32_t time_ms)
{
    uint16_t time_wcnt = 0;
    //
    RTC_WakeUpCmd(DISABLE);
    WAIT_WCNT_WRITE_READY();
    RTC_WakeUpClockConfig(RTC_WakeUpClock_RTCCLK_Div2); // RTC_clock / 2
    //
    do {
        if (time_ms > WCNT_MAX_MS) {
          time_wcnt = GET_WCNT_MS(WCNT_MAX_MS); // 3900 * ((33000 / DIV2(2)) / 1000)
          time_ms -= WCNT_MAX_MS;
        }
        else {
          time_wcnt = GET_WCNT_MS(time_ms);
        }
        //
        sleep_wcnt(time_wcnt);
    } while (time_ms > WCNT_MAX_MS);
}
    
    
    
/**
  * @brief  delay for some time in ms unit
  * @caller auto_test
  * @param  n_ms is how many ms of time to delay
  * @retval None
  */
void delay_ms(u16 n_ms)
{
  /* Init TIMER 4 */
  CLK_PeripheralClockConfig(CLK_Peripheral_TIM4, ENABLE);

  /* Init TIMER 4 prescaler: / (2^6) = /64 */
  //TIM4->PSCR = 6;
  TIM4->PSCR = DELAYMS_TIM_PREDIV_POV;

  /* HSI div by 1 --> Auto-Reload value: 16M / 64 = 1/4M, 1/4M / 1k = 250*/
  //TIM4->ARR = 250;
  TIM4->ARR = DELAYMS_TIM_RELOAD_VAL;
  
  /* Counter value: 2, to compensate the initialization of TIMER*/
  TIM4->CNTR = 2;

  /* clear update flag */
  TIM4->SR1 &= ~TIM4_SR1_UIF;

  /* Enable Counter */
  TIM4->CR1 |= TIM4_CR1_CEN;

  while(n_ms--)
  {
    while((TIM4->SR1 & TIM4_SR1_UIF) == 0) ;
    TIM4->SR1 &= ~TIM4_SR1_UIF;
  }

  /* Disable Counter */
  TIM4->CR1 &= ~TIM4_CR1_CEN;
}

/**
  * @brief  delay for some time in 10us unit(partial accurate)
  * @caller auto_test
  * @param n_10us is how many 10us of time to delay
  * @retval None
  */
void delay_10us(u16 n_10us)
{
  /* Init TIMER 4 */
  CLK_PeripheralClockConfig(CLK_Peripheral_TIM4, ENABLE);

  /* prescaler: / (2^0) = /1 */
  //TIM4->PSCR = 0;
  TIM4->PSCR = DELAY10US_TIM_PREDIV_POV;

  /* SYS_CLK_HSI_DIV1 Auto-Reload value: 16M / 1 = 16M, 16M / 100k = 160 */
  //TIM4->ARR = 160;
  TIM4->ARR = DELAY10US_TIM_RELOAD_VAL;

  /* Counter value: 10, to compensate the initialization of TIMER */
  TIM4->CNTR = 10;

  /* clear update flag */
  TIM4->SR1 &= ~TIM4_SR1_UIF;

  /* Enable Counter */
  TIM4->CR1 |= TIM4_CR1_CEN;

  while(n_10us--)
  {
    while((TIM4->SR1 & TIM4_SR1_UIF) == 0) ;
    TIM4->SR1 &= ~TIM4_SR1_UIF;
  }

  /* Disable Counter */
  TIM4->CR1 &= ~TIM4_CR1_CEN;
 CLK_PeripheralClockConfig(CLK_Peripheral_TIM4, DISABLE);

}

void delay_lowp_ms(u16 n_ms)
{
  /* Init TIMER 4 */
  CLK_PeripheralClockConfig(CLK_Peripheral_TIM4, ENABLE);

  /* Init TIMER 4 prescaler: / (2^6) = /64 */
  //TIM4->PSCR = 6;
  TIM4->PSCR = DELAYMS_TIM_PREDIV_POV;

  /* HSI div by 1 --> Auto-Reload value: 16M / 64 = 1/4M, 1/4M / 1k = 250*/
  //TIM4->ARR = 250;
  TIM4->ARR = DELAYMS_TIM_RELOAD_VAL;
  
  /* Counter value: 2, to compensate the initialization of TIMER*/
  TIM4->CNTR = 2;

  /* Enable TIM4 interrupt */
  TIM4->IER |= TIM4_IER_UIE;
  
  /* clear update flag */
  TIM4->SR1 &= ~TIM4_SR1_UIF;
  
  /* Enable Counter */
  TIM4->CR1 |= TIM4_CR1_CEN;
  
  
  enableInterrupts();
  
  while(n_ms--)
  {
    wfi();
  }
  disableInterrupts();

  /* Disable TIM4 interrupt */
  TIM4->IER &= ~TIM4_IER_UIE;
  
  /* Disable Counter */
  TIM4->CR1 &= ~TIM4_CR1_CEN;
}


void delay_lowp_intr(void)
{
  if (TIM4->SR1 & TIM4_SR1_UIF) {
    TIM4->SR1 &= ~TIM4_SR1_UIF;
  }
}

/******************* (C) COPYRIGHT 2010 STMicroelectronics *****END OF FILE****/
