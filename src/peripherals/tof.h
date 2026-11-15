#ifndef TOF_H
#define TOF_H

#include <stdint.h> // for uint8_t
#include <Adafruit_VL53L0X.h>
#include "../../lib/Common/sensors_state.h"
#include "../../lib/Common/water_level.h"

// Sensor-to-water distance at 100%; override for the installed tank in extra_defines.ini.
#ifndef WATER_TANK_FULL_DISTANCE_MM
#define WATER_TANK_FULL_DISTANCE_MM 15u
#endif
static_assert(WATER_TANK_FULL_DISTANCE_MM >= 0 && WATER_TANK_FULL_DISTANCE_MM < 125u,
  "Full tank distance must be nonnegative and below the 125 mm low-water boundary");

Adafruit_VL53L0X tof_sensor;

class TOF {
  public:
    TOF();
    void init(SensorState& sensor);
    uint16_t readLvl();
    uint16_t readRangeToPct(uint16_t val);

  private:
    // HardwareTimer* hw_timer;
    // static void TimerHandler10(void);
    uint32_t tofReading = 0;
    uint16_t latestReading = 0;
    WaterLevelAverage average;
};

TOF::TOF() {}

// void TOF::TimerHandler10() {
//   if (instance != nullptr && tof_sensor.isRangeComplete()) {
//     TOF::tofReading = tof_sensor.readRangeResult();
//   }
// }

void TOF::init(SensorState& sensor) {
  #ifdef TOF_VL53L0X
  while(!sensor.tofReady) {
    sensor.tofReady = tof_sensor.begin(0x29, false, &Wire, Adafruit_VL53L0X::VL53L0X_SENSE_HIGH_ACCURACY);
  }
  tof_sensor.startRangeContinuous();
  // Configure the hardware timer
  // hw_timer = new HardwareTimer(TIM10);
  // hw_timer->setCount(100000, MICROSEC_FORMAT);
  // hw_timer->setOverflow(100000, MICROSEC_FORMAT);
  // hw_timer->setInterruptPriority(1, 1);
  // hw_timer->attachInterrupt(TOF::TimerHandler10); // Attach the ISR function to the timer

  #endif
}

uint16_t TOF::readLvl() {
  #ifdef TOF_VL53L0X
  if(tof_sensor.isRangeComplete()) {
    latestReading = tof_sensor.readRangeResult();
    average.add(latestReading, millis());
  }
  TOF::tofReading = average.reading(millis());
  // A newly empty tank must stop operation without waiting for the average.
  if (latestReading > 125u) return 5u;
  #endif
  return  TOF::tofReading != 0 ? readRangeToPct(TOF::tofReading) : 30u;
}

uint16_t TOF::readRangeToPct(uint16_t val) {
  return waterLevelPercent<WATER_TANK_FULL_DISTANCE_MM>(val);
}

#endif
