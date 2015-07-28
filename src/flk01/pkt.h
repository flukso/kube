#ifndef __PKT_H__
#define __PKT_H__

struct pkt_ekmb_s {
    uint32_t cntr;
};

struct __attribute__((__packed__)) pkt_mma8452_s {
    uint32_t cntr;
    uint8_t padding;
};

struct __attribute__((__packed__)) pkt_gauge_s {
    uint32_t pressure: 20;      /* 20 bits */
    uint32_t humid: 12;         /* 12 bits */
    uint16_t temp;              /* 14 bits */
    uint16_t light;             /* 16 bits */
    uint8_t batt;               /*  5 bits */
    uint8_t temp_err : 1;
    uint8_t humid_err : 1;
    uint8_t light_err: 1;
    uint8_t pressure_err: 1;
    uint8_t accel_err: 1;
    uint8_t reserved : 1;
    uint8_t bod_event: 1;
    uint8_t wwdt_event: 1;
};

void pkt_tx_ekmb(void);
void pkt_tx_mma8452(void);
void pkt_tx_gauge(uint8_t wwdt_event);

#endif
