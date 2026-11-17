// Exercise the production sampler and guard together, with only the ADC mocked.
#include <cassert>
#include <cmath>
#include <cstdio>
#include <HX711_2.h>
#include "src/peripherals/scales.h"
#include "boiler_fill.h"
#include "scales_filter.h"
#include "profiling_phases.h"
void remoteScalesTare() {}
bool remoteScalesIsPresent() { return false; }
Measurement remoteScalesGetWeight() { return {0,0}; }
void sample(uint32_t time,long a=250000,long b=50000) {
  testScale::now=time; testScale::raw[0]=a; testScale::raw[1]=b;
  testScale::newSample=true; scalesPoll();
}

void warmScales() {
  testScale::detected=true;
  testScale::raw[0]=250000;
  testScale::raw[1]=50000;
  testScale::now=100;
  scalesInit(1000,-1000);
  for(unsigned n=1;n<=10;++n) sample(100+n*100);
}

void testRejectedSamplesCannotRefreshWeightOrPermitSafetyActions() {
  warmScales();
  const auto before=scalesGetWeight();
  sample(1200,315536,50000);
  assert(scalesDiagnostics.rejectedSamples==1 && scalesDiagnostics.samples==0);
  assert(scalesDiagnostics.observedRaw[0]==315536 && scalesDiagnostics.observedMillis==1200);
  assert(!scalesTare()); // the caller must keep tare pending until an accepted frame arrives
  assert(scalesGetWeight().value==before.value && scalesGetWeight().millis==before.millis);
  assert(std::isnan(scalesGetSettledAbsoluteWeight().value));
  assert(scalesGetDripTrayStatus()!=DripTrayStatus::Present);
  for(unsigned n=0;n<3;++n) {
    sample(1300+n*100);
    assert(std::isnan(scalesGetSettledAbsoluteWeight().value));
  }
  sample(1600);
  assert(std::isfinite(scalesGetSettledAbsoluteWeight().value));

  for(unsigned n=0;n<4;++n) {
    sample(1700+n*100,-1,-1);
    assert(scalesDiagnostics.samples==0);
    assert(std::isnan(scalesGetSettledAbsoluteWeight().value));
    assert(scalesGetDripTrayStatus()!=DripTrayStatus::Present);
    assert(scalesDiagnostics.sampleMillis==1600);
  }
  assert(scalesDiagnostics.rejectedSamples==5);
  assert(!scalesIsPresent());
  assert(scalesGetWeight().millis==0); // never substitute a fresh timestamp for held data
  for(unsigned n=0;n<4;++n) sample(2100+n*100);
  assert(scalesIsPresent() && std::isfinite(scalesGetSettledAbsoluteWeight().value));
  std::puts("PASS: rejected frames hold the display timestamp, reset safety settling and fail closed when persistent");
}

void testPredictiveWeightStopThroughCutoffGlitches() {
  const float rates[] = {2.f,6.f,10.f};
  for(float rate : rates) for(bool corrupt : {false,true}) {
    warmScales();
    assert(scalesHasLocalHardware() && scalesTare());
    ShotStopPredictor predictor;
    predictor.reset(1100);
    predictor.add(scalesGetControlWeight(),1100);
    GlobalStopConditions stop;
    stop.weight=36.f;
    SensorState state = {};
    const unsigned idealCutMs=static_cast<unsigned>((36.f/rate-0.5f)*1000.f+0.5f);
    bool stopped=false;
    for(unsigned elapsed=10;elapsed<=20000;elapsed+=10) {
      const unsigned time=1100+elapsed;
      testScale::now=time;
      if(elapsed%100==0) {
        const bool bad=corrupt && elapsed>=idealCutMs-200 && elapsed<=idealCutMs;
        if(bad) sample(time,elapsed==idealCutMs-200 ? 315536 : -1,elapsed==idealCutMs-200 ? 50000 : -1);
        else sample(time,250000+static_cast<long>(rate*elapsed),50000);
      }
      state.scalesPresent=scalesIsPresent();
      if(state.scalesPresent) state.shotWeight=state.weight=scalesGetWeight().value;
      predictor.add(scalesGetControlWeight(),time);
      state.shotStop=predictor.predict(time,500);
      state.smoothedPumpFlow=1000.f; // must never influence local scale-based stopping
      if(stop.isReached(state,elapsed)) {
        const float finalMass=rate*(elapsed/1000.f+0.5f);
        assert(!state.shotStop.expired);
        assert(std::fabs(finalMass-36.f)<=rate*0.021f);
        if(corrupt) assert(scalesDiagnostics.samples==0);
        std::printf("PASS: %.0f g/s simulated pour%s stops at %.2f g including 0.5s tail\n",
          static_cast<double>(rate),corrupt ? " with cutoff glitches" : "",static_cast<double>(finalMass));
        stopped=true;
        break;
      }
    }
    assert(stopped);
  }
}

void testConfirmedLoadChangesTareAndLightSmoothing() {
  warmScales();
  scalesTare();
  assert(std::fabs(scalesGetWeight().value)<0.001);
  sample(1200,350000,50000);
  sample(1300,350000,50000);
  assert(std::fabs(scalesGetWeight().value)<0.001 && scalesGetWeight().millis==1100);
  // Repeated polling of the cached conversion cannot confirm a new load.
  for(unsigned i=0;i<20;++i) { testScale::now=1350; scalesPoll(); }
  assert(scalesDiagnostics.confirmedLoadChanges==0);
  sample(1400,350000,50000);
  assert(scalesDiagnostics.confirmedLoadChanges==1);
  assert(std::fabs(scalesGetWeight().value-100.f)<0.001);
  assert(scalesDiagnostics.samples==1 && std::isnan(scalesGetSettledAbsoluteWeight().value));
  for(unsigned n=0;n<3;++n) sample(1500+n*100,350000,50000);
  assert(std::fabs(scalesGetSettledAbsoluteWeight().value-300.f)<0.001);
  scalesTare();
  assert(std::fabs(scalesGetWeight().value)<0.001);
  assert(std::fabs(scalesGetSettledAbsoluteWeight().value-300.f)<0.001);
  for(unsigned n=0;n<3;++n) sample(1800+n*100);
  assert(scalesDiagnostics.confirmedLoadChanges==2);
  assert(std::fabs(scalesGetWeight().value+100.f)<0.001);
  for(unsigned n=0;n<3;++n) sample(2100+n*100);
  scalesTare();
  sample(2400,250600,50000);
  assert(std::fabs(scalesGetWeight().value-0.3f)<0.001);
  sample(2500,251200,50000);
  assert(std::fabs(scalesGetWeight().value-0.9f)<0.001);
  assert(scalesGetWeight().millis==2500 && scalesDiagnostics.confirmedLoadChanges==2);
  std::puts("PASS: cup placement/removal confirms in three real conversions; tare and the two-sample flow response remain correct");
}

void testCorruptionCannotCompleteBoilerFill() {
  warmScales();
  BoilerFill fill;
  BoilerFillState state=BoilerFillState::Waiting;
  for(uint32_t t=1200;t<=4200;t+=100) {
    sample(t);
    const auto weight=scalesGetSettledAbsoluteWeight();
    state=fill.update(weight.value,weight.millis,t);
  }
  assert(state==BoilerFillState::Filling);
  for(uint32_t t=4300;t<=9000;t+=100) {
    // Interleave large positive bit errors with the observed all-ones frames.
    sample(t,t%300 ? -1 : 315536,t%300 ? -1 : 50000);
    const auto weight=scalesGetSettledAbsoluteWeight();
    assert(std::isnan(weight.value));
    assert(fill.update(weight.value,weight.millis,t)==BoilerFillState::Waiting);
    assert(scalesGetDripTrayStatus()!=DripTrayStatus::Present);
  }
  for(uint32_t t=9100;t<=12300;t+=100) {
    sample(t);
    const auto weight=scalesGetSettledAbsoluteWeight();
    assert(fill.update(weight.value,weight.millis,t)==BoilerFillState::Waiting);
  }
  sample(12400);
  const auto weight=scalesGetSettledAbsoluteWeight();
  assert(fill.update(weight.value,weight.millis,12400)==BoilerFillState::Filling);
  std::puts("PASS: corrupt conversions cannot confirm water or restart filling before a fresh settled baseline");
}

void testCapturedScaleReplay(const char *path) {
  FILE *input=std::fopen(path,"r");
  assert(input);
  char header[128];
  assert(std::fgets(header,sizeof(header),input));
  unsigned time=0, mustReject=0, samples=0;
  long a=0,b=0;
  double rawMin=INFINITY,rawMax=-INFINITY,filteredMin=INFINITY,filteredMax=-INFINITY;
  while(std::fscanf(input,"%u,%ld,%ld,%u",&time,&a,&b,&mustReject)==4) {
    if(!samples) {
      testScale::now=time-100;
      testScale::raw[0]=a; testScale::raw[1]=b;
      scalesInit(1966,-1986);
    }
    sample(time,a,b);
    if(mustReject) {
      assert(scalesDiagnostics.filterStatus==static_cast<unsigned>(ScaleSampleResult::Outlier) ||
             scalesDiagnostics.filterStatus==static_cast<unsigned>(ScaleSampleResult::Invalid));
      assert(scalesDiagnostics.samples==0);
    }
    const double raw=(a+140084)/1966.0+(b-191846)/-1986.0;
    const double filtered=(scalesDiagnostics.displayedRaw[0]+140084)/1966.0
      +(scalesDiagnostics.displayedRaw[1]-191846)/-1986.0;
    rawMin=std::fmin(rawMin,raw); rawMax=std::fmax(rawMax,raw);
    filteredMin=std::fmin(filteredMin,filtered); filteredMax=std::fmax(filteredMax,filtered);
    ++samples;
  }
  std::fclose(input);
  assert(samples==290 && rawMax-rawMin>100 && filteredMax-filteredMin<2);
  assert(scalesDiagnostics.confirmedLoadChanges==0 && scalesDiagnostics.rejectedSamples>=4);
  std::printf("PASS: %u captured observations replayed; gross range %.2f g -> %.2f g; no false load changes\n",
    samples,rawMax-rawMin,filteredMax-filteredMin);
}

void testSettledAbsoluteWeightAndFill() {
  testScale::now=100; scalesInit(1000,-1000);
  assert(std::isnan(scalesGetSettledAbsoluteWeight().value));
  for (unsigned n=1;n<=3;n++) {
    sample(100+n*100);
    assert(std::isnan(scalesGetSettledAbsoluteWeight().value));
  }
  sample(500);
  const auto original=scalesGetSettledAbsoluteWeight();
  assert(std::fabs(original.value-200.f)<0.01 && original.millis==500);
  scalesTare();
  assert(std::fabs(scalesGetSettledAbsoluteWeight().value-original.value)<0.01);
  assert(std::fabs(scalesGetWeight().value)<0.01);

  BoilerFill fill;
  for (uint32_t t=500;t<=12000;t+=100) {
    sample(t);
    const auto w=scalesGetSettledAbsoluteWeight();
    const auto state=fill.update(w.value,w.millis,t);
    assert(state==(t<3500 ? BoilerFillState::Waiting : BoilerFillState::Filling));
  }
  // Simulated water starts late, after the old fill program would have stopped.
  BoilerFillState state=BoilerFillState::Filling;
  for (uint32_t t=12100;t<=14000;t+=100) {
    sample(t,250000+static_cast<long>(t-12000)*10,50000);
    if(t==12800) scalesTare(); // A UI tare during filling cannot affect completion.
    const auto w=scalesGetSettledAbsoluteWeight();
    state=fill.update(w.value,w.millis,t);
    if(t<13500) assert(state==BoilerFillState::Filling);
  }
  assert(state==BoilerFillState::Complete);
  testScale::now=14301;
  assert(std::isnan(scalesGetSettledAbsoluteWeight().value));
  for(unsigned n=0;n<4;n++) sample(15000+n*100);
  sample(15400,260000,50000);
  assert(std::isnan(scalesGetSettledAbsoluteWeight().value));
  sample(15500,0,50000);
  assert(std::isnan(scalesGetSettledAbsoluteWeight().value));
  std::puts("PASS: local settled absolute weight survives tare; delayed water completes fill; stale/noisy/invalid data rejected");
}

int main(int argc,char **argv) {
  testPredictiveWeightStopThroughCutoffGlitches();
  testRejectedSamplesCannotRefreshWeightOrPermitSafetyActions();
  testConfirmedLoadChangesTareAndLightSmoothing();
  testCorruptionCannotCompleteBoilerFill();
  assert(argc==2);
  testCapturedScaleReplay(argv[1]);
  testSettledAbsoluteWeightAndFill();
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
  // Three conversions confirm the changed load, then rebuild four safety samples.
  for(unsigned n=0;n<6;n++) sample(3600+n*100,100000,200000);
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
