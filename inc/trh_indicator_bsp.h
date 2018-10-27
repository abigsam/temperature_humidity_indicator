/**
  ******************************************************************************
  * @file    trh_indicator_bsp.h
  * @author  abigsam
  * @version v1.0
  * @date    25.09.2018
  * @brief   BSP for TRH indicator header
  ******************************************************************************
  */

#ifndef __TRH_INDICATOR_BSP_H
#define __TRH_INDICATOR_BSP_H

#ifdef __cplusplus
 extern "C" {
#endif

#include "stm8l15x.h"
#include "utils.h"
#include "delay.h"
#include "sht21.h"
#include "stm8l_discovery_lcd.h"
#include "stm8l_low_power.h"
#include "trh_lcd.h"

/* GPIO defines */
#define TRH_UART_TX_PIN         (GPIO_Pin_2)
#define TRH_UART_TX_PORT        (GPIOA)
#define TRH_UART_RX_PIN         (GPIO_Pin_3)
#define TRH_UART_RX_PORT        (GPIOA)
#define TRH_LIGHTSENS_PIN       (GPIO_Pin_1)
#define TRH_LIGHTSENS_PORT      (GPIOD)
#define TRH_LIGHTSENS_ADC_CH    (ADC_Channel_21)
#define TRH_LIGHTSENS_EN_PIN    (GPIO_Pin_0)
#define TRH_LIGHTSENS_EN_PORT   (GPIOF)
#define TRH_LIGHTSENS_HYSTR     ((uint16_t) 30)         //Hysteresis value for light sensor
#define TRH_SENSOR_PWR_PIN      (GPIO_Pin_5)
#define TRH_SENSOR_PWR_PORT     (GPIOA)
//
#define TRH_MEASURE_MEAN        (2) //Number of measurements for meaning
//
#define TRH_DELAY_US(us)        { delay_lowp_ms(us); }
#define TRH_SLEEP_MS(ms)        { sleep_ms(ms); }

/* Types */
typedef enum {
    NO_DECIMAL = 0u,    //Rounded to integer part
    ONE_DIGIT_DECIMAL   //One digit after decimal point
} DecimalPointPos;

typedef enum {
    SHOW_T_RH = 0u, //Show temperature and humidity alternately
    SHOW_ONLY_T,    //Show only temperature
    SHOW_ONLY_RH    //Show only humidity
} DisplConfig;

typedef enum {
    MEASURE_TEMPERATURE = 0u,
    MEASURE_HUMIDITY
} MeasureType;

typedef enum {
  USE_OLD_VALUE = 0,
  REFRESH_VALUE
} ShowValueType;

typedef enum {
  SHT21_off = 0,
  SHT21_on,
  SHT21_initialize,
  SHT21_measure_T,
  SHT21_measure_RH
} SHT21workflowType;

/*--------------------------------------------------------
--- Private functions
--------------------------------------------------------*/
static void clr_buff(void);
static void adc_config(void);
static void adc_meas_lowp(uint16_t mdelay);
static uint16_t get_batt_voltage(void);
static SHT21State_td sht21_workflow(SHT21workflowType t);


/*--------------------------------------------------------
--- Public functions
--------------------------------------------------------*/
/* High level */
uint8_t BSP_init(void);
uint8_t BSP_measure(MeasureType t);
uint8_t BSP_showT(DecimalPointPos p, ShowValueType refresh);
uint8_t BSP_showRH(DecimalPointPos p, ShowValueType refresh);
void BSP_showErr(uint8_t err_num);

bool BSP_testBattery(uint16_t batLevel);
uint8_t BSP_sht21_heater(bool state);
DisplConfig BSP_getShowMode(void);
bool BSP_checkAmbientLight(uint16_t lightLevel);

#ifdef __cplusplus
}
#endif

#endif  //__TRH_INDICATOR_BSP_H