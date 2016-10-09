#include "Buttons.h"


#define NO_EVENT                    (0b000000)
#define EVT_BUTTON_SINGLE_CLICK     (0b000001)
#define EVT_BUTTON_DOUBLE_CLICK     (0b000010)
#define EVT_BUTTON_DOWN_LONG        (0b000100)
#define ENCODER_UP                  (0b010000)
#define ENCODER_DOWN                (0b100000)

static uint32_t event = NO_EVENT;

//{{{ prot-type enum
typedef enum button_state_t button_state_t;
//}}} prot-type enum


//{{{ static globals
const uint32_t kBUTTON_DEBOUNCE_TICKS  =   10;
const uint32_t kBUTTON_DOUBLE_TICKS    =  500;
const uint32_t kBUTTON_DOWN_LONG_TICKS = 2000;

static button_state_t button_state = state_pushed;
//}}} static globals


//{{{ prot-type functions
static void run_idle(const bool is_pushed, const uint32_t cur_tick);
static void run_pushed(const bool is_pushed, const uint32_t cur_tick);
static void run_pushed_long(const bool kIS_PUSHED, const uint32_t kCUR_TICK);
static void run_released(const bool kIS_PUSHED, const uint32_t kCUR_TICK);
//}}} prot-type functions


void init_button_state(void)
{
    event = NO_EVENT;
    button_state = state_idle;
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
    default: {return;}
    }
}


static void run_idle(const bool kIS_PUSHED, const uint32_t kCUR_TICK)
{
    button_state = (kIS_PUSHED) ? state_pushed: state_idle;
    return;
}

static bool is_double_pushed = false;
static bool is_single_pushed = false;

static void run_pushed(const bool kIS_PUSHED, const uint32_t kCUR_TICK)
{
    //NOTE: check long push
    if (kIS_PUSHED == true) {
        if (kCUR_TICK >= kBUTTON_DOWN_LONG_TICKS) {
            event = EVT_BUTTON_DOWN_LONG;
            button_state = state_pushed_long;
            return;
        }
        return;
    }

    //NOTE: check debounce
    if (kCUR_TICK >= kBUTTON_DEBOUNCE_TICKS) {
        if (is_single_pushed) {
            is_double_pushed = true;
        } else {
            is_single_pushed = true;
        }
    }
    button_state = state_released;

    return;
}


static void run_pushed_long(const bool kIS_PUSHED, const uint32_t kCUR_TICK)
{
    if (kIS_PUSHED == false) {
        button_state = state_released;
        return;
    }

    return;
}


static void run_released(const bool kIS_PUSHED, const uint32_t kCUR_TICK)
{
    if (kIS_PUSHED) {
        if (kCUR_TICK >= kBUTTON_DOUBLE_TICKS) {
        }
    }

    if (is_double_pushed) {
        is_double_pushed = false;
        event = EVT_BUTTON_DOUBLE_CLICK;
        button_state = state_idle;
        return;
    }
    if (is_single_pushed) {
        is_single_pushed = false;
        event = EVT_BUTTON_SINGLE_CLICK;
        button_state = state_idle;
        return;
    }
    event = NO_EVENT;
    button_state = state_idle;
    return;
}

