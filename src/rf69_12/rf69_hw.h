// Driver for the RF69 radio chip, radio-specific code.
// 2013-11-06 <jc@wippler.nl> http://opensource.org/licenses/mit-license.php

#define REG_FIFO            0x00
#define REG_OPMODE			    0x01
#define REG_FRFMSB	  		  0x07
#define REG_RSSIVALUE		    0x24
#define REG_IRQFLAGS1       0x27
#define REG_IRQFLAGS2       0x28
#define REG_SYNCVALUE1      0x2F
#define REG_SYNCVALUE2      0x30
#define REG_NODEADRS		    0x39
#define REG_PACKETCONFIG2	  0x3D
#define REG_AESKEY1			    0x3E

#define RF_OPMODE_SLEEP							0x00
#define RF_OPMODE_STANDBY		        0x04
#define RF_OPMODE_RECEIVER	        0x10
#define RF_OPMODE_TRANSMITTER			  0x0C
#define RF_IRQFLAGS1_MODEREADY			0x80
#define RF_IRQFLAGS1_RXREADY		    0x40
#define RF_IRQFLAGS2_FIFOFULL			  0x80
#define RF_IRQFLAGS2_FIFONOTEMPTY   0x40
#define RF_IRQFLAGS2_FIFOOVERRUN    0x10
#define RF_IRQFLAGS2_PACKETSENT			0x08
#define RF_IRQFLAGS2_PAYLOADREADY   0x04

static void writeReg (uint8_t addr, uint8_t value) {
  spiTransfer(((addr | 0x80) << 8) | value);
}

static uint8_t readReg (uint8_t addr) {
  return spiTransfer(addr << 8);
}

static void flushFifo () {
  while (readReg(REG_IRQFLAGS2) & (RF_IRQFLAGS2_FIFONOTEMPTY |
                                      RF_IRQFLAGS2_FIFOOVERRUN))
    readReg(REG_FIFO);
}

static void setMode (int mode) {
  writeReg(REG_OPMODE, (readReg(REG_OPMODE) & 0xE3) | mode);
  // while ((readReg(REG_IRQFLAGS1) & RF_IRQFLAGS1_MODEREADY) == 0)
  //     ;
}

static void initRadio (const uint8_t* init) {
  spiInit();

  do
    writeReg(REG_SYNCVALUE1, 0xAA);
  while (readReg(REG_SYNCVALUE1) != 0xAA);
  
  do
    writeReg(REG_SYNCVALUE1, 0x55);
  while (readReg(REG_SYNCVALUE1) != 0x55);

  while (*init) {
    uint8_t cmd = *init++;
    writeReg(cmd, *init++);  
  }
}
