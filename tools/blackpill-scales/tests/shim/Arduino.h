#pragma once
#include <cstddef>
#include <cstdint>
constexpr unsigned PB0=0, PB8=8, PB9=9, TIM3=3;
constexpr unsigned OUTPUT=1, OUTPUT_OPEN_DRAIN=2;
namespace testScale { inline uint32_t now=0; }
inline uint32_t millis() { return testScale::now; }
inline void configureScalesClock(uint32_t, unsigned) {}
