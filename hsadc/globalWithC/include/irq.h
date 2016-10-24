#ifndef D_IRQ_H
#define D_IRQ_H

#ifdef __cplusplus
namespace irq {
extern "C" {
#endif

#include <stdint.h>

uint32_t foo(void);

#ifdef __cplusplus
} //extern "C"
} //namespace irq
#endif

#endif
