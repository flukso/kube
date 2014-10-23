// Driver for the RF69 radio chip, used in RF12 compatibility mode.
// 2013-11-06 <jc@wippler.nl> http://opensource.org/licenses/mit-license.php

#include <stdint.h>

/// Shorthand for RFM12B group byte in rf12_buf.
#define rf12_grp        rf12_buf[0]
/// Shorthand for RFM12B header byte in rf12_buf.
#define rf12_hdr        rf12_buf[1]
/// Shorthand for RFM12B length byte in rf12_buf.
#define rf12_len        rf12_buf[2]
/// Shorthand for first RFM12B data byte in rf12_buf.
#define rf12_data       (rf12_buf + 3)

/// RFM12B CTL bit mask.
#define RF12_HDR_CTL    0x80
/// RFM12B DST bit mask.
#define RF12_HDR_DST    0x40
/// RFM12B ACK bit mask.
#define RF12_HDR_ACK    0x20
/// RFM12B HDR bit mask.
#define RF12_HDR_MASK   0x1F

/// RFM12B Maximum message size in bytes.
#define RF12_MAXDATA    66

#define RF12_433MHZ     1   ///< RFM12B 433 MHz frequency band.
#define RF12_868MHZ     2   ///< RFM12B 868 MHz frequency band.
#define RF12_915MHZ     3   ///< RFM12B 915 MHz frequency band.

/// Shorthand to simplify detecting a request for an ACK.
#define RF12_WANTS_ACK ((rf12_hdr & RF12_HDR_ACK) && !(rf12_hdr & RF12_HDR_CTL))
/// Shorthand to simplify sending out the proper ACK reply.
#define RF12_ACK_REPLY (rf12_hdr & RF12_HDR_DST ? RF12_HDR_CTL : \
            RF12_HDR_CTL | RF12_HDR_DST | (rf12_hdr & RF12_HDR_MASK))
            
/// Running crc value, should be zero at end.
extern volatile uint16_t rf12_crc;
/// Recv/xmit buf including hdr & crc bytes.
extern volatile uint8_t rf12_buf[];

/// Call this once with the node ID, frequency band, and optional group.
uint8_t rf12_initialize(uint8_t id, uint8_t band, uint8_t group);

/// Call this frequently, returns true if a packet has been received.
uint8_t rf12_recvDone(void);

/// Call this to check whether a new transmission can be started.
/// @return true when a new transmission may be started with rf12_sendStart().
uint8_t rf12_canSend(void);

/// Call this only when rf12_recvDone() or rf12_canSend() return true.
void rf12_sendStart(uint8_t hdr, const void* ptr, uint8_t len);
/// This variant loops on rf12_canSend() and then calls rf12_sendStart() asap.
void rf12_sendNow(uint8_t hdr, const void* ptr, uint8_t len);

/// Wait for send to finish.
/// @param mode sleep mode 0=none, 1=idle, 2=standby, 3=powerdown.
void rf12_sendWait(uint8_t mode);
