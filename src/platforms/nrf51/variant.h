#ifndef VARIANT_H
#define VARIANT_H

#if defined(TARGET_MICRO_BIT)

  #include "variant_micro_bit.h"

#elif defined(TARGET_RBLAB_BLENANO)

  #include "variant_rbl_nano.h"

#else

#error "No variant defined"

#endif

#endif
