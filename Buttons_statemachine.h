#ifndef D_Buttons_H
#define D_Buttons_H

/**********************************************************
 *
 * Buttons is responsible for ...
 *
 **********************************************************/

#include <stdint.h>

#include "Buttons_event.h"


void init_button_state(void);
event_t get_button_event(void);
void update_button_state(bool is_pushed, uint32_t cur_tick);

#endif  /* D_FakeButtons_H */
