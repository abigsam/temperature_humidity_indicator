/*


*/

#include "trh_indicator_bsp.h"

#define STR_BUFF_MAX            ((uint16_t) 10)
#ifndef NULL
    #define NULL                (0x00)
#endif

#define HUMIDITY_SIGN           ('H')
#define TEMPERATURE_SIGN        ('T')


/* Macroses */
#define IS_NEED_ROUNDED(d)      (uint_dg%10u > 4u)
#define IS_SHOW_DECIMAL(p)      (NO_DECIMAL == p ? FALSE : TRUE)
#define GET_ROUNDED(r)          ( (r%10 > 4) ? (r/10 + 1) : (r / 10) )
#define IS_TX_PIN_SET()         ((uint8_t)TRH_UART_TX_PIN & GPIO_ReadInputData(TRH_UART_TX_PORT))
#define IS_RX_PIN_SET()         ((uint8_t)TRH_UART_RX_PIN & GPIO_ReadInputData(TRH_UART_RX_PORT))
#define LIGHT_SENS_EN()         (GPIO_SetBits(TRH_LIGHTSENS_EN_PORT, TRH_LIGHTSENS_EN_PIN))
#define LIGHT_SENS_DIS()        (GPIO_ResetBits(TRH_LIGHTSENS_EN_PORT, TRH_LIGHTSENS_EN_PIN))
#define SENSOR_ON()             (GPIO_SetBits(TRH_SENSOR_PWR_PORT, TRH_SENSOR_PWR_PIN))
#define SENSOR_OFF()            (GPIO_ResetBits(TRH_SENSOR_PWR_PORT, TRH_SENSOR_PWR_PIN))
//



/* Private variables */
uint8_t str_buff_t[STR_BUFF_MAX] = {0u};
uint8_t str_buff_rh[STR_BUFF_MAX] = {0u};
int16_t current_tempr = 0u;
int16_t current_rh = 0u;
bool lights_prev = TRUE;


/*--------------------------------------------------------
--- Private functions
--------------------------------------------------------*/

/**
  * @brief  Clear internal buffer
  * @note   Private
  * param   None
  * @retval None
  */
static void clr_buff(void) {
    uint16_t i;
    for (i = 0u; i < STR_BUFF_MAX; ++i) {
        str_buff_t[i] = (uint8_t)NULL;
        str_buff_rh[i] = (uint8_t)NULL;
    }
}

/**
  * @brief  Configure ADC
  * @note   Private
  * @param  None
  * @retval None
  */
void adc_config(void) {
    CLK_PeripheralClockConfig(CLK_Peripheral_ADC1, ENABLE);
    ADC_Init(ADC1, ADC_ConversionMode_Single, ADC_Resolution_10Bit, ADC_Prescaler_2);
    CLK_PeripheralClockConfig(CLK_Peripheral_ADC1, DISABLE);
}

/**
  * @brief  
  * @note   Private
  * @param  
  * @retval 
  */
uint16_t get_batt_voltage(void) {
    uint16_t res = 0u;
    //
    CLK_PeripheralClockConfig(CLK_Peripheral_ADC1, ENABLE);
    ADC_Cmd(ADC1, ENABLE);
    ADC_VrefintCmd(ENABLE);
    TRH_DELAY_US(3u);
    ADC_ChannelCmd(ADC1, ADC_Channel_Vrefint, ENABLE);
    
    adc_meas_lowp(0);
    
    res = ADC_GetConversionValue(ADC1);
    ADC_ChannelCmd(ADC1, ADC_Channel_Vrefint, DISABLE);
    ADC_VrefintCmd(DISABLE);
    ADC_Cmd(ADC1, DISABLE);
    CLK_PeripheralClockConfig(CLK_Peripheral_ADC1, DISABLE);
    return res;
}

/**
  * @brief  Start ADC convert with prestart delay and wait
  *         end convertation in low power mode
  * @note   Private
  * @param  mdelay: ADC prestart delay in ms
  * @retval None
  */
static void adc_meas_lowp(uint16_t mdelay) {
    if (mdelay) { TRH_DELAY_US(mdelay); }
    ADC_ITConfig(ADC1, ADC_IT_EOC, ENABLE);
    enableInterrupts();
    ADC_SoftwareStartConv(ADC1);
    wfi();
    disableInterrupts();
    ADC_ITConfig(ADC1, ADC_IT_EOC, DISABLE);
}

/**

 */
static SHT21State_td sht21_workflow(SHT21workflowType t)
{
  SHT21State_td st;
  int16_t temp_val = 0u;
  int32_t temp_m = 0u;
  
  if (SHT21_off == t) {
    SENSOR_OFF();
    return SHT21_OK;
  }
  //
  SENSOR_ON();
  delay_ms(500);
  if (SHT21_on == t) { return SHT21_OK; }
  if (SHT21_initialize == t) {
    st = SHT21_init();
    return st;
  }
  for (uint8_t cnt = 0; cnt < TRH_MEASURE_MEAN; cnt++) {
    if (SHT21_measure_T == t) { st = SHT21_getTint(&temp_val); } //current_tempr
    else { st = SHT21_getRHint(&temp_val); } //current_rh
    /* Check for state */
    if (st != SHT21_OK) { return st; }
    temp_m += ((int32_t)temp_val);
    TRH_SLEEP_MS(100);
  }
  //SENSOR_OFF();
  temp_m /= TRH_MEASURE_MEAN;
  if (SHT21_measure_T == t) { current_tempr = (int16_t)temp_m; }
  else { current_rh = (int16_t)temp_m; }
  return st;
}




/*--------------------------------------------------------
--- Public functions
--------------------------------------------------------*/


/*****************************************************************
 * High level
*****************************************************************/

/**
  * @brief  Init BSP
  * @note   Public
  * @param  None
  * @retval sensor HAL status (0x00 if success)
  */
uint8_t BSP_init(void) {
    SHT21State_td st;
    clr_buff();
    current_tempr = 0;
    current_rh = 0;
    lights_prev = TRUE;
    /* Init GPIO */
    GPIO_Init(TRH_UART_TX_PORT, TRH_UART_TX_PIN, GPIO_Mode_In_PU_No_IT);
    GPIO_Init(TRH_UART_RX_PORT, TRH_UART_RX_PIN, GPIO_Mode_In_PU_No_IT);
    GPIO_Init(TRH_LIGHTSENS_EN_PORT, TRH_LIGHTSENS_EN_PIN, GPIO_Mode_Out_PP_Low_Slow);
    GPIO_Init(TRH_SENSOR_PWR_PORT, TRH_SENSOR_PWR_PIN, GPIO_Mode_Out_PP_High_Slow);
    /* Init ADC */
    adc_config();
    /* Init RTC */
    initRTCwakeup();
    /* Init sensor */
    //SENSOR_ON();
    //SHT21State_td st = SHT21_init();
    //SENSOR_OFF();
    st = sht21_workflow(SHT21_initialize);
    //SENSOR_OFF();
    if (SHT21_OK != st) { return (uint8_t)st; }
    //st = SHT21_setResolution(SHT21_11H_11T);
    return (uint8_t)st;
}

/**
  * @brief  Read data from sensor and store it in RAM
  * @note   Public
  * @param  t: what type of data read from sensor
  *          This parameter can be one of the following values:
  *            @arg MEASURE_TEMPERATURE : measure temperature
  *            @arg MEASURE_HUMIDITY : measure humidity
  * @retval sensor HAL status (0x00 if success)
  */
uint8_t BSP_measure(MeasureType t) {
    SHT21State_td sht21_st = SHT21_OK;
    if (MEASURE_TEMPERATURE == t) {
        if (sht21_st = sht21_workflow(SHT21_measure_T)) { return (uint8_t)sht21_st; }
    }
    if (MEASURE_HUMIDITY == t) {
        if (sht21_st = sht21_workflow(SHT21_measure_RH)) { return (uint8_t)sht21_st; }
    }
    return 0u;
}

/**
  * @brief  Display temperature value from buffer. If need refresh data from sensor
  *         Without refreshing data need to be read with BSP_measure()
  * @note   Public
  * @param  p: display or not decimal point
  *          This parameter can be one of the following values:
  *            @arg NO_DECIMAL : display without decimal point
  *            @arg ONE_DIGIT_DECIMAL : show decimal value before last digit
  * @param  refresh: use refresh value or old
  *          This parameter can be one of the:
  *            @arg USE_OLD_VALUE: don't measure new value but use old one
  *            @arg REFRESH_VALUE: measure new value
  * @retval sensor HAL status (0x00 if success)
  */
uint8_t BSP_showT(DecimalPointPos p, ShowValueType refresh) {
    SHT21State_td sht21_st;
    uint16_t rounded_t= 0u;
    if (refresh != USE_OLD_VALUE) {
        if (sht21_st = sht21_workflow(SHT21_measure_T)) { return (uint8_t)sht21_st; }
    }
    rounded_t = current_tempr;
    if (! IS_SHOW_DECIMAL(p)) {
        rounded_t = GET_ROUNDED(current_tempr);
    }
    int2str(rounded_t, str_buff_t, STR_BUFF_MAX);
    if (str_buff_t[0] != '-') {
        str_addBeforeChar(str_buff_t, '+');
    }
    if (!IS_SHOW_DECIMAL(p)) {
        str_addChar(str_buff_t, LCD_CHAR_DEGREES);
    }
    TRH_LCD_DisplayString(str_buff_t, IS_SHOW_DECIMAL(p), 0u);
    return 0u;
}

/**
  * @brief  Display humidity value from buffer. If need refresh data from sensor
  *         Without refreshing data need to be read with BSP_measure()
  * @note   Public
  * @param  p: display or not decimal point
  *          This parameter can be one of the following values:
  *            @arg NO_DECIMAL : display without decimal point
  *            @arg ONE_DIGIT_DECIMAL : show decimal value before last digit
  * @param  refresh: use refresh value or old
  *          This parameter can be one of the:
  *            @arg USE_OLD_VALUE: don't measure new value but use old one
  *            @arg REFRESH_VALUE: measure new value
  * @retval Sensor HAL status (0x00 if success)
  */
uint8_t BSP_showRH(DecimalPointPos p, ShowValueType refresh) {
    SHT21State_td sht21_st;
    uint16_t rounded_rh = 0u;
    //
    if (refresh != USE_OLD_VALUE) {
        if (sht21_st = sht21_workflow(SHT21_measure_RH)) { return (uint8_t)sht21_st; }
    }
    rounded_rh = current_rh;
    if (! IS_SHOW_DECIMAL(p)) {
        rounded_rh = GET_ROUNDED(current_rh);
    }
    int2str(rounded_rh, str_buff_rh, STR_BUFF_MAX);
    if (!IS_SHOW_DECIMAL(p)) {
        str_addChar(str_buff_rh, LCD_CHAR_HUMIDITY);
    }
    TRH_LCD_DisplayString(str_buff_rh, IS_SHOW_DECIMAL(p), 0u);
    return 0u;
}

/**
  * @brief  Display current error number
  * @note   Public
  * @param  err_num: error number
  * @retval None
  */
void BSP_showErr(uint8_t err_num) {
    uint8_t err_char = 0u;
    const uint8_t ERR_MSG[2] = "Er";
    err_char = (err_num > 9u ? '-' : (err_num + '0'));
    LCD_GLASS_Clear();
    LCD_GLASS_DisplayString((uint8_t *)ERR_MSG);
    LCD_GLASS_WriteChar(&err_char, FALSE, FALSE, 2u);
}



/**
  * @brief  Measure battery voltage, output it status and display
  *         bad battery sign
  * @note   Public
  * @param  batLevel -- battery level
  * @retval FALSE if battery is OK
  */
bool BSP_testBattery(uint16_t batLevel) {
    //Bigger value equals to lowest battery voltage
    if (get_batt_voltage() > batLevel) { return TRUE; }
    return FALSE;
}

/**
  * @brief  On/off SHT21 internal heater
  * @note   Public
  * @param  state: TRUE if need enable heater
  * @retval Sensor HAL status (0x00 if success)
  */
uint8_t BSP_sht21_heater(bool state) {
    return (uint8_t)SHT21_setHeater(state);
}

/**
  * @brief  Read GPIO and output show mode configuration
  * @note   Public
  * @param  None
  * @retval Configuration: SHOW_T_RH, SHOW_ONLY_T, SHOW_ONLY_RH
  */
DisplConfig BSP_getShowMode(void) {
    if (!IS_TX_PIN_SET())      { return SHOW_ONLY_T; }
    else if (!IS_RX_PIN_SET()) { return SHOW_ONLY_RH; }
    else                       { return SHOW_T_RH; }
}

/**
  * @brief  Read ambient light sensor and return true if light
  *         equal or bigger then lightLevel
  * @note   Public
  * @param  lightLevel: level of light, valid value from 0 (light) to 1023 (dark)
  * @retval True if light level is equal to lightLevel
  */
bool BSP_checkAmbientLight(uint16_t lightLevel) {
    uint16_t adcVal = 0u;
    bool res = FALSE;
    /* Enable light sensor & ADC*/
    LIGHT_SENS_EN();
    CLK_PeripheralClockConfig(CLK_Peripheral_ADC1, ENABLE);
    ADC_Cmd(ADC1, ENABLE);
    ADC_ChannelCmd(ADC1, TRH_LIGHTSENS_ADC_CH, ENABLE);
    adc_meas_lowp(10);
    /* Read ADC value */
    adcVal = ADC_GetConversionValue(ADC1);
    /* Disable ADC & light sensor */
    ADC_ChannelCmd(ADC1, TRH_LIGHTSENS_ADC_CH, DISABLE);
    ADC_Cmd(ADC1, DISABLE);
    CLK_PeripheralClockConfig(CLK_Peripheral_ADC1, DISABLE);
    LIGHT_SENS_DIS();
    /* Calculate result */
    if (lights_prev) { res = (bool) (adcVal < (lightLevel + TRH_LIGHTSENS_HYSTR)); }
    else             { res = (bool) (adcVal < (lightLevel /*- TRH_LIGHTSENS_HYSTR*/)); }
    lights_prev = res;
    return res;
}

//EOF