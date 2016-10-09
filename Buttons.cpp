#include "Buttons.h"


#define NO_EVENT                    (0b000000)
#define EVT_BUTTON_SINGLE_CLICK     (0b000001)
#define EVT_BUTTON_DOUBLE_CLICK     (0b000010)
#define EVT_BUTTON_DOWN_LONG        (0b000100)
#define ENCODER_UP                  (0b010000)
#define ENCODER_DOWN                (0b100000)

static volatile uint32_t event = NO_EVENT;

//{{{ prot-type enum
typedef enum button_state_t button_state_t;
//}}} prot-type enum


//{{{ static globals
const uint32_t kBUTTON_DEBOUNCE_TICKS  =   10;
const uint32_t kBUTTON_RELEASED_TICKS  =  500;
const uint32_t kBUTTON_DOWN_LONG_TICKS = 2000;

static volatile button_state_t button_state = state_pushed;
static uint32_t last_tick = 0;
//}}} static globals


//{{{ prot-type functions
static void run_idle(const bool is_pushed, const uint32_t cur_tick);
static void run_pushed(const bool is_pushed, const uint32_t cur_tick);
static void run_pushed_long(const bool kIS_PUSHED, const uint32_t kCUR_TICK);
static void run_released(const bool kIS_PUSHED, const uint32_t kCUR_TICK);
static void run_exit(const bool kIS_PUSHED, const uint32_t kCUR_TICK);
//}}} prot-type functions


void init_button_state(void)
{
    event = NO_EVENT;
    button_state = state_idle;
    last_tick = 0;
}


enum button_state_t get_button_state(void)
{
    return button_state;
}


void update_button_state(const bool is_pushed, const uint32_t cur_tick)
{
    switch (button_state) {
    case state_idle:   {run_idle(is_pushed, cur_tick);   return;}
    case state_pushed: {run_pushed(is_pushed, cur_tick); return;}
    case state_pushed_long: {run_pushed_long(is_pushed, cur_tick); return;}
    case state_released: {return;}
    case state_exit: {run_exit(is_pushed, cur_tick); return;}
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

    const bool is_debounced = kCUR_TICK < (kBUTTON_DEBOUNCE_TICKS + last_tick);
    if (is_debounced) { return; }

    //NOTE: check long push
    if (kIS_PUSHED == true) {
        is_passed = true;
        const bool is_long_pushed = kCUR_TICK >= kBUTTON_DOWN_LONG_TICKS + last_tick;
        if (is_long_pushed) {
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

/**
 * pushed -> released
 * pushed -> released -> pushed
 */
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
}

