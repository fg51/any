//extern "C"
//{
#include "Buttons.h"
typedef enum button_state_t button_state_t;
//}

#include "CppUTest/TestHarness.h"

TEST_GROUP(Buttons)
{
    void setup()
    {
        init_button_state();
    }

    void teardown()
    {
    }
};

void set_to_state_pushed(void);
void set_to_state_pushed_long(void);


TEST(Buttons, Create)
{
    CHECK_EQUAL(state_idle, get_button_state());
}


TEST(Buttons, idle_to_pushed)
{
    update_button_state(true, 0);
    CHECK_EQUAL(state_pushed, get_button_state());
}

TEST(Buttons, pushed_to_idle)
{
    set_to_state_pushed();

    update_button_state(false, 0);
    CHECK_EQUAL(state_released, get_button_state());
}

TEST(Buttons, pushed_to_pushed_long)
{
    set_to_state_pushed();

    update_button_state(true, 10);
    CHECK_EQUAL(state_pushed_long, get_button_state());
}

TEST(Buttons, pushed_long_to_released)
{
    set_to_state_pushed_long();

    update_button_state(true, 10);
    CHECK_EQUAL(state_pushed_long, get_button_state());
}



void set_to_state_pushed(void)
{
    update_button_state(true, 0);
}

void set_to_state_pushed_long(void)
{
    update_button_state(true, 0);
    update_button_state(true, 10);
}

