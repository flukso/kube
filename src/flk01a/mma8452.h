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
#define MMA8452_REG_CTRL_REG4 0x2D
#define MMA8452_REG_CTRL_REG5 0x2E

#define MMA8452_CMD_TRANS_XYZ_E 0x0E
#define MMA8452_CMD_TRANS_THS 0x04
#define MMA8452_CMD_TRANS_COUNT 0x02
#define MMA8452_CMD_INT_EN_TRANS 0x20
#define MMA8452_CMD_INT1_TRANS 0x20
#define MMA8452_CMD_ACT_12HZ 0x69

void PININT1_IRQHandler(void);
void mma8452_init(void);
ErrorCode_t mma8452_whoami(void);
ErrorCode_t mma8452_trans_init(void);

#endif

