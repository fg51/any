#ifndef D_Buttons_H
#define D_Buttons_H

/**********************************************************
 *
 * Buttons is responsible for ...
 *
 **********************************************************/

#include <stdint.h>

enum button_state_t {
    state_idle,
    state_pushed,
    state_pushed_long,
    state_released,
};


void init_button_state(void);
enum button_state_t get_button_state(void);

void update_button_state(bool is_pushed, uint32_t cur_tick);

#endif  /* D_FakeButtons_H */
