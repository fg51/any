#include "statemachine.h"

void update_btn(void);
void ui_update(void);
void update_lcd(void);


int main(void) {

    char buf[16] = "no msg";

    while (true) {
        update_btn();
        ui_update();
        update_lcd();
    }

    return 0;
}

#include <stdint.h>

static constexpr uint32_t to_MHz(uint32_t hz);

static constexpr uint32_t to_MHz(uint32_t hz)
{
    return hz / 1000000;
}


