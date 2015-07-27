#ifndef __MAIN_H__
#define __MAIN_H__

#define EVENT_PERIOD_S 8
#define SAMPLE_PERIOD_S 64
#define RESET_PERIOD_S 65536UL /* = 18.2 hours */

enum NVIC_priorities {
    PRIO_SYSTICK,
    PRIO_HIGH,
    PRIO_MEDIUM,
    PRIO_LOW
};

#endif
