// Exercise the production sampler and guard together, with only the ADC mocked.
#include <cassert>
#include <cmath>
#include <cstdio>
#include <HX711_2.h>
#include "src/peripherals/scales.h"
void remoteScalesTare() {}
bool remoteScalesIsPresent() { return false; }
Measurement remoteScalesGetWeight() { return {0,0}; }
void sample(uint32_t time,long a=250000,long b=50000) {
  testScale::now=time; testScale::raw[0]=a; testScale::raw[1]=b;
  testScale::newSample=true; scalesPoll();
}
int main() {
  testScale::now=100; scalesInit(1000,-1000);
#ifndef DRIP_TRAY_ZERO_RAW_1
  assert(scalesGetDripTrayStatus()==DripTrayStatus::Unconfigured);
  std::puts("PASS: missing tray calibration is not hidden as startup settling");
#else
  float gross=0;
  assert(scalesGetDripTrayStatus(&gross)==DripTrayStatus::Settling && std::isnan(gross));
  for(unsigned n=1;n<=3;n++) {
    sample(100+n*100);
    assert(scalesGetDripTrayStatus()==DripTrayStatus::Settling);
  }
  sample(500);
  assert(scalesGetDripTrayStatus(&gross)==DripTrayStatus::Present && std::fabs(gross-300)<0.01);
  std::puts("PASS: first three conversions wait; fourth settled conversion permits tray check immediately");
  testScale::now=3100;
  assert(scalesGetDripTrayStatus(&gross)==DripTrayStatus::Unavailable && std::isnan(gross));
  std::puts("PASS: stale readings after startup still fail closed");
  for(unsigned n=0;n<4;n++) sample(3200+n*100);
  assert(scalesGetDripTrayStatus()==DripTrayStatus::Present);
  std::puts("PASS: fresh settled readings recover without another reboot");
  for(unsigned n=0;n<4;n++) sample(3600+n*100,100000,200000);
  assert(scalesGetDripTrayStatus(&gross)==DripTrayStatus::Missing && std::fabs(gross)<0.01);
  std::puts("PASS: a settled missing tray cannot be mistaken for startup settling or presence");
  testScale::now=5000; scalesInit(1000,-1000);
  testScale::now=7999; assert(scalesGetDripTrayStatus()==DripTrayStatus::Settling);
  testScale::now=8000; assert(scalesGetDripTrayStatus()==DripTrayStatus::Unavailable);
  std::puts("PASS: no new conversions become a real error at the bounded deadline");
  testScale::detected=false; scalesInit(1000,-1000);
  assert(testScale::poweredDown && scalesGetDripTrayStatus()==DripTrayStatus::Unavailable);
  std::puts("PASS: a real ADC detection timeout is reported immediately");
#endif
}
