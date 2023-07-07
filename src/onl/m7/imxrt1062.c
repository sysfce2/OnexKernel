/* 
 * Copyright 2017 Paul Stoffregen
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <stdint.h>
#include "imxrt.h"

void pendablesrvreq_isr(void)
{
}

// Long ago you could install your own systick interrupt handler by just
// creating your own systick_isr() function.  No longer.  But if you
// *really* want to commandeer systick, you can still do so by writing
// your function into the RAM-based vector table.
//
//   _VectorsRam[15] = my_systick_function;
//
// However, for long-term portability, use a MillisTimer object to
// generate an event every millisecond, and attach your function to
// its EventResponder.  You can attach as a software interrupt, so your
// code will run at lower interrupt priority for better compatibility
// with libraries using mid-to-high priority interrupts.

// TODO: this doesn't work for IMXRT - no longer using predefined names

volatile uint32_t systick_millis_count;
volatile uint32_t systick_cycle_count;
uint32_t systick_safe_read; // micros() synchronization
void systick_isr(void)
{
	systick_cycle_count = ARM_DWT_CYCCNT;
	systick_millis_count++;
}

// Entry to any ARM exception clears the LDREX exclusive access flag.
// So we do not need to do anything with "systick_safe_read" here, as
// this code depends on this Cortex-M7 hardware feature to cause any
// STREX instruction to return 1 (fail status) after returning to
// main program or lower priority interrupts.
//  https://developer.arm.com/documentation/dui0646/c/the-cortex-m7-processor/memory-model/synchronization-primitives

void systick_isr_with_timer_events(void)
{
	systick_cycle_count = ARM_DWT_CYCCNT;
	systick_millis_count++;
}

