/**
  ******************************************************************************
  * @file    trh_lcd.h
  * @author  abigsam
  * @version v1.1
  * @date    25.09.2018
  * @brief   LCD driver header
  ******************************************************************************
  * History:
  * 27.10.18 v1.1 -- Fix LCD frame rate defines
  * 
  ******************************************************************************
  * TODO:
  * -- Add functiones to change contrast
  * 
  ******************************************************************************
  */

#ifndef __TRH_LCD_H
#define __TRH_LCD_H

#ifdef __cplusplus
 extern "C" {
#endif


#include "stm8l15x.h"

#define LCD_CHAR_HUMIDITY   ('H') //Char for humidity
#define LCD_CHAR_DEGREES    ('d') //Char for degrees sign
#define LCD_MAX_POSITION    (3U)  //Max. number of positiones in this LCD

/* Frame = (LSI/2) / ( LCD_PRESCALLER * LCD_DIVIDER ) */
#define LCD_PRESCALLER      (LCD_Prescaler_32) //For ~30Hz frame rate (RM, p. 262)
#define LCD_DIVIDER         (LCD_Divider_16)   //For ~30Hz frame rate (RM, p. 262)
#define LCD_VOLTAGE_SOURCE  (LCD_VoltageSource_Internal)
#define LCD_CONTRAST        (LCD_Contrast_Level_3) //Default contrast (LCD_Contrast_Level_3: Medium Density / High Density Maximum Voltage = 2.90V / 2.99V)

/* Type defines */
typedef enum {
    LCD_SIGN_NONE = 0,
    LCD_SIGN_PLUS,
    LCD_SIGN_MINUS
} LCD_SignTypeDef;


/*--------------------------------------------------------
--- Private functions
--------------------------------------------------------*/
static void put_char(char *ch, uint8_t pos);
static void wait_for_update(void);
static uint8_t convert_char(char *ch);
static void show_dpoint(uint8_t pos);
static void clear_all(void);
static void control_lowbat(bool newState);


/*--------------------------------------------------------
--- Public functions
--------------------------------------------------------*/
void TRH_LCD_init(bool initRTCclock);
void TRH_LCD_clear(void);
void TRH_LCD_ShowAll(void);
void TRH_LCD_DisplayString(uint8_t *ptr, bool show_dp, uint8_t point_pos);
void TRH_LCD_DisplayLowBat(bool low_bat);
void TRH_LCD_ShowSign(LCD_SignTypeDef sign);
void TRH_LCD_Control(bool state);

void TRH_LCD_update(void);  //Inser this in LCD interrupt

//void TRH_LCD_debug(void);

#ifdef __cplusplus
}
#endif

#endif //__TRH_LCD_H
