#ifndef __PKT_H__
#define __PKT_H__

struct pkt_ekmb_s {
    uint32_t cntr;
};

struct pkt_mma8452_s {
    uint16_t cntr;
};

/* [4| humidity:u12 pressure:u20] light:u2 [3| error:u7 battery:u3 temperature:u14] */
struct __attribute__((__packed__)) pkt_gauge_s {
    uint32_t pressure: 20;
    uint32_t humid: 12;
    uint16_t light;
    uint16_t temp: 14;
    uint16_t batt_lo: 2;
    uint8_t batt_hi: 1;
    uint8_t temp_err : 1;
    uint8_t humid_err : 1;
    uint8_t light_err: 1;
    uint8_t pressure_err: 1;
    uint8_t accel_err: 1;
    uint8_t bod_event: 1;
    uint8_t wwdt_event: 1;
};

void pkt_tx_ekmb(void);
void pkt_tx_mma8452(void);
void pkt_tx_gauge(uint8_t wwdt_event);

#endif
