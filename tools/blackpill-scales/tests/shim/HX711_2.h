#pragma once
#include <Arduino.h>
#include <cassert>
namespace testScale {
inline bool detected=true, newSample=false, poweredDown=false;
inline long raw[2]={250000,50000};
}
class HX711_2 {
  long offset[2]={};
public:
  explicit HX711_2(unsigned) {}
  void begin(unsigned,unsigned,unsigned,unsigned,unsigned) { offset[0]=offset[1]=0; }
  void set_scale(float,float) {}
  void power_up() { testScale::poweredDown=false; }
  void power_down() { testScale::poweredDown=true; }
  bool wait_ready_timeout(unsigned timeout,unsigned interval) {
    assert(timeout==1000 && interval==10); return testScale::detected;
  }
  void tare(unsigned samples) {
    assert(samples==4); offset[0]=testScale::raw[0]; offset[1]=testScale::raw[1];
  }
  bool try_read(long* out) {
    if(!testScale::newSample) return false;
    testScale::newSample=false;
    out[0]=testScale::raw[0]-offset[0]; out[1]=testScale::raw[1]-offset[1]; return true;
  }
  void get_offset(long* out) { out[0]=offset[0]; out[1]=offset[1]; }
  void set_offset(long a,long b) { offset[0]=a; offset[1]=b; }
};
