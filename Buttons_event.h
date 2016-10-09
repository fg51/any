#ifndef D_BUTTONS_EVENT_H
#define D_BUTTONS_EVENT_H

/*
 *
 * Buttons is responsible for ...
 *
 */

enum event_t {
    NO_EVENT                = 0b000000,
    EVT_BUTTON_SINGLE_CLICK = 0b000001,
    EVT_BUTTON_DOUBLE_CLICK = 0b000010,
    EVT_BUTTON_DOWN_LONG    = 0b000100,
    EVT_ENCODER_UP          = 0b010000,
    EVT_ENCODER_DOWN        = 0b100000,
};
typedef enum event_t event_t;

#endif  /* D_FakeButtons_H */
