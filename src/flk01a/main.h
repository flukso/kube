#ifndef __MAIN_H__
#define __MAIN_H__


#define SAMPLE_PERIOD_S 8
#define RESET_PERIOD_S 65536UL

enum NVIC_priorities {
    PRIO_SYSTICK,
    PRIO_HIGH,
    PRIO_MEDIUM,
    PRIO_LOW
};

#endif

