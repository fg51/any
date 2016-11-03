#include "irq.h"

#include "shared.h"

uint32_t foo(void)
{
    return 100 + g_data;
}
