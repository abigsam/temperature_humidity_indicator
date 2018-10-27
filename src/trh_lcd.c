




#include "trh_lcd.h"
#include "utils.h"


/* Defines */
#define LCD_RAM_BYTES           ((uint8_t) 4)
#define SEG_MASK_A              ((uint8_t) 1 << 0)
#define SEG_MASK_B              ((uint8_t) 1 << 1)
#define SEG_MASK_C              ((uint8_t) 1 << 2)
#define SEG_MASK_D              ((uint8_t) 1 << 3)
#define SEG_MASK_E              ((uint8_t) 1 << 4)
#define SEG_MASK_F              ((uint8_t) 1 << 5)
#define SEG_MASK_G              ((uint8_t) 1 << 6)

#define LETTER_H                ((uint8_t) 0x76)
#define LETTER_h                ((uint8_t) 0x74)
#define LETTER_C                ((uint8_t) 0x76)
#define DEGREES_SIGN            ((uint8_t) 0x63)
#define SPACE_SIGN              (0U)
#define LETTER_E                ((uint8_t) 0x79)
#define LETTER_F                ((uint8_t) 0x71)

#ifndef NULL
    #define NULL            (0x00)
#endif

/* Macroses */
#define LCD_LOWBAT_SET()        { lcdRam[3] |= (uint8_t) (1 << 2); }
#define LCD_LOWBAT_RESET()      { lcdRam[3] &= ~((uint8_t)(1 << 2)); }
#define LCD_SIGN_HBAR_SET()     { lcdRam[0] |= (uint8_t) 1; }
#define LCD_SIGN_HBAR_RESET()   { lcdRam[0] &= ~((uint8_t) 1); }
#define LCD_SIGN_VBAR_SET()     { lcdRam[3] |= (uint8_t) (1 << 3); }
#define LCD_SIGN_VBAR_RESET()   { lcdRam[3] &= ~((uint8_t) (1 << 3)); }

#define LCD_INTR_CLR()          { LCD->CR3 |= (uint8_t)(LCD_CR3_SOFC); }


/* Constants */
const uint8_t NUMBER_MAP[10] = {
    //0
    ( SEG_MASK_A | SEG_MASK_B | SEG_MASK_C | SEG_MASK_D | SEG_MASK_E | SEG_MASK_F ),
    //1
    ( SEG_MASK_B | SEG_MASK_C ),
    //2
    ( SEG_MASK_A | SEG_MASK_B | SEG_MASK_D | SEG_MASK_E | SEG_MASK_G ),
    //3
    ( SEG_MASK_A | SEG_MASK_B | SEG_MASK_C | SEG_MASK_D | SEG_MASK_G ),
    //4
    ( SEG_MASK_B | SEG_MASK_C | SEG_MASK_F | SEG_MASK_G ),
    //5
    ( SEG_MASK_A | SEG_MASK_C | SEG_MASK_D | SEG_MASK_F | SEG_MASK_G ),
    //6
    ( SEG_MASK_A | SEG_MASK_C | SEG_MASK_D | SEG_MASK_E | SEG_MASK_F | SEG_MASK_G ),
    //7
    ( SEG_MASK_A | SEG_MASK_B | SEG_MASK_C ),
    //8
    ( SEG_MASK_A | SEG_MASK_B | SEG_MASK_C | SEG_MASK_D | SEG_MASK_E | SEG_MASK_F | SEG_MASK_G ),
    //9
    ( SEG_MASK_A | SEG_MASK_B | SEG_MASK_C | SEG_MASK_D | SEG_MASK_F | SEG_MASK_G )
};


/* Private variables */
uint8_t lcdRam[LCD_RAM_BYTES] = {0};
bool lowbatFlag = FALSE;


/*--------------------------------------------------------
--- Private functions
--------------------------------------------------------*/

/**
  * @brief  Put char on specified position on LCD
  * @note   Private
  * @param  ch: pointer to char
  * @param  pos: char position on LCD
  * @retval None
  */
static void put_char(char *ch, uint8_t pos) {
    uint8_t chCoded = convert_char(ch);
    switch(pos) {
        case (0u):
            /* SEG11...SEG17 */
            lcdRam[1] &= ~((uint8_t) 0xf8);
            lcdRam[2] &= ~((uint8_t) 0x03);
            //
            lcdRam[1] |= (chCoded & SEG_MASK_A) << 7;
            lcdRam[1] |= (chCoded & SEG_MASK_B) << 5;
            lcdRam[1] |= (chCoded & SEG_MASK_C) << 3;
            lcdRam[1] |= (chCoded & SEG_MASK_D) << 1;
            lcdRam[1] |= (chCoded & SEG_MASK_E) >> 1;
            lcdRam[2] |= (chCoded & SEG_MASK_F) >> 5;
            lcdRam[2] |= (chCoded & SEG_MASK_G) >> 5;
        break;
        
        case (1u):
            /* SEG7...SEG9, SEG18...SEG21 */
            lcdRam[0] &= ~((uint8_t) 0x80);
            lcdRam[1] &= ~((uint8_t) 0x03);
            lcdRam[2] &= ~((uint8_t) 0x3c);
            //
            lcdRam[2] |= (chCoded & SEG_MASK_A) << 3;
            lcdRam[2] |= (chCoded & SEG_MASK_B) << 1;
            lcdRam[1] |= (chCoded & SEG_MASK_C) >> 1;
            lcdRam[1] |= (chCoded & SEG_MASK_D) >> 3;
            lcdRam[0] |= (chCoded & SEG_MASK_E) << 3;
            lcdRam[2] |= (chCoded & SEG_MASK_F) >> 1;
            lcdRam[2] |= (chCoded & SEG_MASK_G) >> 1;
        break;
        
        case (2u):
            /* SEG3...SEG5, SEG22...SEG25 */
            lcdRam[0] &= ~((uint8_t) 0x38); // 3...5
            lcdRam[2] &= ~((uint8_t) 0xc0); //22...23
            lcdRam[3] &= ~((uint8_t) 0x03); //24...25
            //
            lcdRam[2] |= (chCoded & SEG_MASK_A) << 7;
            lcdRam[2] |= (chCoded & SEG_MASK_B) << 5;
            lcdRam[0] |= (chCoded & SEG_MASK_C) << 3;
            lcdRam[0] |= (chCoded & SEG_MASK_D) << 1;
            lcdRam[0] |= (chCoded & SEG_MASK_E) >> 1;
            lcdRam[3] |= (chCoded & SEG_MASK_F) >> 5;
            lcdRam[3] |= (chCoded & SEG_MASK_G) >> 5;
        break;
        
        default: break;
    }
}


/**
  * @brief  Convert char to 7-segment coding (0b0HGFDCBA)
  * @note   Private
  * @param  None
  * @retval None
  */
static uint8_t convert_char(char *ch) {
    uint8_t outv = SPACE_SIGN;
    switch(*ch) {
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            outv = NUMBER_MAP[(*ch - '0')];
        break;
        
        case 'h':
            outv = LETTER_h;
        break;
        
        //case 'H':
        case LCD_CHAR_HUMIDITY:
            outv = LETTER_H;
        break;
        
        case 'C':
            outv = LETTER_C;
        break;
        
        //case 'd':
        case LCD_CHAR_DEGREES:
            outv = DEGREES_SIGN;
        break;
        
        case 'E':
            outv = LETTER_E;
        break;
        
        case 'F':
        case 'f':
            outv = LETTER_F;
        break;
        
        default:
            outv = SPACE_SIGN;
        break;
    }
    return outv;
}


/**
  * @brief  Wait until LCD will be updated
  * @note   Switch CPU in WFI mode
  * @note   Private
  * @param  None
  * @retval None
  */
static void wait_for_update(void) {
    LCD_INTR_CLR();
    LCD_ITConfig(ENABLE);
    enableInterrupts();
    wfi();
    LCD_ITConfig(DISABLE);
    disableInterrupts();
}

/**
  * @brief  Show/hide decimal point
  * @note   Control one decimal point at once; all other are hided
  * @note   Private
  * @param  pos: decimap point position; 0...2
  * @retval None
  */
static void show_dpoint(uint8_t pos) {
    switch(pos) {
        case 0u: /* SEG10 */
            lcdRam[1] |= 0x4u;
        break;
        
        case 1u: /* SEG6 */
            lcdRam[0] |= 0x40u;
        break;
        
        case 2u: /* SEG2 */
            lcdRam[0] |= 0x4u;
        break;
        
        default: break;
    }
}


static void clear_all(void) {
    uint8_t cnt = 0u;
    for (; cnt < LCD_RAM_BYTES; ++cnt) {
        lcdRam[cnt] = LCD_RAM_RESET_VALUE;
    }
}

/**
  * @brief  Control "Low Bat" signa on LCD
  * @note   Private
  * @param  newState: new state of "low Bat" sign
  * @retval None
  */
static void control_lowbat(bool newState) {
  lowbatFlag = newState;
  if (lowbatFlag) { LCD_LOWBAT_SET(); }
  else            { LCD_LOWBAT_RESET(); }
}


/*--------------------------------------------------------
--- Public functions
--------------------------------------------------------*/

/**
  * @brief  Init LCD controller
  * @note   Public
  * @param  initRTCclock: false if RTC already inited
  * @retval None
  */
void TRH_LCD_init(bool initRTCclock) {
    uint8_t cnt = 0u;
    
    /* Enable LCD/RTC clock */
    CLK_PeripheralClockConfig(CLK_Peripheral_LCD, ENABLE);

    if (initRTCclock) {
        CLK_PeripheralClockConfig(CLK_Peripheral_RTC, ENABLE);
//#if (USE_LSE > 0)
//        CLK_RTCClockConfig(CLK_RTCCLKSource_LSE, CLK_RTCCLKDiv_1);
//#else
        CLK_RTCClockConfig(CLK_RTCCLKSource_LSI, CLK_RTCCLKDiv_1);
//#endif
    
    }
  
    /* Initialize the LCD */
    LCD_Init(LCD_PRESCALLER, LCD_DIVIDER,
             LCD_Duty_Static, LCD_Bias_1_2,
             LCD_VOLTAGE_SOURCE);
  
    /* Mask register
    For declare the segements used: from 0 to 27 */
    LCD_PortMaskConfig(LCD_PortMaskRegister_0, 0xff); // 0...7
    LCD_PortMaskConfig(LCD_PortMaskRegister_1, 0xff); // 8...15
    LCD_PortMaskConfig(LCD_PortMaskRegister_2, 0xff); //16...23
    LCD_PortMaskConfig(LCD_PortMaskRegister_3, 0x0f); //24...27
  
    /* To set contrast to mean value */
    LCD_ContrastConfig(LCD_Contrast_3V0);
    LCD_DeadTimeConfig(LCD_DeadTime_0);
    LCD_PulseOnDurationConfig(LCD_PulseOnDuration_1);
    
    /* Clear LCD RAM */
    for (cnt = 0u; cnt <= LCD_RAMRegister_13; ++cnt) {
        LCD->RAM[cnt] = LCD_RAM_RESET_VALUE;
    }
    for (cnt = 0u; cnt < LCD_RAM_BYTES; ++cnt) {
        lcdRam[cnt] = LCD_RAM_RESET_VALUE;
    }
    /* Clear lowbatFlag */
    lowbatFlag = 0u;
    
    /* Enable LCD peripheral */ 
    LCD_Cmd(ENABLE);
}


/**
  * @brief  Clear all segments
  * @note   Public
  * @param  None
  * @retval None
  */
void TRH_LCD_clear(void) {
    clear_all();
    lowbatFlag = 0u;
    wait_for_update();
}


/**
  * @brief  Enable all segments
  * @note   Public
  * @param  None
  * @retval None
  */
void TRH_LCD_ShowAll(void) {
    uint8_t cnt;
    for (cnt = 0u; cnt < LCD_RAM_BYTES; ++cnt) {
        lcdRam[cnt] = 0xff;
    }
    wait_for_update();
}


void TRH_LCD_DisplayString(uint8_t *ptr, bool show_dp, uint8_t point_pos) {
    uint8_t pcnt = 0u;
    uint8_t *p = NULL;
    clear_all();
    
    /* Detect if string has sign '+' or '-' */
    if ((*ptr) == '-') { ++ptr; TRH_LCD_ShowSign(LCD_SIGN_MINUS); }
    else if ((*ptr) == '+') { ++ptr; TRH_LCD_ShowSign(LCD_SIGN_PLUS); }
    
    /* Get last char before NULL */
    p = str_getLast(ptr, LCD_MAX_POSITION+1);
    for (pcnt = 0u; (pcnt < LCD_MAX_POSITION) && (p >= ptr); ++pcnt, --p) {
        put_char((char*)p, pcnt);
    }
    
    if (show_dp) { show_dpoint(point_pos); }
    
    /* Restore lowbat */
    control_lowbat(lowbatFlag);
    
    wait_for_update();
}


/**
  * @brief  Show or hide "Battery Low" sign
  * @note   Public
  * @param  low_bat: show "LOBAT" sign if TRUE
  * @retval None
  */
void TRH_LCD_DisplayLowBat(bool low_bat) {
    control_lowbat(low_bat);
    wait_for_update();
}


/**
  * @brief  Show or hide left sigh on LCD
  * @note   Public
  * @param  sign: type of sign; can be:
  *         LCD_SIGN_NONE -- without any sign;
  *         LCD_SIGN_PLUS -- show '+' sign;
  *         LCD_SIGN_MINUS -- show '-' sign.
  * @retval None
  */
void TRH_LCD_ShowSign(LCD_SignTypeDef sign) {
    switch(sign) {
        case LCD_SIGN_PLUS:
            LCD_SIGN_HBAR_SET();
            LCD_SIGN_VBAR_SET();
        break;
        
        case LCD_SIGN_MINUS:
            LCD_SIGN_HBAR_SET();
            LCD_SIGN_VBAR_RESET();
        break;
        
        case LCD_SIGN_NONE:
        default:
            LCD_SIGN_HBAR_RESET();
            LCD_SIGN_VBAR_RESET();
        break;
    }
    wait_for_update();
}


/**
  * @brief  Update LCD RAM in interrupt with actual values
  * @note   Public
  * @param  None
  * @retval None
  */
void TRH_LCD_update(void) {
    uint8_t cnt;
    for (cnt = 0u; cnt < LCD_RAM_BYTES; ++cnt) {
        LCD->RAM[cnt] = lcdRam[cnt];
    }
    //Clear flag
    LCD_INTR_CLR();
}


//For debug
void TRH_LCD_debug(void) {
    char temp[3] = {'0', '1', '2'};
    //
    put_char(temp,   0);
    put_char(temp+1, 1);
    put_char(temp+2, 2);
    wait_for_update();
}

/**
  * @brief  Control LCD
  * @note   Public
  * @param  state -- if TRUE LCD enabled; FALSE LCD disabled
  * @retval None
  */
void TRH_LCD_Control(bool state) {//, bool initRTCclock) {
  if (state) {
    TRH_LCD_init(TRUE);
  }
  else {
    LCD_DeInit();
    LCD_Cmd(DISABLE);
    CLK_PeripheralClockConfig(CLK_Peripheral_LCD, DISABLE);
  }
}

//EOF