/*

    For "LP_RUN_MODE" and "LP_WAIT_MODE" need add MY_RAM_FUNC to
    linker configuration file (icf). For details see IAR Technical Note 27498
    URL: https://www.iar.com/search-results/#!?query=27498&tab=pages
    
*/

#ifndef __STM8L_LOW_POWER_H
#define __STM8L_LOW_POWER_H

#ifdef __cplusplus
 extern "C" {
#endif

#include "stm8l15x.h"

//--- Defines
#define LP_LP_USE_LOW_POWER_RUN     (0)
#define LP_USE_LSE                  (0)
#define LP_USE_HSE                  (0)

//--- Macroses

//--- Types
typedef enum {
    MCU_RUN = 0u,
    WAIT_INTERRUPT,
    WAIT_EVENT,
#if (LP_USE_LOW_POWER_RUN == 1)
    LP_RUN_MODE,    //Low power run mode
    LP_WAIT_MODE,   //Low power wait mode
#endif
    ACTIVE_HALT,
    HALT
} LowPowerModes_td;

//typedef enum {
//    LP_RTC_USE_LSI = 0,     //Use internal low-speed clock as RTC clock source (the same used for LCD)
//    LP_RTC_USE_LSE          //Use external low-speed clock as RTC clock source (the same used for LCD)
//} LowPowerRTCclock_td

/*--------------------------------------------------------
--- Private functions
--------------------------------------------------------*/
#if (LP_USE_LOW_POWER_RUN == 1)
static void LowPowerRunInit(mode);
#endif

/*--------------------------------------------------------
--- Public functions
--------------------------------------------------------*/
void LowPowerEnable(LowPowerModes_td mode);
uint8_t LowPowerExit(void);//LowPowerModes_td mode);
//void LowPower_RTCinit(LowPowerRTCclock_td clockType);
//void LowPower_RTC

/*--------------------------------------------------------
--- Public weak functions
--------------------------------------------------------*/
__weak void GPIOlowPowerConfig(void);
__weak void GPIOwaitConfig(void);
__weak void GPIOlowPowerExitConfig(void);
__weak void RunBeforeLowPowerMode(LowPowerModes_td mode);
__weak void RunAfterExitLowPowerMode(LowPowerModes_td mode);

#ifdef __cplusplus
}
#endif

#endif  //__STM8L_LOW_POWER_H