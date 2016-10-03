#ifndef D_SHARED_DATA_H
#define D_SHARED_DATA_H

#include <stdint.h>

extern uint32_t data;

int get_msTick(void);
void set_msTick(uint32_t tick);

void set_button_event(uint32_t event);
uint32_t get_button_event(void);

#endif
