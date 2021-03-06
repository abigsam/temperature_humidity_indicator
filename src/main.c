/**
  ******************************************************************************
  * @file    main.c
  * @author  abigsam
  * @version v1.2
  * @date    25.09.2018
  * @brief   Main program body
  ******************************************************************************
  * History:
  * 25.09.18 v01 -- Add low power I2C read/write (WFI)
  * 27.10.18 v02 -- Fix LowBat sign problem;
  *                 add define "BATTERY_mV" where low voltage specified in mV;
  *                 increase "LIGHTSENSOR_LEVEL" from 820 to 880
  *
  * 27.10.18 v03 -- Fix sleep functiones: skip time less than MAX values;
  *                 updated LSI frequency (measured with stopwatch)
  * 27.10.18 v04 -- Fix LCD frame rate: set to ~30 Hz;
  *                 consumption in sleep (day) decreased to ~7 uA
  * 27.10.18 v05 -- Measure time and show data time now can be configured
  *                 separately
  *
  ******************************************************************************
  * TODO:
  * -- Enable/disable SHT21 by pin
  * -- Disable clock for I2C module when comunication no needed
  * -- Decrease number measure per time (measure ones in 3..5 minutes; between
  *    this time show old data
  * -- Don't convert T/RH values to string; use two screens and chenge them
  *    when need show old data
  * 
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "stm8l15x.h"
#include "delay.h"
#include "trh_indicator_bsp.h"
#include "trh_lcd.h"
#include "utils.h"

/** @addtogroup STM8L15x_StdPeriph_Template
  * @{
  */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define FIRMWARE_VERSION        ("f05") //Three chars only
#define TEST_LSI                (0)     //If need output LSI clock to PC4 set it to '1'
//Time defines
#define DISPLAY_TIME_S          (3UL)   //Update period for LCD in seconds
#define REFRESH_DATA_S          (60UL)  //Refresh data (T and RH) in seconds
#define LIGHT_CHECK_MIN         (5UL)   //Light checking period in minutes
#define BAT_CHECK_HOURS         (24UL)  //Checking battery period, hours
#define LIGHTSENSOR_LEVEL       (880u)  //Bigger value darker ambient light
#define BATTERY_mV              (2200UL) //Lowbat voltage, mV; can be lower than 1224 mV

/* Private macro -------------------------------------------------------------*/
#define DELAY_MS(us)            { delay_lowp_ms(us); }
#define LIGHT_CHECK_PERIOD      ((uint32_t) (LIGHT_CHECK_MIN*60UL / DISPLAY_TIME_S) )
#define BAT_CHECK_PERIOD        ((uint32_t) (BAT_CHECK_HOURS*60UL*60UL / DISPLAY_TIME_S) )
#define REFRESH_DATA_PERIOD     ((uint32_t) (REFRESH_DATA_S / DISPLAY_TIME_S) )
#define LOWBAT_RAW              ((uint16_t) ((1224UL * 1024UL) / BATTERY_mV) ) //ADC_value = Vref[1.224 V] * (2^10)/ Vbat_min

/* Private variables ---------------------------------------------------------*/
typedef enum {
  PowerUp = 0,
  CheckLight,
  CheckShowConfig,
  CheckBattery,
  MeasureRH,
  MeasureT,
  SleepDay,
  SleepNight
} fsm_enum;

const uint8_t FIRMWARE[3] = FIRMWARE_VERSION;

/* Private function prototypes -----------------------------------------------*/
void error_handler(uint8_t *str, uint8_t err_num);
void measure_show(void);
bool check_light(void);
void check_battery(void);
void heater_contr(void);

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Main program.
  * @param  None
  * @retval None
  */
void main(void)
{
    uint8_t error = 0u;
    DisplConfig showConfig;
    uint32_t batCheckCnt = 0u;
    uint32_t lightChkCnt = 0u;
    uint32_t refreshDataCnt = 0u;
    fsm_enum fsm_state = PowerUp;
    bool lightCheck = TRUE, lightPrevious = TRUE;
    bool dataShowEnd = FALSE;
    ShowValueType isNeedRefresh;
    
    
#if (TEST_LSI == 1)
    GPIO_Init(GPIOC, GPIO_Pin_4, GPIO_Mode_Out_PP_Low_Fast);
    CLK_LSICmd(ENABLE);
    while (SET != CLK_GetFlagStatus(CLK_FLAG_LSIRDY)) {}
    CLK_CCOConfig(CLK_CCOSource_LSI, CLK_CCODiv_1);
    for(;;) { delay_ms(400u); }
#endif
    
    /**************************************
    * Configure Clock
    **************************************/
    CLK_SYSCLKDivConfig(CLK_SYSCLKDiv_16); //Set system clock to 1 MHz (HSI by defult, HSI = 16 MHz) --> change "SYSCLK_DIV" in delay.h
    
    /**************************************
    * Init LCD
    **************************************/
    TRH_LCD_init(TRUE);
    sleep_s(1u);
    TRH_LCD_ShowAll();
    sleep_s(1u);
    TRH_LCD_clear();
    sleep_ms(500u);
    TRH_LCD_DisplayString((uint8_t *)FIRMWARE, FALSE, 0);
    sleep_s(1u);
    TRH_LCD_clear();
    //sleep_ms(500u);
  
    /**************************************
    * Init BSP
    **************************************/
    if (error = BSP_init()) { error_handler(" BSP init err ", error); }
    DELAY_MS(100);
    
    /**************************************
    * Configure start state for FSM; run light & lowbat cheking
    **************************************/
    fsm_state = PowerUp;
    
    /**************************************
    * Main FSM
    **************************************/
    for(;;)
    {
      switch(fsm_state)
      {
      
      /* Check light sensor */
      case (CheckLight):
        fsm_state = CheckBattery;
        if (lightChkCnt >= LIGHT_CHECK_PERIOD) {
          lightChkCnt = 0u;
          lightCheck = BSP_checkAmbientLight((uint16_t)LIGHTSENSOR_LEVEL);
          if (lightCheck != lightPrevious) { //If state different, Enable or Disable LCD
            TRH_LCD_Control(lightCheck);
            lightPrevious = lightCheck;
          }
          if (!lightCheck) {
            fsm_state = SleepNight;
          }
        }
      break;
      
      /* Check battery status */
      case (CheckBattery):
        if (batCheckCnt >= BAT_CHECK_PERIOD) {
          TRH_LCD_DisplayLowBat( BSP_testBattery(LOWBAT_RAW) );
          batCheckCnt = 0u;
        }
        fsm_state = CheckShowConfig;
      break;
      
      /* Check config: show only T, only RH or both */
      case (CheckShowConfig):
        showConfig = BSP_getShowMode();
        dataShowEnd = FALSE;
        fsm_state = MeasureT;
      break;
      
      /* Show temperature and refresh it when time */
      case (MeasureT):
        if (refreshDataCnt >= REFRESH_DATA_PERIOD) { isNeedRefresh = REFRESH_VALUE; }
        else { isNeedRefresh = USE_OLD_VALUE; }
        if (showConfig != SHOW_ONLY_RH) {
          if (error = BSP_showT(NO_DECIMAL, isNeedRefresh)) { error_handler(" T read err ", error); }
        }
        fsm_state = SleepDay;
      break;
      
      /* Show humidity value and refresh it when time */
      case (MeasureRH):
        if (showConfig != SHOW_ONLY_T) {
          if (error = BSP_showRH(NO_DECIMAL, isNeedRefresh)) { error_handler(" RH read err ", error); }
        }
        dataShowEnd = TRUE;
        fsm_state = SleepDay;
      break;
      
      /* Update conters & sleep */
      case (SleepDay):
        if (dataShowEnd) {
          fsm_state = CheckLight;
          if (refreshDataCnt > REFRESH_DATA_PERIOD) { refreshDataCnt = 0u; }
        }
        else {
          fsm_state = MeasureRH;
        }
        lightChkCnt++;
        batCheckCnt++;
        refreshDataCnt++;
        sleep_s((uint32_t)DISPLAY_TIME_S);
      break;
      
      /* Sleep for the longer time when is dark */
      case (SleepNight):
        sleep_s((uint32_t)DISPLAY_TIME_S);
        lightChkCnt = LIGHT_CHECK_PERIOD; //Check light after SleepNight
        batCheckCnt = BAT_CHECK_PERIOD;   //Check battery after SleepNight
        fsm_state = CheckLight;
      break;
        
      case (PowerUp):
      default:
        showConfig = SHOW_T_RH;
        dataShowEnd = FALSE;
        lightChkCnt = 0u;
        batCheckCnt = 0u;
        refreshDataCnt =  REFRESH_DATA_PERIOD;
        TRH_LCD_DisplayLowBat( BSP_testBattery(LOWBAT_RAW) );
        fsm_state = MeasureT;
      break;
      }
    }
}

#ifdef  USE_FULL_ASSERT

/**
  * @brief  Reports the name of the source file and the source line number
  *   where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{ 
    /* User can add his own implementation to report the file name and line number,
    ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
    
    /* Infinite loop */
    while (1)
    {
    }
}
#endif

/**************************************
 * Error handler
**************************************/
void error_handler(uint8_t *str, uint8_t err_num) {
    uint8_t errMsg[4];
    //
    int2str((uint16_t)err_num, errMsg, 3u);
    str_addBeforeChar(errMsg, 'E');
    TRH_LCD_DisplayString(errMsg, FALSE, 0u);
    for(;;);
}

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
