#include <unity.h>
#include <algorithm>
#include <string>
#include "lcd_startup.h"

namespace {
// Model the stock splash's 50 ms / 3% countdown and a display that may still be
// booting. No power-on banner is supplied, as it may already have been drained.
struct StartupDisplay {
  unsigned now = 0, readyAt = 0, progress = 0, reads = 0;
  unsigned lostLabelWrites = 0, wrongReadbacks = 0;
  bool splash = true, countdown = true;
  std::string label;

  void advance(unsigned ms) {
    const unsigned previous = now;
    now += ms;
    if (splash && countdown && now >= readyAt) {
      progress += (now - std::max(previous, readyAt)) / 50 * 3;
      if (progress >= 99) splash = false;
    }
  }
  void begin(unsigned long) { advance(100); }
  void writeStr(const char *command) {
    if (now < readyAt) return;
    if (std::string(command) == "rest") {
      readyAt = now + 500;
      splash = countdown = true;
      progress = 0;
      label.clear();
    }
    if (std::string(command) == "page splash") {
      splash = countdown = true;
      progress = 0;
      label.clear();
    }
  }
  void writeStr(const char *, const char *value) {
    if (now < readyAt || !splash) return;
    if (lostLabelWrites) { --lostLabelWrites; return; }
    label = value;
  }
  void writeNum(const char *field, unsigned value) {
    if (now < readyAt || !splash) return;
    if (std::string(field) == "splash.tm0.en") countdown = value;
    else if (std::string(field) == "splash.loadBar.val") progress = value;
  }
  std::string readStr(const char *) {
    ++reads;
    if (now < readyAt) { advance(400); return "ERROR"; }
    // Waiting for readback must not consume the time available to see the ID.
    advance(400);
    if (!splash) return "ERROR";
    if (wrongReadbacks) { --wrongReadbacks; return "old"; }
    return label;
  }
};

void checkSplashVisible(StartupDisplay &display) {
  TEST_ASSERT_TRUE(display.splash);
  TEST_ASSERT_TRUE(display.countdown);
  TEST_ASSERT_EQUAL_STRING("bddc57a", display.label.c_str());
  display.advance(1600);
  TEST_ASSERT_TRUE(display.splash);
  display.advance(50);
  TEST_ASSERT_FALSE(display.splash);
}

void testLcdStartupWithDelayedColdBoot() {
  StartupDisplay display;
  display.readyAt = 5000;
  initializeLcdSplash(display, "bddc57a", [&](unsigned ms) { display.advance(ms); });
  TEST_ASSERT_GREATER_THAN(1, display.reads);
  checkSplashVisible(display);
}

void testLcdStartupAfterMcuOnlyReset() {
  StartupDisplay display;
  display.splash = false; // Screen is still on Home, with no boot greeting.
  display.label = "old";
  initializeLcdSplash(display, "bddc57a", [&](unsigned ms) { display.advance(ms); });
  checkSplashVisible(display);
}

void testLcdStartupRetriesLostLabelWrite() {
  StartupDisplay display;
  display.lostLabelWrites = 1;
  initializeLcdSplash(display, "bddc57a", [&](unsigned ms) { display.advance(ms); });
  TEST_ASSERT_EQUAL_UINT(2, display.reads);
  checkSplashVisible(display);
}

void testLcdStartupWaitsForMatchingReadback() {
  StartupDisplay display;
  display.wrongReadbacks = 2;
  initializeLcdSplash(display, "bddc57a", [&](unsigned ms) { display.advance(ms); });
  TEST_ASSERT_EQUAL_UINT(3, display.reads);
  checkSplashVisible(display);
}
}

void runAllLcdStartupTests() {
  RUN_TEST(testLcdStartupWithDelayedColdBoot);
  RUN_TEST(testLcdStartupAfterMcuOnlyReset);
  RUN_TEST(testLcdStartupRetriesLostLabelWrite);
  RUN_TEST(testLcdStartupWaitsForMatchingReadback);
}

#ifdef STANDALONE_LCD_STARTUP_TEST
void setUp() {}
void tearDown() {}
int main() {
  UNITY_BEGIN();
  runAllLcdStartupTests();
  return UNITY_END();
}
#endif
