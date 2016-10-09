#include "Buttons_statemachine.h"

#include "Buttons_event.h"



//{{{ prot-type enum
typedef enum button_state_t {
    state_idle,
    state_pushed,
    state_pushed_long,
    state_released,
    state_exit,
} button_state_t;
//}}} prot-type enum


//{{{ static globals
const uint32_t kBUTTON_DEBOUNCE_TICKS  =   10;
const uint32_t kBUTTON_RELEASED_TICKS  =  500;
const uint32_t kBUTTON_DOWN_LONG_TICKS = 2000;

static volatile event_t event = NO_EVENT;
static volatile button_state_t button_state = state_pushed;
static uint32_t last_tick = 0;
//}}} static globals


//{{{ prot-type functions
static void run_idle(const bool kIS_PUSHED, const uint32_t kCUR_TICK);
static void run_pushed(const bool kIS_PUSHED, const uint32_t kCUR_TICK);
static void run_pushed_long(const bool kIS_PUSHED, const uint32_t kCUR_TICK);
static void run_released(const bool kIS_PUSHED, const uint32_t kCUR_TICK);
static void run_exit(const bool kIS_PUSHED, const uint32_t kCUR_TICK);
//}}} prot-type functions


event_t get_button_event(void)
{
    return event;
}


void init_button_state(void)
{
    event = NO_EVENT;
    button_state = state_idle;
    last_tick = 0;
}


button_state_t get_button_state(void)
{
    return button_state;
}


void update_button_state(const bool kIS_PUSHED, const uint32_t kCUR_TICK)
{
    switch (button_state) {
    case state_idle:   {run_idle(kIS_PUSHED, kCUR_TICK);   return;}
    case state_pushed: {run_pushed(kIS_PUSHED, kCUR_TICK); return;}
    case state_pushed_long: {run_pushed_long(kIS_PUSHED, kCUR_TICK); return;}
    case state_released: {return;}
    case state_exit: {run_exit(kIS_PUSHED, kCUR_TICK); return;}
    default: {return;}
    }
}


static void run_idle(const bool kIS_PUSHED, const uint32_t kCUR_TICK)
{
    if (kIS_PUSHED) {
        button_state = state_pushed;
        last_tick = kCUR_TICK;
    }
    return;
}


static void run_pushed(const bool kIS_PUSHED, const uint32_t kCUR_TICK)
{
    static bool is_passed = false;

    const bool kIS_DEBOUNCED = kCUR_TICK < (kBUTTON_DEBOUNCE_TICKS + last_tick);
    if (kIS_DEBOUNCED) { return; }

    //NOTE: check long push
    if (kIS_PUSHED == true) {
        is_passed = true;
        const bool kIS_LONG_PUSHED = kCUR_TICK >= kBUTTON_DOWN_LONG_TICKS + last_tick;
        if (kIS_LONG_PUSHED) {
            button_state = state_pushed_long;
            event = EVT_BUTTON_DOWN_LONG;
            is_passed = false;
            return;
        }
    }
    button_state = (is_passed)? state_released: state_exit;
    is_passed = false;
    return;
}


static void run_pushed_long(const bool kIS_PUSHED, const uint32_t kCUR_TICK)
{
    if (kIS_PUSHED == false) {
        button_state = state_exit;
        return;
    }
    return;
}


static void run_released(const bool kIS_PUSHED, const uint32_t kCUR_TICK)
{
    static bool is_double_pushed = false;

    const bool kIS_RELEASED = kCUR_TICK >= kBUTTON_RELEASED_TICKS + last_tick;
    if (kIS_RELEASED == false) { return; }

    if (kIS_PUSHED == false) {
        if (is_double_pushed == false) {
            event = EVT_BUTTON_SINGLE_CLICK;
        }
        is_double_pushed == false;
        button_state = state_exit;
        return;
    }
    if (is_double_pushed == false) {
        is_double_pushed = true;
        event = EVT_BUTTON_DOUBLE_CLICK;
    }
    return;
}


static void run_double_pushed(const bool kIS_PUSHED, const uint32_t kCUR_TICK)
{
    if (kIS_PUSHED == false) {
        button_state = state_exit;
    }
    return;
}


static void run_exit(const bool kIS_PUSHED, const uint32_t kCUR_TICK)
{
    //event = NO_EVENT;
    button_state = state_idle;
    last_tick = 0;
    return;
}

