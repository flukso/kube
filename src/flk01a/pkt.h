struct pkt_counter_s {
    uint32_t cntr;
};

struct pkt_gauge_s {
    uint16_t temp;
    uint16_t humid;
    uint8_t batt;
    uint8_t temp_err : 1;
    uint8_t humid_err : 1;
    uint8_t padding : 6;
};
