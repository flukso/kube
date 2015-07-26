struct pkt_ekmb_s {
    uint32_t cntr;
};

struct __attribute__((__packed__)) pkt_mma8452_s {
    uint32_t cntr;
    uint8_t padding;
};

struct pkt_gauge_s {
    uint32_t pressure;
    uint16_t temp;
    uint16_t humid;
    uint16_t light;
    uint8_t batt;
    uint8_t temp_err : 1;
    uint8_t humid_err : 1;
    uint8_t light_err: 1;
    uint8_t pressure_err: 1;
    uint8_t padding : 3;
    uint8_t wwdt_event : 1;
};
