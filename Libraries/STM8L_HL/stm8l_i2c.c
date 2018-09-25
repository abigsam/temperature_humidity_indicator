/*


*/

#include "stm8l_i2c.h"

/* Defines */
/* I2C ADD0 mask */
#define OAR1_ADD0_Set               ((uint8_t)0x01)
#define OAR1_ADD0_Reset             ((uint8_t)0xFE)

/* For IT handler */
typedef struct {
  I2C_TypeDef*          i2cx;
  uint8_t*              buff_ptr;
  uint8_t               addr;
  uint8_t               bytes;
  I2C_Direction_TypeDef dir;
  I2CState_td           err;
  uint8_t               is_busy;
} I2C_rw_struct;

I2C_rw_struct it_handler = {0, 0, 0, 0, I2C_Direction_Transmitter, I2C_OK, 0};
uint8_t buff = 0u;
uint8_t i2c_byte_cnt = 0u;

//--- Private functions

/**
  * @brief  Control I2C peripheral clocking
  * @note   Private
  * @param  I2Cx: where x can be 1 select the I2C peripheral.
  * @param  st: ENABLE or DISABLE I2C clocking
  * @retval '0' if success; '1' if wrong I2C number
  */
static uint8_t i2c_clk_contr(I2C_TypeDef* I2Cx, FunctionalState st) {
    if (I2C1 == I2Cx) {
        CLK_PeripheralClockConfig(CLK_Peripheral_I2C1, st);
        return 0u;
    }
    return 1u;
}

/**
  * @brief  Check I2C flag for defined time
  * @note   Private
  * @param  I2Cx: where x can be 1 select the I2C peripheral.
  * @param  flag: specifies the flag to check.
  *          This parameter can be one of the following values:
  *            @arg I2C_FLAG_SMBHOST: SMBus host header (Slave mode)
  *            @arg I2C_FLAG_SMBDEFAULT: SMBus default header (Slave mode)
  *            @arg I2C_FLAG_GENCALL: General call header flag (Slave mode)
  *            @arg I2C_FLAG_TRA: Transmitter/Receiver flag
  *            @arg I2C_FLAG_BUSY: Bus busy flag
  *            @arg I2C_FLAG_MSL: Master/Slave flag
  *            @arg I2C_FLAG_SMBALERT: SMBus Alert flag
  *            @arg I2C_FLAG_TIMEOUT: Timeout or Tlow error flag
  *            @arg I2C_FLAG_PECERR: PEC error in reception flag
  *            @arg I2C_FLAG_OVR: Overrun/Underrun flag (Slave mode)
  *            @arg I2C_FLAG_AF: Acknowledge failure flag
  *            @arg I2C_FLAG_ARLO: Arbitration lost flag (Master mode)
  *            @arg I2C_FLAG_BERR: Bus error flag
  *            @arg I2C_FLAG_TXE: Data register empty flag (Transmitter)
  *            @arg I2C_FLAG_RXNE: Data register not empty (Receiver) flag
  *            @arg I2C_FLAG_STOPF: Stop detection flag (Slave mode)
  *            @arg I2C_FLAG_ADD10: 10-bit header sent flag (Master mode)
  *            @arg I2C_FLAG_BTF: Byte transfer finished flag
  *            @arg I2C_FLAG_ADDR: Address sent flag (Master mode) “ADSL”
  * @param  state: check flag for 'RESET' or 'SET'
  * @param  timeout: timeout value in MCU tics
  * @retval 'true' if checking flag is success; 'false' if timeout
  */
static bool i2c_poll_flag(I2C_TypeDef* I2Cx, I2C_FLAG_TypeDef flag, FlagStatus state, uint32_t timeout) {
    while(state != I2C_GetFlagStatus(I2Cx, flag) && timeout) {
        --timeout;
    }
    return (bool)timeout;
}

/**
  * @brief   Check I2C event for SUCCESS state for defined time
  * @note    Private
  * @param   I2Cx: where x can be 1 select the I2C peripheral.
  * @param   event: I2C event.
  *            This parameter can be one of the following values:
  *            @arg I2C_EVENT_SLAVE_TRANSMITTER_ADDRESS_MATCHED: EV1
  *            @arg I2C_EVENT_SLAVE_RECEIVER_ADDRESS_MATCHED: EV1
  *            @arg I2C_EVENT_SLAVE_GENERALCALLADDRESS_MATCHED: EV1
  *            @arg I2C_EVENT_SLAVE_BYTE_RECEIVED: EV2
  *            @arg (I2C_EVENT_SLAVE_BYTE_RECEIVED | I2C_FLAG_GENCALL): EV2
  *            @arg I2C_EVENT_SLAVE_BYTE_TRANSMITTED: EV3
  *            @arg (I2C_EVENT_SLAVE_BYTE_TRANSMITTED | I2C_FLAG_GENCALL): EV3
  *            @arg I2C_EVENT_SLAVE_ACK_FAILURE: EV3_2
  *            @arg I2C_EVENT_SLAVE_STOP_DETECTED: EV4
  *            @arg I2C_EVENT_MASTER_MODE_SELECT: EV5
  *            @arg I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED: EV6
  *            @arg I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED: EV6
  *            @arg I2C_EVENT_MASTER_BYTE_RECEIVED: EV7
  *            @arg I2C_EVENT_MASTER_BYTE_TRANSMITTING: EV8
  *            @arg I2C_EVENT_MASTER_BYTE_TRANSMITTED: EV8_2
  *            @arg I2C_EVENT_MASTER_MODE_ADDRESS10: EV9
  * @param   timeout: timeout value in MCU tics
  * @retval  'true' if checking event is success; 'false' if timeout
  */
static bool i2c_event_poll(I2C_TypeDef* I2Cx, I2C_Event_TypeDef event, uint32_t timeout) {
    while(SUCCESS != I2C_CheckEvent(I2Cx, event) && timeout) {
        --timeout;
    }
    return (bool)timeout;
}


//--- Public functions

/**
  * @brief  Init I2C
  * @note   Public
  * @param  I2Cx: where x can be 1 select the I2C peripheral.
  * @param  OutputClockFrequency: Specifies the output clock frequency in Hz.
  * @param  OwnAddress: Specifies the own address.
  * @param  I2C_Mode: Specifies the addressing mode to apply.
  * @param  I2C_DutyCycle: Specifies the duty cycle to apply in fast mode.
  * @note   The I2C_DutyCycle parameter doesn't have impact when the OutputClockFrequency
  *         is lower than 100KHz.
  * @param  I2C_Ack: Specifies the acknowledge mode to apply.
  * @param  I2C_AcknowledgedAddress: Specifies the acknowledge address to apply.
  * @retval I2C_OK if init success
  */
I2CState_td i2c_init(I2C_TypeDef* I2Cx, uint32_t OutputClockFrequency, uint16_t OwnAddress,
            I2C_Mode_TypeDef I2C_Mode, I2C_DutyCycle_TypeDef I2C_DutyCycle,
            I2C_Ack_TypeDef I2C_Ack, I2C_AcknowledgedAddress_TypeDef I2C_AcknowledgedAddress)
{
    if (i2c_clk_contr(I2Cx, ENABLE)) { return I2C_ERR; }    //Wrong I2C number
    I2C_Init(I2Cx, OutputClockFrequency,  OwnAddress,
             I2C_Mode, I2C_DutyCycle,
             I2C_Ack, I2C_AcknowledgedAddress);
    I2C_Cmd(I2Cx, ENABLE);
    it_handler.i2cx = I2Cx;
    it_handler.buff_ptr = &buff;
    it_handler.addr = 0;
    it_handler.bytes = 0;
    it_handler.dir = I2C_Direction_Transmitter;
    it_handler.err = I2C_OK;
    it_handler.is_busy = 0;
    return I2C_OK;
}

/**
  * @brief  Deinit I2C
  * @note   Public
  * @param  I2Cx: where x can be 1 select the I2C peripheral.
  * @retval None
  */
void i2c_deinit(I2C_TypeDef* I2Cx) {
    I2C_Cmd(I2Cx, DISABLE);
    I2C_DeInit(I2Cx);
    i2c_clk_contr(I2Cx, DISABLE);
}

/**
  * @brief  Write byte(s) in Master polling mode
  * @note   Public
  * @param  I2Cx: where x can be 1 select the I2C peripheral.
  * @param  addr: slave address
  * @param  ptr: pointer write data
  * @param  bnum: number write bytes
  * @param  timeout: timeout in MCU ticks
  * @retval I2C_OK if success
  */
I2CState_td i2c_master_write_poll(I2C_TypeDef* I2Cx, uint8_t addr, uint8_t *ptr, uint8_t bnum, uint32_t timeout) {
    if (!(bnum)) { return I2C_ERR; }
    //
    if (!i2c_poll_flag(I2Cx, I2C_FLAG_BUSY, RESET, timeout)) { return I2C_ERR_BUSY; }
    I2C_GenerateSTART(I2Cx, ENABLE);
    //EV5
    if (!i2c_event_poll(I2Cx, I2C_EVENT_MASTER_MODE_SELECT, timeout)) { return I2C_ERR_TIMEOUT; }
    I2C_Send7bitAddress(I2Cx, addr, I2C_Direction_Transmitter);
    //EV6
    if (!i2c_event_poll(I2Cx, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED, timeout)) { return I2C_ERR_TIMEOUT; }
    /* Transfer data */
    for (; bnum > 0u; --bnum) {
        I2C_SendData(I2Cx, *ptr);
        //EV8
        if (!i2c_event_poll(I2Cx, I2C_EVENT_MASTER_BYTE_TRANSMITTED, timeout)) { return I2C_ERR_TIMEOUT; }
        if (bnum > 1u) { ++ptr; }  //Increment pointer if need
    }
    I2C_GenerateSTOP(I2Cx, ENABLE);
    return I2C_OK;
}


/**
  * @brief  Read byte(s) in Master polling mode
  * @note   Public
  * @param  I2Cx: where x can be 1 select the I2C peripheral.
  * @param  addr: slave address
  * @param  ptr: pointer where read data to write
  * @param  bnum: number read bytes
  * @param  timeout: timeout in MCU ticks
  * @retval I2C_OK if success
  */
I2CState_td i2c_master_read_poll(I2C_TypeDef* I2Cx, uint8_t addr, uint8_t *ptr, uint8_t bnum, uint32_t timeout) {
    if (!(bnum)) { return I2C_ERR; }
    //
    if (!i2c_poll_flag(I2Cx, I2C_FLAG_BUSY, RESET, timeout)) { return I2C_ERR_BUSY; }
    I2C_GenerateSTART(I2Cx, ENABLE);
    //EV5
    if (!i2c_event_poll(I2Cx, I2C_EVENT_MASTER_MODE_SELECT, timeout)) { return I2C_ERR_TIMEOUT; }
    I2C_Send7bitAddress(I2Cx, addr, I2C_Direction_Receiver);
    /* Receive data */
    if (1u == bnum) { //if need to read one byte
        I2C_AcknowledgeConfig(I2Cx, DISABLE);
        if (!i2c_poll_flag(I2Cx, I2C_FLAG_ADDR, SET, timeout)) { return I2C_ERR_TIMEOUT; }
        (void)(I2Cx->SR3);  //Clear ADDR flag
        I2C_GenerateSTOP(I2Cx, ENABLE);
        //EV7
        if (!i2c_poll_flag(I2Cx, I2C_FLAG_RXNE, SET, timeout)) { return I2C_ERR_TIMEOUT; }
        *ptr = I2C_ReceiveData(I2Cx);
    }
    else if (2u == bnum) { //If need to read 2 bytes
        //EV6
        if (!i2c_event_poll(I2Cx, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED, timeout)) { return I2C_ERR_TIMEOUT; }
        I2C_AcknowledgeConfig(I2Cx, DISABLE);
        //EV7_3
        if (!i2c_poll_flag(I2Cx, I2C_FLAG_BTF, SET, timeout)) { return I2C_ERR_TIMEOUT; }
        I2C_GenerateSTOP(I2Cx, ENABLE);
        *ptr = I2C_ReceiveData(I2Cx);
        ++ptr;
        *ptr = I2C_ReceiveData(I2Cx);
    }
    else { //If need to read more then 3 bytes
        //EV6
        if (!i2c_event_poll(I2Cx, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED, timeout)) { return I2C_ERR_TIMEOUT; }
        for (; bnum > 3u; --bnum) {
            //EV7
            if (!i2c_event_poll(I2Cx, I2C_EVENT_MASTER_BYTE_RECEIVED, timeout)) { return I2C_ERR_TIMEOUT; }
            *ptr = I2C_ReceiveData(I2Cx);
            ++ptr;
        }
        //EV7_2
        if (!i2c_poll_flag(I2Cx, I2C_FLAG_BTF, SET, timeout)) { return I2C_ERR_TIMEOUT; }
        I2C_AcknowledgeConfig(I2Cx, DISABLE);
        //Read N-2
        *ptr = I2C_ReceiveData(I2Cx);
        ++ptr;
        I2C_GenerateSTOP(I2Cx, ENABLE);
        //Read N-1
        *ptr = I2C_ReceiveData(I2Cx);
        ++ptr;
        if (!i2c_poll_flag(I2Cx, I2C_FLAG_RXNE, SET, timeout)) { return I2C_ERR_TIMEOUT; }
        //Read N
        *ptr = I2C_ReceiveData(I2Cx);
    }
    I2C_AcknowledgeConfig(I2Cx, ENABLE);
    return I2C_OK;
}


I2CState_td i2c_master_write_IT(I2C_TypeDef* I2Cx, uint8_t addr, uint8_t *ptr, uint8_t bnum)
{
  /* Check inputs */
  if (!(bnum)) { return I2C_ERR; }
  if (I2C_GetFlagStatus(I2Cx, I2C_FLAG_BUSY) != RESET) { return I2C_ERR_BUSY; }
  /* Init IT handler */
  it_handler.i2cx = I2Cx;
  it_handler.buff_ptr = ptr;
  it_handler.bytes = bnum;
  it_handler.addr = addr;
  it_handler.dir = I2C_Direction_Transmitter;
  /* Enable I2C interrupts */
  I2C_ITConfig(I2Cx, (I2C_IT_BUF | I2C_IT_EVT | I2C_IT_ERR), ENABLE);
  enableInterrupts();
  /* Generate START condition */
  I2C_GenerateSTART(I2Cx, ENABLE);
  do {
    wfi();
  }
  while(it_handler.is_busy);
  disableInterrupts();
  I2C_ITConfig(I2Cx, (I2C_IT_BUF | I2C_IT_EVT | I2C_IT_ERR), DISABLE); //Disable all interrupts
  return I2C_OK;
}


I2CState_td i2c_master_read_IT(I2C_TypeDef* I2Cx, uint8_t addr, uint8_t *ptr, uint8_t bnum)
{
  if (!(bnum)) { return I2C_ERR; }
  if (I2C_GetFlagStatus(I2Cx, I2C_FLAG_BUSY) != RESET) { return I2C_ERR_BUSY; }
  /* Init IT handler */
  it_handler.i2cx = I2Cx;
  it_handler.buff_ptr = ptr;
  it_handler.bytes = bnum;
  it_handler.addr = addr;
  it_handler.dir = I2C_Direction_Receiver;
  /* Enable I2C interrupts */
  I2C_ITConfig(I2Cx, (I2C_IT_BUF | I2C_IT_EVT | I2C_IT_ERR), ENABLE);
  enableInterrupts();
  /* Generate START condition */
  I2C_GenerateSTART(I2Cx, ENABLE);
  do {
    wfi();
  }
  while(it_handler.is_busy);
  disableInterrupts();
  I2C_ITConfig(I2Cx, (I2C_IT_BUF | I2C_IT_EVT | I2C_IT_ERR), DISABLE); //Disable all interrupts
  return it_handler.err;//I2C_OK;
}


void i2c_it_handler(void)
{
  /* Read */
  if (it_handler.i2cx->SR2 != 0) {
    //if (it_handler.i2cx->SR2 & I2C_SR2_SMBALERT)     { it_handler.err = I2C_ERR; } /*!< SMBus Alert */
    //else if (it_handler.i2cx->SR2 & I2C_SR2_TIMEOUT) { it_handler.err = I2C_ERR; } /*!< Time out or TLow error */
    //else if (it_handler.i2cx->SR2 & I2C_SR2_WUFH)    { it_handler.err = I2C_ERR; } /*!< Wake-up from Halt */
    //else if (it_handler.i2cx->SR2 & I2C_SR2_PECERR)  { it_handler.err = I2C_ERR; } /*!< PEC error in reception */
    //else if (it_handler.i2cx->SR2 & I2C_SR2_OVR)     { it_handler.err = I2C_ERR; } /*!< Overrun/Underrun */
    //else if (it_handler.i2cx->SR2 & I2C_SR2_AF)      { it_handler.err = I2C_ERR; } /*!< Acknowledge Failure */
    //else if (it_handler.i2cx->SR2 & I2C_SR2_ARLO)    { it_handler.err = I2C_ERR; } /*!< Arbitration Lost (master mode) */
    //else if (it_handler.i2cx->SR2 & I2C_SR2_BERR)    { it_handler.err = I2C_ERR_BUSY; } /*!< Bus Error */
    it_handler.err = I2C_ERR;
  }
  else {
    it_handler.err = I2C_OK;
  }
  
  /* Check for START condition (master) */
  if (it_handler.i2cx->SR1 & I2C_SR1_SB) {
    //SB (Start Bit) is cleared by software sequence: a read operation to
    // I2C_SR1 register (I2C_GetITStatus()) followed by a write operation to
    // I2C_DR register (I2C_SendData()).
    if (it_handler.dir != I2C_Direction_Transmitter) {
      it_handler.i2cx->DR = it_handler.addr | OAR1_ADD0_Set; /* Set the address bit0 for read */
       it_handler.i2cx->CR2 |= I2C_CR2_ACK; // Set ACK for reading
    } else {
      it_handler.i2cx->DR = it_handler.addr & OAR1_ADD0_Reset; /* Reset the address bit0 for write */
    }
    i2c_byte_cnt = it_handler.bytes; //Need for receiving bytes (when mor than 2 bytese need receive, see below)
    it_handler.is_busy = 1;
    return;
  }
  
  /* Check for success ADDRESS transfer (ACK received) */
  if (it_handler.i2cx->SR1 & I2C_SR1_ADDR) {
    //ADDR (Address sent) is cleared by software sequence: a read operation
    // to I2C_SR1 register (I2C_GetITStatus()) followed by a read operation
    // to I2C_SR3 register ((void)(I2Cx->SR3)).
    (void)(it_handler.i2cx->SR3);
    if (it_handler.dir != I2C_Direction_Transmitter) {
      if (1 == i2c_byte_cnt) { //If need receive just one byte
        it_handler.i2cx->CR2 &= ~I2C_CR2_ACK;
        it_handler.i2cx->CR2 |= I2C_CR2_STOP;
      }
      if (2 == i2c_byte_cnt) { //If need receive 2 bytes
        it_handler.i2cx->CR2 &= ~I2C_CR2_ACK;
      }
    }
    return;
  }
  
  /* TX buffer */
  if ((it_handler.i2cx->SR1 & I2C_SR1_TXE) && !(it_handler.i2cx->SR1 & I2C_SR1_BTF)) {
    if (it_handler.bytes != 0) {
      it_handler.i2cx->DR = *(it_handler.buff_ptr);
      it_handler.bytes--;
      it_handler.buff_ptr++;
    }
    return;
  }
  
  /* Byte transfer finished */
  if ((it_handler.i2cx->SR1 & I2C_SR1_TXE) && (it_handler.i2cx->SR1 & I2C_SR1_BTF)) {
    /* Write */
    it_handler.i2cx->CR2 |= I2C_CR2_STOP;
    it_handler.is_busy = 0;
    return;
  }
  
  /* RX buffer -- if receive one byte */
  if ((it_handler.i2cx->SR1 & I2C_SR1_RXNE) && !(it_handler.i2cx->SR1 & I2C_SR1_BTF)) {
    if (1 == i2c_byte_cnt) {
      *(it_handler.buff_ptr) = it_handler.i2cx->DR;
      it_handler.is_busy = 0;
    }
    return;
  }
  
  /* RX buffer -- receive more than 1 byte */
  if ((it_handler.i2cx->SR1 & I2C_SR1_RXNE) && (it_handler.i2cx->SR1 & I2C_SR1_BTF)) {
    switch(i2c_byte_cnt) {
    case 2: //If just 2 bytes, NACK configured in ADDRESS interrupt
      it_handler.i2cx->CR2 |= I2C_CR2_STOP;
      *(it_handler.buff_ptr)     = it_handler.i2cx->DR;
      *(it_handler.buff_ptr + 1) = it_handler.i2cx->DR;
      it_handler.is_busy = 0; //End receiving
    break;
    
    case 3:
      it_handler.i2cx->CR2 &= ~I2C_CR2_ACK; //Set NACK for the next received byte
      *(it_handler.buff_ptr) = it_handler.i2cx->DR; //Free shift register for the next byte (it received with NACK)
      it_handler.buff_ptr++;
      i2c_byte_cnt--;
    break;
    
    default: //More than 3 bytes
      *(it_handler.buff_ptr) = it_handler.i2cx->DR;
      it_handler.buff_ptr++;
      i2c_byte_cnt--;
    break;
    }
    return;
  }
  
}

//EOF