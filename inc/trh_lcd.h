





#ifndef __TRH_LCD_H
#define __TRH_LCD_H

#ifdef __cplusplus
 extern "C" {
#endif


#include "stm8l15x.h"

#define LCD_CHAR_HUMIDITY   ('H')
#define LCD_CHAR_DEGREES    ('d')

#define LCD_USE_LSE         (0)

#define LCD_PRESCALLER      (LCD_Prescaler_1)
#define LCD_DIVIDER         (LCD_Divider_31)
#define LCD_VOLTAGE_SOURCE  (LCD_VoltageSource_Internal)
#define LCD_CONTRAST        (LCD_Contrast_3V0)

#define LCD_MAX_POSITION    (3U)


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

void TRH_LCD_update(void);  //Inser this in LCD interrupt

void TRH_LCD_debug(void);

void TRH_LCD_Control(bool state);//, bool initRTCclock);

#ifdef __cplusplus
}
#endif

#endif //__TRH_LCD_H