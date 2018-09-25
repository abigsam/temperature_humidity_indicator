/*


*/

#include "stm8l_low_power.h"


//Private variables
LowPowerModes_td currentMode = MCU_RUN;


/*--------------------------------------------------------
--- Private functions
--------------------------------------------------------*/
#if (LP_USE_LOW_POWER_RUN == 1)
/**
  * @brief  User low power RAM program
  * @note   Executed from RAM
  * @param  None
  * retval  None
  */
/* Begin Section LPRUN */ 	
#ifdef _COSMIC_
#pragma section (LPRUN)
void	User_LPRAM(void)
#endif
#ifdef _RAISONANCE_
void	User_LPRAM(void) inram
#endif
#ifdef _IAR_
#pragma location="MY_RAM_FUNC"
void	User_LPRAM(void)
#endif
{
    /* User code begin */
    
    nop();
    nop();
    nop();
    nop();
    
    /* User code end */
}
/* End Section LPRUN */
#ifdef _COSMIC_
#pragma section ()
#endif


/**
  * @brief  Low power wait mode
  * @note   Executed from RAM
  * @param  None
  * retval  None
  */
/* Begin Section LPRUN */ 	
#ifdef _COSMIC_
#pragma section (LPRUN)
void	LPRAM_WaitMode(void)
#endif
#ifdef _RAISONANCE_
void	LPRAM_WaitMode(void) inram
#endif
#ifdef _IAR_
#pragma location="MY_RAM_FUNC"
void	LPRAM_WaitMode(void)
#endif
{
    nop();
    nop();
    wfe();
    nop();
    nop();
}
/* End Section LPRUN */
#ifdef _COSMIC_
#pragma section ()
#endif


/**
  * @brief  Init low power run mode
  * @note   
  * @param
  * retval  None
  */
static void LowPowerRunInit(mode) {
    /* Read & save current divider value */
    uint8_t tmp = (uint8_t)(CLK->CKDIVR & CLK_CKDIVR_CKM);
    
    /*Switch the clock to LSE and disable HSI*/
#if (LP_USE_LSE == 1)
    //CLK_SYSCLKDivConfig(CLK_SYSCLKDiv_1);
    CLK_SYSCLKSourceConfig(CLK_SYSCLKSource_LSE);	
    CLK_SYSCLKSourceSwitchCmd(ENABLE);
    while (((CLK->SWCR)& 0x01)==0x01);
#else
    CLK_SYSCLKDivConfig(CLK_SYSCLKDiv_1);
    CLK_SYSCLKSourceConfig(CLK_SYSCLKSource_LSI);
    CLK_SYSCLKSourceSwitchCmd(ENABLE);
    while (((CLK->SWCR)& 0x01)==0x01);
#endif

#if (LP_USE_HSE == 1)
    CLK_HSEConfig(CLK_HSE_OFF);
#else    
    CLK_HSICmd(DISABLE);
#endif

    if (LP_RUN_MODE == mode) {
/* To copy function LPR_Ram in RAM section LPRUN*/
#ifdef _COSMIC_
  if (!(_fctcpy('L')))
    while(1);
#endif
        User_LPRAM();
    }
    else {
/* To copy function LPR_Ram in RAM section LPRUN*/
#ifdef _COSMIC_
  if (!(_fctcpy('L')))
    while(1);
#endif
        LPRAM_WaitMode();
    }
    
    /* Low power end, restore CLK configuration */
    CLK_SYSCLKDivConfig((CLK_SYSCLKDiv_TypeDef)tmp);
    
#if (LP_USE_HSE == 1)
    /*Switch the clock to HSE*/
    CLK_HSEConfig(CLK_HSE_ON);
    while(((CLK->ECKCR) & CLK_ECKCR_HSERDY) != CLK_ECKCR_HSERDY) {}
    CLK_SYSCLKSourceConfig(CLK_SYSCLKSource_HSE);
    CLK_SYSCLKSourceSwitchCmd(ENABLE);
    while (((CLK->SWCR)& 0x01)==0x01);
#else    
    /*Switch the clock to HSI*/
    CLK_HSICmd(ENABLE);
    while (((CLK->ICKCR)& CLK_ICKCR_HSIRDY)!= CLK_ICKCR_HSIRDY) {}
    CLK_SYSCLKSourceConfig(CLK_SYSCLKSource_HSI);
    CLK_SYSCLKSourceSwitchCmd(ENABLE);
    while (((CLK->SWCR)& 0x01)==0x01);
#endif
    
}

#endif /* if (LP_USE_LOW_POWER_RUN == 1) */

/*--------------------------------------------------------
--- Public functions
--------------------------------------------------------*/

/**
  * @brief  Enable low power mode
  * @note   Public
  * @param  mode: Low power mode, can be:
  *          This parameter can be one of the following values:
  *            @arg MCU_RUN : normal run mode
  *            @arg WAIT_INTERRUPT : Wait for interrupt, wakeup with all internal or
  *                                  external interrupts, reset
  *            @arg WAIT_EVENT : Wait for event, wakeup with all internal or
  *                              external interrupts, wakeup events, reset
  *            @arg LP_RUN_MODE : Low power run mode, wakeup with software sequence, reset
  *            @arg LP_WAIT_MODE : Low power wait mode, wakeup with internal/external
  *                                event, reset
  *            @arg ACTIVE_HALT : wakeup with RTC or external interrupt, reset
  *            @arg HALT : wakeup only with external interrupt or reset
  * @retval None
  */
void LowPowerEnable(LowPowerModes_td mode) {
    
    currentMode = mode;
    switch(mode) {
        
        case MCU_RUN:
        break;
        
        /* Wait for interrupt */
        case WAIT_INTERRUPT:
            disableInterrupts();
            GPIOwaitConfig();
            RunBeforeLowPowerMode(mode);
            //
            wfi();
        break;
        
        /* Wait for event */
        case WAIT_EVENT:
            disableInterrupts();
            GPIOwaitConfig();
            RunBeforeLowPowerMode(mode);
            //
            wfe();
        break;

#if (LP_USE_LOW_POWER_RUN == 1)        
        /* Low power run */
        case LP_RUN_MODE:
            PWR_UltraLowPowerCmd(ENABLE);
            disableInterrupts();
            GPIOlowPowerConfig();
            RunBeforeLowPowerMode(mode);
            //
            LowPowerRunInit(mode);
        break;
        
        /* Low power wait mode */
        case LP_WAIT_MODE:
            PWR_UltraLowPowerCmd(ENABLE);
            disableInterrupts();
            GPIOlowPowerConfig();
            RunBeforeLowPowerMode(mode);
            //
            LowPowerRunInit(mode);
        break;
#endif
        
        /* Enter active HAL mode */
        case ACTIVE_HALT:
        
            //disableInterrupts();
            GPIOlowPowerConfig();
            RunBeforeLowPowerMode(mode);
            //
            halt();
        break;
        
        /* Active HALT mode */
        case HALT:
        
            //disableInterrupts();
            GPIOlowPowerConfig();
            RunBeforeLowPowerMode(mode);
            //
            halt();
        break;
        
    }
    
}

/**
  * @brief  Sequence for exit low power modes
  * @note   Public
  * @param  void
  * @retval ?
  */
uint8_t LowPowerExit(void) {
    RunAfterExitLowPowerMode(currentMode);
    GPIOlowPowerExitConfig();
    enableInterrupts();
    currentMode = MCU_RUN;
    return 0u;
}

///**
//  * @brief  Init RTC for used in ACTIVE_HALT
//  * @note   Public
//  * @param  clockType: use LSI or LSE as clock source for RTC
//  *          This parameter can be one of the following values:
//  *            @arg LP_RTC_USE_LSI : use as source clock LSI
//  *            @arg LP_RTC_USE_LSE : use as source clock LSE
//  * @retval None
//  */
//void LowPower_RTCinit(LowPowerRTCclock_td clockType) {
//    /* When used LSE */
//    
//    /* When used LSI */
//
//}

/*--------------------------------------------------------
--- Public weak functions
--------------------------------------------------------*/

/**
  * @brief  Configure GPIOs to low power/HALT mode
  * @note   Public & weak
  * @param  None
  * @retval None
  */
__weak void GPIOlowPowerConfig(void) {
    nop();
}

/**
  * @brief  Configure GPIOs to wait mode
  * @note   Public & weak
  * @param  None
  * @retval None
  */
__weak void GPIOwaitConfig(void) {
    nop();
}

/**
  * @brief  Configure GPIOs after exit from low power
  * @note   Public & weak
  * @param  None
  * @retval None
  */
__weak void GPIOlowPowerExitConfig(void) {
    nop();
}

/**
  * @brief  Code have to be run before enter Low Power mode
  *         There low power exit condition are programmed
  * @note   Public & weak
  * @param  mode: Low power mode
  * @retval None
  */
__weak void RunBeforeLowPowerMode(LowPowerModes_td mode) {
    nop();
}

/**
  * @brief  Code have to be run after exit Low Power mode
  * @note   Public & weak
  * @param  mode: Low power mode
  * @retval None
  */
__weak void RunAfterExitLowPowerMode(LowPowerModes_td mode) {
    nop();
}


//EOF