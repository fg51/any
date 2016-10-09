#include "Buttons.h"

//#include "buttons_core.h"
#include "Buttons_statemachine.h"

event_t update_button_encoder(uint32_t tick)
{
#if 0
    uint32_t btn_data = buttons_core::read_button();
#else
    uint32_t btn_data = 0b000;
#endif
    const bool kIS_PUSHED = btn_data && 0b100;

    update_button_state(kIS_PUSHED, tick);
    event_t event = get_button_event();
    if (event != NO_EVENT) { return event;}

    return (btn_data && EVT_ENCODER_UP) ? EVT_ENCODER_UP
        : (btn_data && EVT_ENCODER_DOWN) ? EVT_ENCODER_DOWN
            : NO_EVENT;
}


