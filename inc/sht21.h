/*


*/

#ifndef __SHT21_H
#define __SHT21_H

#ifdef __cplusplus
 extern "C" {
#endif

#include "stm8l_i2c.h"
#include "delay.h"

//#define SHT21_PLATFORM_STM8L
//#define SHT21_PLATFORM_STM32

#define SHT21_CHECK_CRC     (0)     //Change to '1' if need check CRC
#define SHT21_I2C_INST      I2C1    //Used for STM8L & STM32 platforms
#define SHT21_SCL_PORT      GPIOC
#define SHT21_SCL_PIN       GPIO_Pin_1
#define SHT21_SDA_PORT      GPIOC
#define SHT21_SDA_PIN       GPIO_Pin_0
#define SHT21_I2C_FREQ      (10000)

//#define SHT21_DELAY_MS(d)   { delay_lowp_ms(d); }
#define SHT21_DELAY_MS(d)   { sleep_ms(d); }


/* Types */
typedef enum {
    SHT21_OK          = 0x00u, //HAL_OK = 0x00U
    SHT21_HAL_ERR     = 0x01U, //HAL_ERROR    = 0x01U,
    SHT21_HAL_BUSY    = 0x02U, //HAL_BUSY     = 0x02U,
    SHT21_HAL_TIMEOUT = 0x03U, //HAL_TIMEOUT  = 0x03U
    SHT21_CRC_ERROR   = 0x04U,
    SHT21_MEAS_TIME_ERROR = 0x05U
} SHT21State_td;

typedef enum {
    SHT21_12H_14T = 0x00U, //Default
    SHT21_8H_12T  = 0x01U,
    SHT21_10H_13T = 0x80U,
    SHT21_11H_11T = 0x81U
} SHT21Resol_td;

typedef struct {
    SHT21Resol_td resol; //Default SHT21_12H_14T
    bool       heater;   //If 'false' heater is off (default)
    bool       end_batt; //if 'true’: VDD < 2.25V ('false' is default)
} SHT21Config_td;

//typedef unsigned long long     uint64_t;

//--- Public functions
SHT21State_td SHT21_init(void);
void SHT21_deinit(void);
SHT21State_td SHT21_softReset(void);
SHT21State_td SHT21_setResolution(SHT21Resol_td res);
SHT21State_td SHT21_setHeater(bool heater_state);
SHT21State_td SHT21_readConfig(SHT21Config_td *conf);
SHT21State_td SHT21_getRH(float *res);
SHT21State_td SHT21_getT(float *res);
SHT21State_td SHT21_getRHint(int16_t *res);
SHT21State_td SHT21_getTint(int16_t *res);


#ifdef __cplusplus
}
#endif

#endif  //__SHT21_H