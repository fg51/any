#include "statemachine.h"

#include <iostream>


typedef enum state_t {
    IDLE,
    DEBOUNCE_CHECK,
    SINGLE_PUSHED,
    DOUBLE_PUSHED,
    LONG_PUSHED,
} state_t;

const int event_single_pushed = 0;
const int event_long_pushed = 1;


static state_t status = IDLE;
static int event;

const int single_time = 0;
const int long_time = 0;


int get_msTick(void);

void update_event(int event_name);

void statemachine(const bool is_changed)
{
    const int curTick = get_msTick();

    switch (status) {
    case IDLE:
        if (is_changed) {
            status = DEBOUNCE_CHECK;
            return;
        }
        return;
    case DEBOUNCE_CHECK:
        if (is_changed) {
            status = IDLE;
            return;
        }
        if (curTick >= single_time) {
            status = SINGLE_PUSHED;
            return;
        }
        return;
    case SINGLE_PUSHED:
        if (is_changed) {
            status = IDLE;
            update_event(event_single_pushed);
            return;
        }
        if (curTick > long_time) {
            status = LONG_PUSHED;
        }
        return;
    case DOUBLE_PUSHED:
        return;
    case LONG_PUSHED:
        update_event(event_long_pushed);
            status = IDLE;
            return;
    default:
        return;
    }
}


int get_msTick(void)
{
    return 0;
}

void update_event(int event_name)
{
    event = event_name;
}

