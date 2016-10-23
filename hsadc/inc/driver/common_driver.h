#ifndef D_COMMON_DRIVER_H
#define D_COMMON_DRIVER_H

#if defined (__USE_LPCOPEN)
    #if defined(NO_BOARD_LIB)
        #include "chip.h"
    #else
        #include "board.h"
    #endif
#endif

#include <cr_section_macros.h>
#include <stdint.h>

#endif
