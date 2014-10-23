// Driver for the RF69 radio chip, used in RF12 compatibility mode.
// 2013-11-06 <jc@wippler.nl> http://opensource.org/licenses/mit-license.php

#include "rf69_12.h"
#include "rf69_mcu.h"
#include "rf69_hw.h"

// #include <stdio.h>

// transceiver states, these determine what to do with each interrupt
enum { TXCRC1, TXCRC2, TXTAIL, TXDONE, TXIDLE, TXRECV };

static uint8_t nodeid;              // address of this node
static uint8_t group;               // network group
static volatile uint8_t rxfill;     // number of data bytes in rf12_buf
static volatile int8_t rxstate;     // current transceiver state

// maximum transmit / receive buffer: 3 header + data + 2 crc bytes
#define RF_MAX   (RF12_MAXDATA + 5)

volatile uint16_t rf12_crc;         // running crc value
volatile uint8_t rf12_buf[RF_MAX];  // recv/xmit buf, including hdr & crc bytes

// adapted from http://akbar.marlboro.edu/~mahoney/support/alg/alg/node186.html

static const uint16_t crc_16_table[16] = {
  0x0000, 0xCC01, 0xD801, 0x1400, 0xF001, 0x3C00, 0x2800, 0xE401,
  0xA001, 0x6C00, 0x7800, 0xB401, 0x5000, 0x9C01, 0x8801, 0x4400
};

uint16_t _crc16_update (uint16_t crc, uint8_t data) {
  crc = (crc >> 4) ^ crc_16_table[crc & 0xF] ^ crc_16_table[data & 0xF];
  crc = (crc >> 4) ^ crc_16_table[crc & 0xF] ^ crc_16_table[data >> 4];
  return crc;
}

static const uint8_t configRegs [] = {
  0x01, 0x04, // OpMode = standby
  0x02, 0x00, // DataModul = packet mode, fsk
  0x03, 0x02, // BitRateMsb, data rate = 49,261 khz
  0x04, 0x8A, // BitRateLsb, divider = 32 MHz / 650
  0x05, 0x05, // FdevMsb = 90 KHz
  0x06, 0xC3, // FdevLsb = 90 KHz
  0x07, 0xD9, // FrfMsb, freq = 868.000 MHz
  0x08, 0x00, // FrfMib, divider = 14221312
  0x09, 0x00, // FrfLsb, step = 61.03515625
  0x0B, 0x20, // AfcCtrl, afclowbetaon
  0x19, 0x42, // RxBw ...
  0x25, 0x40, // DioMapping1 ...
  0x29, 0xDC, // RssiThresh ...
  0x2E, 0x88, // SyncConfig = sync on, sync size = 2
  0x2F, 0x2D, // SyncValue1 = 0x2D
  0x30, 0x05, // SyncValue2 = 0x05
  0x37, 0x00, // PacketConfig1 = fixed, no crc, filt off
  0x38, 0x00, // PayloadLength = 0, unlimited
  0x3C, 0x8F, // FifoTresh, not empty, level 15
  0x3D, 0x10, // PacketConfig2, interpkt = 1, autorxrestart off
  0x6F, 0x20, // TestDagc ...
  0
};

uint8_t rf12_initialize(uint8_t id, uint8_t band, uint8_t g) {
  nodeid = id;
  group = g;
  initRadio(configRegs);
  writeReg(REG_SYNCVALUE2, g);
  long freq = band == RF12_868MHZ ? 860000000 :
              band == RF12_915MHZ ? 900000000 : 430000000;
  // Values above represent below the base of the respective bands for an RFM12B
  // Offset MUST be added to the above, the Offset (96 - 3903) has a multiplier 
  // dependant on the band, historically the default offset used is 1600
  // and the multipliers 0.25, 0.50 and 0.75 for bands 433, 868 & 915
  // frequency steps are in units of (32,000,000 >> 19) = 61.03515625 Hz
  // use multiples of 64 to avoid multi-precision arithmetic, i.e. 3906.25 Hz
  // due to this, the lower 6 bits of the calculated factor will always be 0
  // this is still 4 ppm, i.e. well below the radio's 32 MHz crystal accuracy
  // 868.0 MHz = 0xD90000, 868.3 MHz = 0xD91300, 915.0 MHz = 0xE4C000
  
  int offset =  1600;         // Replace with the eeprom value at some point.
  freq = freq + (offset * (band * 2500L));
  
  int frf = (((uint32_t) freq << 2) / (32000000 >> 11)) << 6;
  writeReg(REG_FRFMSB, frf >> 16);
  writeReg(REG_FRFMSB+1, frf >> 8);
  writeReg(REG_FRFMSB+2, frf);
  rxstate = TXIDLE;
  return nodeid;
}

uint8_t rf12_recvDone(void) {
  if (rxstate == TXIDLE) {
    rxfill = rf12_len = 0;
    rf12_crc = _crc16_update(~0, group);
    rxstate = TXRECV;
    flushFifo();
    setMode(RF_OPMODE_RECEIVER);
  } else {
    uint8_t irq2 = readReg(REG_IRQFLAGS2);
    if (rxstate == TXRECV) {
      if (irq2 & (RF_IRQFLAGS2_FIFONOTEMPTY | RF_IRQFLAGS2_FIFOOVERRUN)) {
        uint8_t in = readReg(REG_FIFO);
        if (rxfill == 0)
          rf12_buf[rxfill++] = group;
        rf12_buf[rxfill++] = in;
        rf12_crc = _crc16_update(rf12_crc, in);

        if (rxfill >= rf12_len + 5 || rxfill >= RF_MAX) {
          // printf("fill %d grp %d hdr %d len %d\n",
          //           rxfill, rf12_grp, rf12_hdr, rf12_len);
          rxstate = TXIDLE;
          setMode(RF_OPMODE_STANDBY);
          if (rf12_len > RF12_MAXDATA)
            rf12_crc = 1; // force bad crc if packet length is invalid
          if (!(rf12_hdr & RF12_HDR_DST) || nodeid == 31 ||
                  (rf12_hdr & RF12_HDR_MASK) == nodeid)
            return 1; // it's a broadcast packet or it's addressed to this node
        }
      }
    } else if ((irq2 & RF_IRQFLAGS2_FIFOFULL) == 0) {
      uint8_t out;
      if (rxstate < 0) {
        uint8_t pos = 3 + rf12_len + rxstate++;
        out = rf12_buf[pos];
        rf12_crc = _crc16_update(rf12_crc, out);
      } else if (rxstate == TXDONE) {
        if ((irq2 & RF_IRQFLAGS2_PACKETSENT)) {
          rxstate = TXIDLE;
          setMode(RF_OPMODE_STANDBY);
        }
        return 0; // keep transmitting until the packet has been sent
      } else
        switch (rxstate++) {
          case TXCRC1: out = rf12_crc; break;
          case TXCRC2: out = rf12_crc >> 8; break;
          default:     out = 0xAA;
        }
      writeReg(REG_FIFO, out);
    }
  }
  return 0;
}

uint8_t rf12_canSend(void) {
  if (rxstate == TXRECV && rxfill == 0) {
    rxstate = TXIDLE;
    setMode(RF_OPMODE_STANDBY);
    return 1;
  }
  return 0;
}

void rf12_sendStart(uint8_t hdr, const void* ptr, uint8_t len) {
  rf12_len = len;
  for (int i = 0; i < len; ++i)
    rf12_data[i] = ((const uint8_t*) ptr)[i];
  rf12_hdr = hdr & RF12_HDR_DST ? hdr : (hdr & ~RF12_HDR_MASK) + nodeid;  
  rf12_crc = _crc16_update(~0, group);
  rxstate = - (2 + rf12_len); // preamble and SYN1/SYN2 are sent by hardware
  flushFifo();
  setMode(RF_OPMODE_TRANSMITTER);
}

void rf12_sendNow(uint8_t hdr, const void* ptr, uint8_t len) {
  while (!rf12_canSend())
    rf12_recvDone(); // keep the driver state machine going, ignore incoming
  rf12_sendStart(hdr, ptr, len);
}

void rf12_sendWait(uint8_t mode) {
  while (rxstate < TXIDLE)
    rf12_recvDone();
}
