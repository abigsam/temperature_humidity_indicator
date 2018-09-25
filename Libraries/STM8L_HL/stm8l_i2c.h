/*


*/

#ifndef __STM8L_I2C_H
#define __STM8L_I2C_H

#ifdef __cplusplus
 extern "C" {
#endif

#include "stm8l15x.h"



//--- Macroses
//#define ADDR_READ(a)        ((a << 1) | 0x01)
//#define ADDR_WRITE(a)       ((a << 1) & 0xfe)

//--- Types
typedef enum {
    I2C_OK          = 0x00u,
    I2C_ERR         = 0x01U,
    I2C_ERR_BUSY    = 0x02U,
    I2C_ERR_TIMEOUT = 0x03U
} I2CState_td;

//--- Private functions
static uint8_t i2c_clk_contr(I2C_TypeDef* I2Cx, FunctionalState st);
static bool i2c_poll_flag(I2C_TypeDef* I2Cx, I2C_FLAG_TypeDef flag, FlagStatus state, uint32_t timeout);
static bool i2c_event_poll(I2C_TypeDef* I2Cx, I2C_Event_TypeDef event, uint32_t timeout);

//--- Public functions/poll
I2CState_td i2c_init(I2C_TypeDef* I2Cx, uint32_t OutputClockFrequency, uint16_t OwnAddress,
            I2C_Mode_TypeDef I2C_Mode, I2C_DutyCycle_TypeDef I2C_DutyCycle,
            I2C_Ack_TypeDef I2C_Ack, I2C_AcknowledgedAddress_TypeDef I2C_AcknowledgedAddress);
void i2c_deinit(I2C_TypeDef* I2Cx);
I2CState_td i2c_master_write_poll(I2C_TypeDef* I2Cx, uint8_t addr, uint8_t *ptr, uint8_t bnum, uint32_t timeout);
I2CState_td i2c_master_read_poll(I2C_TypeDef* I2Cx, uint8_t addr, uint8_t *ptr, uint8_t bnum, uint32_t timeout);

//--- Public functions/interrupt
I2CState_td i2c_master_write_IT(I2C_TypeDef* I2Cx, uint8_t addr, uint8_t *ptr, uint8_t bnum);
I2CState_td i2c_master_read_IT(I2C_TypeDef* I2Cx, uint8_t addr, uint8_t *ptr, uint8_t bnum);
void i2c_it_handler(void);

#ifdef __cplusplus
}
#endif

#endif  //__STM8L_I2C_H