#pragma once

#include <Arduino.h>

#if defined(ARDUINO_ARCH_STM32)
#include <HardwareTimer.h>
#include "pinmap.h"

#ifndef HX711_CLOCK_HALF_PERIOD_US
#define HX711_CLOCK_HALF_PERIOD_US 2
#endif

// Leave headroom below the HX711's 50 us maximum clock-high time for IRQ latency.
static_assert(HX711_CLOCK_HALF_PERIOD_US >= 1 && HX711_CLOCK_HALF_PERIOD_US <= 20,
              "HX711 clock half-period must be between 1 and 20 us");

// Call after HX711_2::begin(), while its timer is still stopped, before power_up().
inline void configureScalesClock(uint32_t clockPin, TIM_TypeDef* timerInstance) {
#if defined(HX711_CLOCK_PULLUP) && !defined(SINGLE_HX711_BOARD)
  // Arduino's OUTPUT_OPEN_DRAIN selects GPIO_NOPULL. Supply the rising edge
  // internally without changing the shared-clock/two-data-channel interface.
  pin_function(digitalPinToPinName(clockPin),
               STM_PIN_DATA(STM_MODE_OUTPUT_OD, GPIO_PULLUP, 0));
#endif

#if HX711_CLOCK_HALF_PERIOD_US != 2
  // HX711_2 owns the HardwareTimer. Reuse the instance registered by the pinned
  // STM32 core: constructing another HardwareTimer here would replace its ISR.
  auto* registeredTimer = HardwareTimer_Handle[get_timer_index(timerInstance)];
  if (registeredTimer == nullptr || registeredTimer->__this == nullptr) {
    Error_Handler();
    return;
  }
  auto* timer = static_cast<HardwareTimer*>(registeredTimer->__this);
  timer->setOverflow(HX711_CLOCK_HALF_PERIOD_US, MICROSEC_FORMAT);
#endif
}
#endif
