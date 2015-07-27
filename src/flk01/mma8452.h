#ifndef __MMA8452_H__
#define __MMA8452_H__

#include "i2c.h"

#define MMA8452_ID "mma8452"
#define MMA8452_ADDRESS 0x1C
#define MMA8452_REG_WHOAMI 0x0D
#define MMA8452_REG_TRANS_CFG 0x1D
#define MMA8452_REG_TRANS_SRC 0x1E
#define MMA8452_REG_TRANS_THS 0x1F
#define MMA8452_REG_TRANS_COUNT 0x20
#define MMA8452_REG_CTRL_REG1 0x2A
#define MMA8452_REG_CTRL_REG2 0x2B
#define MMA8452_REG_CTRL_REG4 0x2D
#define MMA8452_REG_CTRL_REG5 0x2E

#define MMA8452_CMD_TRANS_XYZ_E 0x0E
#define MMA8452_CMD_TRANS_THS 0x01
#define MMA8452_CMD_TRANS_COUNT 0x01
#define MMA8452_CMD_INT_EN_TRANS 0x20
#define MMA8452_CMD_INT1_TRANS 0x20
#define MMA8452_CMD_LNOISE_ACT_12HZ 0x6D
#define MMA8452_CMD_LPOWER 0x1B
#define MMA8452_CMD_LNOISE_LPOWER 0x09

void PININT1_IRQHandler(void);
void mma8452_init(void);
uint8_t mma8452_whoami(void);
ErrorCode_t mma8452_trans_init(void);
ErrorCode_t mma8452_trans_clear(void);

#endif
