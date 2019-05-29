#ifndef VARIANT_H
#define VARIANT_H

#if defined(TARGET_MICRO_BIT)

  #include "variant_micro_bit.h"

#elif defined(TARGET_NRF51_DK)

  #include "variant_nrf51_dk.h"

#elif defined(TARGET_NRF51_USB)

  #include "variant_nrf51_usb.h"

#elif defined(TARGET_NRF52_USB)

  #include "variant_nrf52_usb.h"

#elif defined(TARGET_NRF52_DK)

  #include "variant_nrf52_dk.h"

#elif defined(TARGET_RBL_UNO)

  #include "variant_rbl_uno.h"

#elif defined(TARGET_RBL_NANO)

  #include "variant_rbl_nano.h"

#elif defined(TARGET_RFDUINO)

  #include "variant_rfduino.h"

#elif defined(TARGET_WAVESHARE)

  #include "variant_waveshare.h"

#elif defined(TARGET_BLE_USB_FRIEND)

  #include "variant_ble_usb_friend.h"

#elif defined(TARGET_BLUEFRUIT_FLORA)

  #include "variant_bluefruit_flora.h"

#elif defined(TARGET_ARCH_BLE)

  #include "variant_arch_ble.h"

#elif defined(TARGET_LINUX)

  #include "time.h"
  #include "variant_linux.h"

#else

#error "No variant defined"

#endif

#endif
