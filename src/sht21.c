/*


*/


#include "sht21.h"


#define SHT21_ADDR          ((uint8_t) 0x40 << 1)
/* User register (configuration) */
#define USER_REG_DEF        ((uint8_t) (1 << 1))                 //Bit#1 always '1'
#define RESOLUTION_MASK     ((uint8_t) ((1 << 7) | (1 << 0)))    //Resolution mask
#define HEATER_MASK         ((uint8_t) (1 << 2))                 //On-chip heater bit mask
#define END_BATT_MASK       ((uint8_t) (1 << 6))                 //End battery bit mask
#define IS_END_OF_BATT(r)   ((r & END_BATT_MASK) ? TRUE : FALSE) //Return 'true' if VDD < 2.25V
#define IS_HEATER_ON(r)     ((r & HEATER_MASK) ? TRUE : FALSE)   //Return 'true' if heater is on
#define SET_HEATER(s)       (s ? HEATER_MASK : 0u)               //Control heater
/* Softreset delay */
#define SOFT_RESET_MS       ((uint32_t) 15)
/* CRC */
#define CRC_POLYNOMIAL      ((uint16_t) ((1 << 8) | (1 << 5) | (1 << 4) | (1 << 0))) //0x131, P(x)=x^8+x^5+x^4+1 = 100110001

/* Others */
//#define I2C_WRITE(h, a, p, n, t)    i2c_master_write_poll(h, a, p, n, t)
#define I2C_WRITE(h, a, p, n, t)    i2c_master_write_IT(h, a, p, n)
//#define I2C_READ(h, a, p, n, t)     i2c_master_read_poll(h, a, p, n, t)
#define I2C_READ(h, a, p, n, t)     i2c_master_read_IT(h, a, p, n)
#define I2C_TIMEOUT         ((uint32_t) 10000)
#define ATTMEPTS_MAX        ((uint8_t) 2)   //Number of attempts while no-hold measure command

/* Macroses */
#define CLR_STATUS_BITS(d)  (d & (uint16_t)0xFFFC)
#define DEFAULT_CONFIG(c)       { c.resol = SHT21_12H_14T; \
                                  c.heater = FALSE; \
                                  c.end_batt = FALSE; }
#define PARSE_USER_REG(r, d)    { r->resol = (SHT21Resol_td)(d & RESOLUTION_MASK); \
                                  r->heater = IS_HEATER_ON(d); \
                                  r->end_batt = IS_END_OF_BATT(d); }
#define MAKE_16BIT(m,l)         ( ((uint16_t)m << 8) | (uint16_t)l )

/* Types */
typedef enum {
    CMD_T_MEAS_HOLD   = 0xe3U,
    CMD_RH_MEAS_HOLD  = 0xe5U,
    CMD_T_MEAS_NHOLD  = 0xf3U,
    CMD_RH_MEAS_NHOLD = 0xf5U,
    CMD_WR_USER_REG   = 0xe6U,
    CMD_RD_USER_REG   = 0xe7U,
    CMD_SOFT_RES      = 0xfeU
} SHT21command_td;


//--- Private variables
SHT21Config_td sht21_conf;


//--- Private functions definitions
static float convert_rh(uint16_t raw);
static float convert_t(uint16_t raw);
static int16_t convert_rh_int(uint16_t raw);
static int16_t convert_t_int(uint16_t raw);
static bool crc_check(uint8_t *data, uint8_t nbyte, uint8_t crc_in);
static void measure_delay(SHT21Resol_td res);
static SHT21State_td get_meas_data(SHT21command_td cmd, uint16_t *mdata);


//--- Private functions

/**
  * @brief  Convert SHT21 raw data to relative humidity value
  * @note	Private
  * @note   Convertion formula: RH% = -6 + 125*(raw/2^16)
  * @param  raw: 16 bit data from SHT21
  * @retval RH value in %
  */
static float convert_rh(uint16_t raw) {
    float res = 0.0;
    uint32_t temp = (uint32_t) CLR_STATUS_BITS(raw);
    temp *= 1000u;
    temp >>= 16;
    res = (float)temp / 8.0; //1000/8 = 125
    res -= 6.0;
    return res;
}

/**
  * @brief  Convert SHT21 raw data to temperature value (result in Celsius)
  * @note	Private
  * @note   Convertion formula: T = -46.85 + 175.72*(raw/2^16)
  * @param  raw: 16 bit data from SHT21
  * @retval T value in C
  */
static float convert_t(uint16_t raw) {
    float res = (float) CLR_STATUS_BITS(raw);
    res *= 175.72;
    res /= (float)(1 << 8);
    res /= (float)(1 << 8);
    res -= 46.85;
    return res;
}

/**
  * @brief  Convert SHT21 raw data to relative humidity value (in 0.1 percent)
  * @note	Private
  * @note   Convertion formula: 0.1RH% = -60 + 1250*(raw/2^16)
  * @param  raw: 16 bit data from SHT21
  * @retval integer RH value in 0.1%
  */
static int16_t convert_rh_int(uint16_t raw) {
    int32_t res = (int32_t) CLR_STATUS_BITS(raw);
    res *= 1250;
    res /= (1 << 8);
    res /= (1 << 8);
    res -= 60;
    if (res < 0) { return 0; }
    return (int16_t)res;
}

/**
  * @brief  Convert SHT21 raw data to temperature value (0.1 degrees)
  * @note	Private
  * @note   Convertion formula: 0.1T = -469 + 1757*(raw/2^16)
  * @param  raw: 16 bit data from SHT21
  * @retval integer T value in 0.1C
  */
static int16_t  convert_t_int(uint16_t raw) {
    int32_t res = (int32_t) CLR_STATUS_BITS(raw);
    res *= 1757;
    res /= (1 << 8);
    res /= (1 << 8);
    res -= 469;
    return (int16_t)res;
}

/**
  * @brief  Check CRC data from SHT21
  * @note	Private
  * @note	Polynomial is: x^8 + x^5 + x^4 + 1
  * @param  data: pointer to SHT21 raw data
  * @param  nbyte: number of byte
  * @param  crc_in: CRC from SHT21
  * @retval 'true' if CRC OK; 'false' if CRC error
  */
static bool crc_check(uint8_t *data, uint8_t nbyte, uint8_t crc_in) {
    uint8_t crc = 0u;	
    uint8_t byteCtr = 0u;
    uint8_t bit = 8u;
    //calculates 8-Bit checksum with given polynomial
    for (byteCtr = 0u; byteCtr < nbyte; ++byteCtr) {
        crc ^= *(data + byteCtr);
        for (bit = 8u; bit > 0u; --bit) {
            if (crc & 0x80U) { crc = (crc << 1) ^ CRC_POLYNOMIAL; }
            else { crc = (crc << 1); }
        }
    }
    return ((crc == crc_in) ? TRUE : FALSE);
}

/**
  * @brief  Select delay for current resolution
  * @note	Private
  * @note	See Table 7 in SHT21 datasheet
  * @param  res: current resolution
  * @retval None
  */
static void measure_delay(SHT21Resol_td res) {
    uint32_t delay = 0u;
    switch(res) {
        case SHT21_8H_12T:
            delay = 22u; break;
        case SHT21_10H_13T:
            delay = 43u; break;
        case SHT21_11H_11T:
            delay = 11u; break;
        default: //SHT21_12H_14T
            delay = 85u; break;
    }
    SHT21_DELAY_MS(delay);
}

/**
  * @brief  Get measure data from SHT21
  * @note	Private
  * @param  cmd: command (CMD_T_MEAS_NHOLD or CMD_RH_MEAS_NHOLD)
  * @param  mdata: pointer to where to write read value
  * @retval None
  */
static SHT21State_td get_meas_data(SHT21command_td cmd, uint16_t *mdata) {
    uint8_t wdata = (uint8_t)cmd;
    uint8_t rdata[3] = {0u, 0u, 0u};
    uint8_t hal_st = 0u;
    uint8_t attempts = 0u;
    //
    if ((cmd != CMD_T_MEAS_NHOLD) && (cmd != CMD_RH_MEAS_NHOLD)) {
        return SHT21_HAL_ERR;
    }
    if (hal_st = (uint8_t)I2C_WRITE(SHT21_I2C_INST, SHT21_ADDR, &wdata, 1u, I2C_TIMEOUT)) {
        return (SHT21State_td)hal_st;
    }
    for (attempts = 0u; attempts < ATTMEPTS_MAX; ++attempts) {
        measure_delay(sht21_conf.resol);
        hal_st = (uint8_t)I2C_READ(SHT21_I2C_INST, SHT21_ADDR, rdata, 3u, I2C_TIMEOUT);
        if (I2C_OK == hal_st) { break; }
    }
    if (I2C_OK != hal_st) { return SHT21_MEAS_TIME_ERROR; }
#if (SHT21_CHECK_CRC == 1)    
    if ( !crc_check(rdata, 2u, rdata[2]) ) { return SHT21_CRC_ERROR; }
#endif
    *mdata = MAKE_16BIT(rdata[0], rdata[1]);
    return SHT21_OK;
}

//--- Public functions

/**
  * @brief  Init I2C for SHT21
  * @note	Public function
  * @param  None
  * @retval SHT21 status
  */
SHT21State_td SHT21_init(void) {
    /* Configurate I2C for STM8L */
    GPIO_Init(SHT21_SCL_PORT, SHT21_SCL_PIN, GPIO_Mode_In_PU_No_IT);
    GPIO_Init(SHT21_SDA_PORT, SHT21_SDA_PIN, GPIO_Mode_In_PU_No_IT);
    if (I2C_OK != i2c_init(SHT21_I2C_INST, (uint32_t)100000, 0x00,
                 I2C_Mode_I2C, I2C_DutyCycle_2,
                 I2C_Ack_Enable, I2C_AcknowledgedAddress_7bit))
    { return SHT21_HAL_ERR; }
    SHT21_DELAY_MS(50u);
    return SHT21_softReset();
}

/**
  * @brief  De-init I2C for SHT21
  * @note	Public function
  * @param  None
  * @retval None
  */
void SHT21_deinit(void) {
    /* De-init sequence I2C for STM8L */
    i2c_deinit(SHT21_I2C_INST);
}

/**
  * @brief  Send soft reset command to SHT21
  * @note	Public function
  * @param  None
  * @retval SHT21 status
  */
SHT21State_td SHT21_softReset(void) {
    uint8_t wdata = CMD_SOFT_RES;
    uint8_t hal_st = 0u;
    SHT21State_td rd_state;
    //
    if (hal_st = (uint8_t)I2C_WRITE(SHT21_I2C_INST, SHT21_ADDR, &wdata, 1u, I2C_TIMEOUT)) {
        return (SHT21State_td)hal_st;
    }
    SHT21_DELAY_MS(SOFT_RESET_MS);
    if (rd_state = SHT21_readConfig(&sht21_conf)) {
      return rd_state; }
    return SHT21_OK;
}

/**
  * @brief  Write resolution to SHT21
  * @note	Public function
  * @param  res: resolution
  * @retval SHT21 status
  */
SHT21State_td SHT21_setResolution(SHT21Resol_td res) {
    uint8_t wdata[2] = {0u, 0u};
    uint8_t hal_st = 0u;
    SHT21State_td rd_state;
    //
    if (rd_state = SHT21_readConfig(&sht21_conf)) { return rd_state; }
    wdata[0] = (uint8_t)CMD_WR_USER_REG;
    wdata[1] = USER_REG_DEF | (uint8_t)res | SET_HEATER(sht21_conf.heater);
    if (hal_st = (uint8_t)I2C_WRITE(SHT21_I2C_INST, SHT21_ADDR, wdata, 2u, I2C_TIMEOUT)) {
        return (SHT21State_td)hal_st;
    }
    sht21_conf.resol = res;
    return SHT21_OK;
}


/**
  * @brief  Control heater on SHT21
  * @note	Public function
  * @param  heater_state: ON/OFF heater
  * @retval SHT21 status
  */
SHT21State_td SHT21_setHeater(bool heater_state) {
    uint8_t wdata[2] = {0u, 0u};
    uint8_t hal_st = 0u;
    SHT21State_td rd_state;
    //
    if (rd_state = SHT21_readConfig(&sht21_conf)) { return rd_state; }
    wdata[0] = (uint8_t)CMD_WR_USER_REG;
    wdata[1] = USER_REG_DEF | (uint8_t)sht21_conf.resol | SET_HEATER(heater_state);
    if (hal_st = (uint8_t)I2C_WRITE(SHT21_I2C_INST, SHT21_ADDR, wdata, 2u, I2C_TIMEOUT)) {
        return (SHT21State_td)hal_st;
    }
    sht21_conf.heater = heater_state;
    return SHT21_OK;
}

/**
  * @brief  Read configuration from SHT21
  * @note	Public function
  * @param  conf: pointer to configuration
  * @retval SHT21 status
  */
SHT21State_td SHT21_readConfig(SHT21Config_td *conf) {
    uint8_t wdata = (uint8_t)CMD_RD_USER_REG;
    uint8_t rdata = 0u;
    uint8_t hal_st = 0u;
    //
    if (hal_st = (uint8_t)I2C_WRITE(SHT21_I2C_INST, SHT21_ADDR, &wdata, 1u, I2C_TIMEOUT)) {
        return (SHT21State_td)hal_st;
    }
    if (hal_st = (uint8_t)I2C_READ(SHT21_I2C_INST, SHT21_ADDR, &rdata, 1u, I2C_TIMEOUT)) {
        return (SHT21State_td)hal_st;
    }
    PARSE_USER_REG(conf, rdata);
    return SHT21_OK;
}

/**
  * @brief  Read humidity in persents from sensor
  * @note	Public function
  * @param  res: pointer to output temperature 
  * @retval SHT21 status
  */
SHT21State_td SHT21_getRH(float *res) {
    SHT21State_td st;
    uint16_t mval = 0u;
    if (st = get_meas_data(CMD_RH_MEAS_NHOLD, &mval)) { return st; }
    *res = convert_rh(mval);
    return SHT21_OK;
}

/**
  * @brief  Read temperature (Celsus) from sensor
  * @note	Public function
  * @param  res: rointer to output temperature 
  * @retval SHT21 status
  */
SHT21State_td SHT21_getT(float *res) {
    SHT21State_td st;
    uint16_t mval = 0u;
    if (st = get_meas_data(CMD_T_MEAS_NHOLD, &mval)) { return st; }
    *res = convert_t(mval);
    return SHT21_OK;
}

/**
  * @brief  Read humidity from sensor and out value in 0.1 percent
  * @note	Public function
  * @param  res: rointer to output temperature in 0.1 percent
  * @retval SHT21 status
  */
SHT21State_td SHT21_getRHint(int16_t *res) {
    SHT21State_td st;
    uint16_t mval = 0u;
    if (st = get_meas_data(CMD_RH_MEAS_NHOLD, &mval)) { return st; }
    *res = convert_rh_int(mval);
    return SHT21_OK;
}

/**
  * @brief  Read temperature (Celsus) from sensor out value in 0.1 degrees (*C)
  * @note	Public function
  * @param  res: rointer to output temperature in 0.1 degrees
  * @retval SHT21 status
  */
SHT21State_td SHT21_getTint(int16_t *res) {
    SHT21State_td st;
    uint16_t mval = 0u;
    if (st = get_meas_data(CMD_T_MEAS_NHOLD, &mval)) { return st; }
    *res = convert_t_int(mval);
    return SHT21_OK;
}

// EOF