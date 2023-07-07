#ifndef VARIANT_H
#define VARIANT_H

#if defined(TARGET_TEENSY_3)

  #include <variant_teensy_3.h>

#elif defined(TARGET_TEENSY_4)

  #include <variant_teensy_4.h>

#else

  #error "No TARGET_* defined"

#endif

#endif
