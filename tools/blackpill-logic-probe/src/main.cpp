#include <Arduino.h>
#include <cstdlib>
#include <cstring>
#include "stm32f4xx_ll_adc.h"

// Probe pins are only digital/analog inputs. No pull-ups, pull-downs, outputs,
// or target power connections. Analog input range is GND..VDDA, not 5 V.
namespace {
constexpr uint32_t kMask = 7; // PA0=CLK, PA1=D1, PA2=D2
constexpr size_t kCapacity = 8192;
struct Event { uint32_t tick; uint32_t state; };
constexpr size_t kAnalogCapacity = 32768;
union CaptureBuffer {
  Event events[kCapacity];
  uint16_t samples[kAnalogCapacity];
} buffer;
char command[64];
size_t commandLength = 0;
bool commandOverflow = false;
bool timerReady = false;

void identify() {
  Serial.print("BLACKPILL_LOGIC_PROBE 1 hz=");
  Serial.print(SystemCoreClock);
  Serial.print(" timer=");
  Serial.print(timerReady ? "ok" : "failed");
  Serial.print(" analog=1 adc_hz=");
  Serial.println(HAL_RCC_GetPCLK2Freq() / 2U);
}

void capture(uint32_t durationMs) {
  if (!timerReady) {
    Serial.println("ERROR cycle_counter_unavailable");
    return;
  }
  pinMode(PA0, INPUT);
  Event* events = buffer.events;
  const uint32_t limit = (SystemCoreClock / 1000U) * durationMs;
  size_t count = 1;
  bool truncated = false;
  uint32_t state = GPIOA->IDR & kMask;
  const uint32_t start = DWT->CYCCNT;
  uint32_t previous = start;
  uint32_t tick = start;
  uint32_t maxGap = 0;
  events[0] = {0, state};

  // No USB printing during acquisition. IRQs remain enabled for USB/SysTick;
  // maxGap reports the largest blind interval, including interrupt servicing.
  while (true) {
    const uint32_t nextState = GPIOA->IDR & kMask;
    tick = DWT->CYCCNT;
    const uint32_t gap = tick - previous;
    if (gap > maxGap) maxGap = gap;
    previous = tick;
    if (nextState != state) {
      events[count++] = {tick - start, nextState};
      state = nextState;
      if (count == kCapacity - 1) {
        truncated = true;
        break;
      }
    }
    if (tick - start >= limit) break;
  }
  const uint32_t elapsed = tick - start;
  // Last record is a time endpoint, not necessarily a transition.
  events[count++] = {elapsed, state};

  Serial.print("BEGIN {\"version\":1,\"hz\":");
  Serial.print(SystemCoreClock);
  Serial.print(",\"duration_ticks\":");
  Serial.print(elapsed);
  Serial.print(",\"max_poll_gap_ticks\":");
  Serial.print(maxGap);
  Serial.print(",\"truncated\":");
  Serial.print(truncated ? "true" : "false");
  Serial.print(",\"count\":");
  Serial.print(count);
  Serial.println("}");
  for (size_t i = 0; i < count; ++i) {
    Serial.print(events[i].tick);
    Serial.print(',');
    Serial.println(events[i].state);
  }
  Serial.println("END");
}

bool adcReadReference(uint16_t& raw) {
  ADC->CCR = ADC_CCR_TSVREFE; // ADC clock /2; internal reference enabled.
  ADC1->CR1 = 0; // 12-bit, single conversion, no interrupt.
  ADC1->SMPR1 = ADC_SMPR1_SMP17; // Reference requires a long acquisition.
  ADC1->SMPR2 = 0;
  ADC1->SQR1 = 0;
  ADC1->SQR2 = 0;
  ADC1->SQR3 = 17;
  ADC1->CR2 = ADC_CR2_ADON;
  delayMicroseconds(30);
  uint32_t sum = 0;
  for (unsigned i = 0; i < 16; ++i) {
    ADC1->SR = 0;
    ADC1->CR2 |= ADC_CR2_SWSTART;
    const uint32_t start = DWT->CYCCNT;
    while (!(ADC1->SR & ADC_SR_EOC)) {
      if (DWT->CYCCNT - start > SystemCoreClock / 100U) return false;
    }
    sum += ADC1->DR;
  }
  raw = static_cast<uint16_t>((sum + 8U) / 16U);
  ADC1->CR2 = 0;
  return true;
}

void analogCapture(unsigned preset, unsigned count) {
  if (!timerReady) { Serial.println("ERROR cycle_counter_unavailable"); return; }
  const uint32_t adcHz = HAL_RCC_GetPCLK2Freq() / 2U;
  if (adcHz > 36000000U) { Serial.println("ERROR adc_clock_too_fast"); return; }
  const unsigned bits = preset == 0 ? 6 : 12;
  const unsigned sampleCycles = preset == 2 ? 480 : 3;
  const unsigned conversionCycles = sampleCycles + bits;
  pinMode(PA0, INPUT_ANALOG);
  __HAL_RCC_DMA2_CLK_ENABLE();
  __HAL_RCC_ADC1_CLK_ENABLE();
  __HAL_RCC_ADC_FORCE_RESET();
  __HAL_RCC_ADC_RELEASE_RESET();
  uint16_t reference = 0;
  if (!adcReadReference(reference) || !reference) {
    ADC1->CR2 = 0; pinMode(PA0, INPUT);
    Serial.println("ERROR adc_reference_failed"); return;
  }
  const uint16_t factory = *VREFINT_CAL_ADDR;
  uint32_t supplyMv = (3300UL * factory + reference / 2U) / reference;
  const bool calibrated = factory > 100 && factory < 4096 && supplyMv >= 2400 && supplyMv <= 3600;
  if (!calibrated) supplyMv = 3300;
  ADC1->CR2 = 0;
  ADC->CCR = 0; // APB2 / 2, turn internal reference off during PA0 capture.
  ADC1->CR1 = bits == 6 ? ADC_CR1_RES : 0;
  ADC1->SMPR1 = 0;
  ADC1->SMPR2 = preset == 2 ? ADC_SMPR2_SMP0 : 0;
  ADC1->SQR1 = 0;
  ADC1->SQR2 = 0;
  ADC1->SQR3 = 0; // PA0 = ADC1_IN0, one channel, right aligned.
  DMA2_Stream0->CR = 0;
  const uint32_t dmaWait = DWT->CYCCNT;
  while (DMA2_Stream0->CR & DMA_SxCR_EN) {
    if (DWT->CYCCNT - dmaWait > SystemCoreClock / 100U) {
      pinMode(PA0, INPUT); Serial.println("ERROR dma_stop_timeout"); return;
    }
  }
  DMA2->LIFCR = DMA_LIFCR_CFEIF0 | DMA_LIFCR_CDMEIF0 | DMA_LIFCR_CTEIF0 |
                 DMA_LIFCR_CHTIF0 | DMA_LIFCR_CTCIF0;
  DMA2_Stream0->PAR = reinterpret_cast<uint32_t>(&ADC1->DR);
  DMA2_Stream0->M0AR = reinterpret_cast<uint32_t>(buffer.samples);
  DMA2_Stream0->NDTR = count;
  DMA2_Stream0->FCR = 0; // Direct mode, no FIFO; peripheral and memory 16 bit.
  DMA2_Stream0->CR = DMA_SxCR_PL | DMA_SxCR_MSIZE_0 | DMA_SxCR_PSIZE_0 | DMA_SxCR_MINC;
  ADC1->SR = 0;
  ADC1->CR2 = ADC_CR2_DMA | ADC_CR2_DDS | ADC_CR2_CONT | ADC_CR2_ADON;
  delayMicroseconds(5);
  DMA2_Stream0->CR |= DMA_SxCR_EN;
  const uint32_t start = DWT->CYCCNT;
  ADC1->CR2 |= ADC_CR2_SWSTART;
  const uint32_t deadline = static_cast<uint32_t>(
      (static_cast<uint64_t>(count) * conversionCycles * SystemCoreClock) / adcHz) + SystemCoreClock / 10U;
  bool failed = false;
  while (!(DMA2->LISR & DMA_LISR_TCIF0)) {
    if ((DMA2->LISR & (DMA_LISR_TEIF0 | DMA_LISR_DMEIF0 | DMA_LISR_FEIF0)) ||
        (ADC1->SR & ADC_SR_OVR) || DWT->CYCCNT - start > deadline) {
      // Completion can race the last conversion; an overrun AFTER the buffer
      // filled is harmless. An overrun before completion invalidates the frame.
      failed = !(DMA2->LISR & DMA_LISR_TCIF0);
      break;
    }
  }
  const uint32_t elapsed = DWT->CYCCNT - start;
  ADC1->CR2 = 0;
  DMA2_Stream0->CR &= ~DMA_SxCR_EN;
  while (DMA2_Stream0->CR & DMA_SxCR_EN) {}
  __DSB();
  const uint32_t remaining = DMA2_Stream0->NDTR;
  pinMode(PA0, INPUT);
  if (failed || remaining) { Serial.println("ERROR adc_dma_capture_failed"); return; }

  Serial.print("ANALOG {\"version\":1,\"kind\":\"analog\",\"channel\":\"PA0\",\"bits\":");
  Serial.print(bits);
  Serial.print(",\"count\":"); Serial.print(count);
  Serial.print(",\"adc_hz\":"); Serial.print(adcHz);
  Serial.print(",\"cycles_per_sample\":"); Serial.print(conversionCycles);
  Serial.print(",\"vdda_mv\":"); Serial.print(supplyMv);
  Serial.print(",\"vref_calibrated\":"); Serial.print(calibrated ? "true" : "false");
  Serial.print(",\"cpu_hz\":"); Serial.print(SystemCoreClock);
  Serial.print(",\"elapsed_ticks\":"); Serial.print(elapsed);
  Serial.println(",\"encoding\":\"u16le\"}");
  Serial.write(reinterpret_cast<const uint8_t*>(buffer.samples), count * sizeof(uint16_t));
  Serial.println();
  Serial.println("END");
}

void processCommand() {
  command[commandLength] = '\0';
  if (commandOverflow) {
    Serial.println("ERROR command_too_long");
  } else if (!strcmp(command, "info")) {
    identify();
  } else if (!strcmp(command, "status")) {
    Serial.print("STATE ");
    Serial.println(GPIOA->IDR & kMask);
  } else if (!strncmp(command, "analog ", 7)) {
    char* end = nullptr;
    const unsigned long preset = strtoul(command + 7, &end, 10);
    if (end == command + 7 || *end != ' ') {
      Serial.println("ERROR analog_requires_preset_and_count");
    } else {
      const char* countStart = end + 1;
      const unsigned long count = strtoul(countStart, &end, 10);
      if (end == countStart || *end != '\0' || preset > 2 || count < 256 || count > kAnalogCapacity) {
        Serial.println("ERROR analog_preset_0_to_2_count_256_to_32768");
      } else analogCapture(preset, count);
    }
  } else if (!strncmp(command, "capture ", 8)) {
    char* end = nullptr;
    unsigned long ms = strtoul(command + 8, &end, 10);
    if (end == command + 8 || *end != '\0' || ms < 10 || ms > 1000) {
      Serial.println("ERROR duration_must_be_10_to_1000_ms");
    } else {
      capture(static_cast<uint32_t>(ms));
    }
  } else {
    Serial.println("ERROR commands_are_info_status_capture_ms_analog_preset_count");
  }
  commandLength = 0;
  commandOverflow = false;
}
}

void setup() {
  pinMode(PA0, INPUT);
  pinMode(PA1, INPUT);
  pinMode(PA2, INPUT);
  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
  const uint32_t before = DWT->CYCCNT;
  for (volatile unsigned i = 0; i < 100; ++i) __NOP();
  timerReady = DWT->CYCCNT != before;
  Serial.begin(115200);
}

void loop() {
  while (Serial.available()) {
    const int ch = Serial.read();
    if (ch == '\r') continue;
    if (ch == '\n') {
      if (commandLength || commandOverflow) processCommand();
    } else if (commandLength < sizeof(command) - 1) {
      command[commandLength++] = static_cast<char>(ch);
    } else {
      commandOverflow = true;
    }
  }
}
