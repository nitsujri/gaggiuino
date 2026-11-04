#include <unity.h>
#include <string>
#include <vector>

#include "lcd_boiler_fill.h"

namespace {
// Model the stock HMI: no page-ID event on popup entry; local one-second
// timer; text cleared on unload. Local timers can only be set on that page.
struct FillDisplay {
  unsigned now = 0, openedAt = 0, opens = 0;
  bool timer = true;
  std::string page = "home", text;
  std::vector<std::string> commands;

  void writeStr(const char *field, const char *value) {
    commands.push_back(std::string(field) + "=" + value);
    TEST_ASSERT_EQUAL_STRING("popupMSG.t0.txt", field);
    text = value;
  }
  void writeStr(const char *command) {
    commands.push_back(command);
    if (std::string(command) == "page popupMSG") {
      page = "popupMSG";
      timer = true;
      openedAt = now;
      ++opens;
    } else if (std::string(command) == "page home") {
      page = "home";
      text.clear();
    } else TEST_FAIL_MESSAGE("Unexpected display command");
  }
  void writeNum(const char *field, unsigned value) {
    commands.push_back(std::string(field) + "=" + std::to_string(value));
    TEST_ASSERT_EQUAL_STRING("tm0.en", field);
    TEST_ASSERT_EQUAL_STRING("popupMSG", page.c_str());
    timer = value;
  }
  void advance(unsigned ms) {
    now += ms;
    if (page == "popupMSG" && timer && now - openedAt >= 1000) {
      page = "home";
      text.clear();
    }
  }
};

void testBoilerFillMessageLastsForTheWholePumpRun() {
  FillDisplay display;
  BoilerFillPopup popup;
  popup.update(display, false, true);
  TEST_ASSERT_TRUE(display.commands.empty());
  popup.update(display, true, true);
  for (unsigned n = 0; n < 200; ++n) {
    display.advance(100);
    popup.update(display, true, true);
    TEST_ASSERT_EQUAL_STRING("popupMSG", display.page.c_str());
    TEST_ASSERT_EQUAL_STRING("Filling boiler!", display.text.c_str());
  }
  // No periodic page re-entry or timer-dependent gaps while pumping.
  TEST_ASSERT_EQUAL_UINT(1, display.opens);
  TEST_ASSERT_EQUAL_UINT(3, display.commands.size());
  popup.update(display, false, true);
  TEST_ASSERT_EQUAL_STRING("home", display.page.c_str());
  TEST_ASSERT_TRUE(display.text.empty());
  TEST_ASSERT_TRUE(display.timer);
}

void testBoilerFillPauseClearsMessageAndRetryShowsItAgain() {
  FillDisplay display;
  BoilerFillPopup popup;
  popup.update(display, true, true);
  display.advance(100);
  popup.update(display, false, true); // E.g. vibration trips the tray interlock.
  TEST_ASSERT_EQUAL_STRING("home", display.page.c_str());
  display.advance(4000);
  popup.update(display, false, true);
  TEST_ASSERT_EQUAL_STRING("home", display.page.c_str());
  popup.update(display, true, true);
  display.advance(5000);
  TEST_ASSERT_EQUAL_STRING("Filling boiler!", display.text.c_str());
  popup.update(display, false, true);
  display.writeStr("popupMSG.t0.txt", "Boiler fill timed out!");
  display.writeStr("page popupMSG");
  display.advance(1000);
  TEST_ASSERT_EQUAL_STRING("home", display.page.c_str());
}

void testBoilerFillDoesNotRedirectNavigation() {
  FillDisplay display;
  BoilerFillPopup popup;
  popup.update(display, true, true);
  display.page = "settings";
  const auto writes = display.commands.size();
  popup.update(display, false, false);
  TEST_ASSERT_EQUAL_STRING("settings", display.page.c_str());
  TEST_ASSERT_EQUAL_UINT(writes, display.commands.size());
  popup.update(display, true, false);
  TEST_ASSERT_EQUAL_UINT(writes, display.commands.size());
}
}

void runAllLcdBoilerFillTests() {
  RUN_TEST(testBoilerFillMessageLastsForTheWholePumpRun);
  RUN_TEST(testBoilerFillPauseClearsMessageAndRetryShowsItAgain);
  RUN_TEST(testBoilerFillDoesNotRedirectNavigation);
}

#ifdef STANDALONE_LCD_BOILER_FILL_TEST
void setUp() {}
void tearDown() {}
int main() {
  UNITY_BEGIN();
  runAllLcdBoilerFillTests();
  return UNITY_END();
}
#endif
