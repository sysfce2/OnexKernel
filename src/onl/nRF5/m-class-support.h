#include <stdint.h>
#include <stdbool.h>

static inline bool in_interrupt_context() {
  uint32_t ipsr;
  __asm volatile ("MRS %0, IPSR" : "=r" (ipsr) );
  return ipsr != 0;
}

extern void HardFault_Handler();

