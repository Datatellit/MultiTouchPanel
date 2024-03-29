#ifndef BUTTON_H_
#define BUTTON_H_

#define BTN_NUM 6

#include "_global.h"
// Key list
typedef enum
{
    keylst1,
    keylst2,
    keylst3,
    keylst4,
    keylst5,
    keylst6,
    keylstDummy
} keylist_t;

typedef enum button_timer_status_e
{
    BUTTON_STATUS_INIT = 0,
    BUTTON_STATUS_LESS_2S,
    BUTTON_STATUS_MORE_2S,
    BUTTON_STATUS_MORE_5S,
    BUTTON_STATUS_DOUBLE_TRACK
} button_timer_status_t;

typedef enum button_event_e
{
    BUTTON_INVALID = 0,
    BUTTON_SHORT_PRESS,
    BUTTON_DOUBLE_PRESS,
    BUTTON_LONG_HOLD,
    BUTTON_LONG_PRESS,
    BUTTON_VERY_LONG_HOLD,
    BUTTON_VERY_LONG_PRESS,
    DOUBLE_BTN_TRACK
} button_event_t;

#define IS_VALID_BUTTON(x)              ((x) >= keylst1 && (x) < keylstDummy)

void button_event_handler(uint8_t _pin);
void button_init(void);
void button_bus_init();

// button map table
extern uint8_t btn_map[BTN_NUM];
// button action table
extern uint8_t btn_action[BTN_NUM];
extern uint8_t btn_scenariomap[BTN_NUM];

#endif // BUTTON_H_
