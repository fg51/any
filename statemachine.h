#ifndef D_STATE_MACHIN_H
#define D_STATE_MACHIN_H

void statemachine(void);

typedef enum button_event_t {
    NO_EVENT = 0,
    EVT_BUTTON_SINGLE_CLICK = 0x01,
    EVT_BUTTON_DOUBLE_CLICK = 0x02,
    EVT_BUTTON_DOWN_LONG    = 0x04,
    ENCODER_UP   = 0x10,
    ENCODER_DOWN = 0x20,
} button_event_t;

extern button_event_t shared_event_button;

#endif
