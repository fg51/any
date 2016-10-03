#include "statemachine.h"

#include "shared_data.h"

button_event_t shared_event_button   = NO_EVENT;

typedef enum button_status_t {
    keeped_off,
    pushed,
    keeped_on,
    keeped_on_long,
    released,
} button_status_t;


const int debounce_time      =   10;
const int double_pushed_time =  500;
const int long_time          = 2000;


static button_status_t status = keeped_off;

static uint32_t last_button_down_tick = 0;
static bool is_single_pushed = false;


static inline
void act_keeped_off(const bool is_pushed, const uint32_t curTick);
static inline
void act_pushed(const bool is_pushed, const uint32_t curTick);
static inline
void act_keeped_on(const bool is_pushed, const uint32_t curTick);
static inline
void act_keeped_on_long(const bool is_pushed, const uint32_t curTick);
static inline
void act_released(const bool is_pushed, const uint32_t curTick);



void statemachine(const bool is_changed, const bool is_pushed)
{
    const int curTick = get_msTick();

    switch (status) {
    case keeped_off: { act_keeped_off(is_pushed, curTick); return; }
    case pushed:     { act_pushed(is_pushed, curTick); return;}
    case keeped_on:  { act_keeped_on(is_pushed, curTick); return;}
    case keeped_on_long: { act_keeped_on_long(is_pushed, curTick); return;}
    case released: {act_released(is_pushed, curTick); return;}
    default: { return; }
    }
}


/**
 * set status pushed
 * event: nothing
 */
static inline
void act_keeped_off(const bool is_pushed, const uint32_t curTick)
{
    if (is_pushed) {
        last_button_down_tick = curTick;
        status = pushed;
    }
    return;
}


/**
 * set status keeped_off / released keeped_on
 * event: nothing
 */
static inline
void act_pushed(const bool is_pushed, const uint32_t curTick)
{
    // debounce check
    if (is_pushed == false) {
        status = keeped_off;
        return;
    }
    if (curTick >= last_button_down_tick + debounce_time) {
        status = keeped_on;
        return;
    }
}

/**
 * set status released / keeped_on_long
 * event: EVT_BUTTON_DOUBLE_CLICK / EVT_BUTTON_DOWN_LONG
 */
static inline
void act_keeped_on(const bool is_pushed, const uint32_t curTick)
{
    if (is_pushed == false) { // released
        if (is_single_pushed) {
            is_single_pushed = false;
            shared_event_button = EVT_BUTTON_DOUBLE_CLICK;
            status = keeped_off;
            return;
        }
        status = released;
        return;
    }
    if (curTick >= last_button_down_tick + long_time) {
        shared_event_button  = EVT_BUTTON_DOWN_LONG;
        status = keeped_on_long;
    }
    return;
}

/**
 * set status keeped_off
 * event: nothing
 */
static inline
void act_keeped_on_long(const bool is_pushed, const uint32_t curTick)
{
    if (is_pushed == false) {
        status = keeped_off;
    }
    return;
}


/**
 * set status keeped_off / pushed
 * event: EVT_BUTTON_SINGLE_CLICK
 */
static inline
void act_released(const bool is_pushed, const uint32_t curTick)
{
    if (is_pushed) {
        is_single_pushed = true;
        status = pushed;
    }

    if (curTick >= double_pushed_time) {
        is_single_pushed = false;
        shared_event_button = EVT_BUTTON_SINGLE_CLICK;
        status = keeped_off;
    }
}

