#include "../../src/eeprom_data/default_profiles.h"
#include "../../src/eeprom_data/legacy/eeprom_data_v12.h"
#include "../fixtures/config_v12.h"
#include "indigo/ui.h"
#include <algorithm>
#include <fstream>
#include <limits>
#include <set>
#include <unity.h>
namespace IndigoTest {
using namespace Indigo;
static eepromValues_t config() {
  eepromValues_t c{};
  c.steamSetPoint = 155;
  c.offsetTemp = 7;
  c.hpwr = 550;
  c.mainDivider = 5;
  c.brewDivider = 3;
  c.powerLineFrequency = 50;
  c.scalesF1 = 3920;
  c.scalesF2 = 4210;
  c.pumpFlowAtZero = .2225f;
  c.lcdSleep = 16;
  c.brightness = 90;
  c.tankFullDistanceMm = 30;
  c.trayMinWeightG = 100;
  for (unsigned i = 0; i < 5; i++) {
    c.profiles[i] = defaultsProfile[i];
    c.profiles[i].shotRatioX100 = c.profiles[i].shotPreset * 100;
  }
  return c;
}
static std::vector<std::pair<Command, int>> calls;
static bool cb(Command c, int i) {
  calls.push_back({c, i});
  return true;
}
struct World {
  eepromValues_t cfg = config();
  SensorState sensor{};
  Ui ui{cfg, sensor, cb};
  Telemetry live{};
  uint32_t now = 100;
  World() {
    calls.clear();
    sensor.temperature = 74.6;
    sensor.waterLvl = 80;
    live.waterValid = true;
    live.startup = true;
    ui.begin();
    draw();
  }
  std::vector<std::string> draw() {
    ui.tick(now, live);
    now += 10;
    std::vector<std::string> out;
    while (const auto *c = ui.front()) {
      TEST_ASSERT_TRUE(c->size() < 192);
      out.push_back(*c);
      ui.pop();
    }
    for (const auto &h : ui.targets()) {
      TEST_ASSERT_TRUE(h.x >= 0 && h.y >= 0 && h.x + h.w <= 320 &&
                       h.y + h.h <= 240);
    }
    return out;
  }
  void tap(const std::string &label) {
    auto targets = ui.targets();
    auto i = std::find_if(targets.begin(), targets.end(),
                          [&](const Hit &h) { return h.label == label; });
    TEST_ASSERT_TRUE_MESSAGE(i != targets.end(), label.c_str());
    if (i == targets.end())
      return;
    ui.touch(i->x + i->w / 2, i->y + i->h / 2, true);
    ui.touch(i->x + i->w / 2, i->y + i->h / 2, false);
    draw();
  }
};
void checksum_and_validation() {
  TEST_ASSERT_EQUAL_HEX32(0xcbf43926, settingsChecksum("123456789", 9));
  auto c = config();
  TEST_ASSERT_TRUE(settingsValid(c));
  for (auto v : {0, -20001, 20001}) {
    auto invalid = c;
    invalid.scalesF1 = v;
    TEST_ASSERT_FALSE(settingsValid(invalid));
    invalid = c;
    invalid.scalesF2 = v;
    TEST_ASSERT_FALSE(settingsValid(invalid));
  }
  for (auto v : {std::numeric_limits<float>::quiet_NaN(),
                 std::numeric_limits<float>::infinity(), -1.f}) {
    auto bad = c;
    bad.profiles[0].mfProfileStart = v;
    TEST_ASSERT_FALSE(settingsValid(bad));
  }
  auto bad = c;
  bad.activeProfile = 5;
  TEST_ASSERT_FALSE(settingsValid(bad));
  bad = c;
  bad.brewDivider = 0;
  TEST_ASSERT_FALSE(settingsValid(bad));
}
void migrate_real_schema12() {
  eepromValues_t_v12 old;
  std::memcpy(&old, capturedConfigV12, sizeof(old));
  auto upgraded = config();
  TEST_ASSERT_TRUE(upgradeSchema_v12(upgraded, old));
  TEST_ASSERT_EQUAL(7, upgraded.offsetTemp);
  TEST_ASSERT_EQUAL(3920, upgraded.scalesF1);
  TEST_ASSERT_EQUAL(4210, upgraded.scalesF2);
  TEST_ASSERT_EQUAL_FLOAT(.2225f, upgraded.pumpFlowAtZero);
  TEST_ASSERT_EQUAL(30, upgraded.tankFullDistanceMm);
  for (unsigned i = 0; i < 5; i++) {
    TEST_ASSERT_EQUAL_STRING(old.profiles[i].name, upgraded.profiles[i].name);
    for (unsigned j = 0; j < profileFieldCount; j++) {
      const auto &f = profileFields[j];
      if (std::strcmp(f.key, "shotRatioX100") == 0)
        continue;
      TEST_ASSERT_TRUE(readField(&old.profiles[i], f) ==
                       readField(&upgraded.profiles[i], f));
    }
    TEST_ASSERT_EQUAL(old.profiles[i].shotPreset * 100,
                      upgraded.profiles[i].shotRatioX100);
  }
  TEST_ASSERT_TRUE(settingsValid(upgraded));
}
void save_scopes_are_independent() {
  auto saved = config(), draft = saved;
  draft.activeProfile = 2;
  draft.offsetTemp = 11;
  draft.profiles[0].setpoint = 88;
  draft.profiles[1].setpoint = 89;
  const auto p = profileSaveValues(saved, draft, 1);
  TEST_ASSERT_EQUAL(saved.activeProfile, p.activeProfile);
  TEST_ASSERT_EQUAL(saved.offsetTemp, p.offsetTemp);
  TEST_ASSERT_EQUAL(saved.profiles[0].setpoint, p.profiles[0].setpoint);
  TEST_ASSERT_EQUAL(89, p.profiles[1].setpoint);
  const auto s = systemSaveValues(saved, draft);
  TEST_ASSERT_EQUAL(saved.activeProfile, s.activeProfile);
  TEST_ASSERT_EQUAL(11, s.offsetTemp);
  for (unsigned i = 0; i < 5; i++)
    TEST_ASSERT_EQUAL_MEMORY(&saved.profiles[i], &s.profiles[i],
                             sizeof(saved.profiles[i]));
}
void tank_full_does_not_move_low_limit() {
  for (unsigned full = 1; full < 125; full++) {
    TEST_ASSERT_EQUAL(100, tankPercent(full, full));
    TEST_ASSERT_EQUAL(10, tankPercent(125, full));
    TEST_ASSERT_EQUAL(9, tankPercent(126, full));
  }
  TEST_ASSERT_EQUAL(100, tankPercent(30, 30));
}
void decimal_ratio_and_fixed_weight() {
  auto c = config();
  auto &p = c.profiles[0];
  p.shotStopOnCustomWeight = 0;
  p.shotDose = 18;
  p.shotRatioX100 = 250;
  TEST_ASSERT_EQUAL_FLOAT(45, profileStopTarget(p));
  p.shotStopOnCustomWeight = 42;
  TEST_ASSERT_EQUAL_FLOAT(42, profileStopTarget(p));
}
void native_home_uses_original_assets_and_bounded_commands() {
  World w;
  w.ui.invalidate();
  const auto out = w.draw();
  TEST_ASSERT_TRUE(out.size() < 220);
  bool ring = false, xy = false;
  for (const auto &c : out) {
    ring |= c.find("cirs 102,116,74,") == 0;
    xy |= c == "sendxy=1";
  }
  TEST_ASSERT_TRUE(ring && xy);
  TEST_ASSERT_EQUAL(0, w.cfg.activeProfile);
}
void editing_and_naming_do_not_select() {
  World w;
  w.tap("Menu");
  w.tap("Profiles");
  auto h = w.ui.targets();
  TEST_ASSERT_EQUAL(10, h.size() - 1); // header back + five pairs
  w.ui.touch(280, 40 + 38 + 16, true);
  w.ui.touch(280, 40 + 38 + 16, false);
  w.draw();
  TEST_ASSERT_EQUAL(1, w.ui.editorProfile());
  TEST_ASSERT_EQUAL(0, w.cfg.activeProfile);
  w.tap("Rename");
  w.tap("S");
  w.tap("O");
  w.tap("U");
  w.tap("P");
  w.tap("OK");
  w.tap("Save");
  TEST_ASSERT_EQUAL_STRING("SOUP", w.cfg.profiles[1].name);
  TEST_ASSERT_EQUAL(0, w.cfg.activeProfile);
  for (const auto &call : calls)
    TEST_ASSERT_FALSE(call.first == Command::SelectProfile);
}
void all_step_banks_remain_reachable() {
  World w;
  w.tap("Menu");
  w.tap("Profiles");
  w.ui.touch(280, 56, true);
  w.ui.touch(280, 56, false);
  w.draw();
  std::set<std::string> seen;
  const char *names[] = {"Temp & dose", "Preinfusion", "Soak",
                         "Ramp",        "Transition",  "Brew profile"};
  for (unsigned g = 0; g < 6; g++) {
    w.tap(names[g]);
    for (bool mode : {false, true}) {
      w.cfg.profiles[0].preinfusionFlowState = mode;
      w.cfg.profiles[0].tpType = mode;
      w.cfg.profiles[0].mfProfileState = mode;
      w.ui.invalidate();
      w.draw();
      for (const auto &h : w.ui.targets())
        seen.insert(h.label);
    }
    w.tap("All");
  }
  for (const auto *key :
       {"preinfusionSec", "preinfusionFlowTime", "preinfusionBar",
        "preinfusionFlowVol", "soakTimePressure", "soakTimeFlow",
        "tpProfilingHoldLimit", "tfProfileHoldLimit", "mpProfilingStart",
        "mfProfileStart"})
    TEST_ASSERT_TRUE_MESSAGE(seen.count(key), key);
}
void numeric_edit_retains_target_and_validates() {
  World w;
  w.tap("Menu");
  w.tap("Machine settings");
  w.tap("Boiler");
  w.tap("Idle divider: 5");
  w.tap("0");
  w.tap("OK");
  TEST_ASSERT_EQUAL(static_cast<int>(Page::Keyboard),
                    static_cast<int>(w.ui.page()));
  TEST_ASSERT_EQUAL(5, w.cfg.mainDivider);
  w.tap("Clear");
  w.tap("2");
  w.tap("OK");
  TEST_ASSERT_EQUAL(2, w.cfg.mainDivider);
  TEST_ASSERT_EQUAL(0, w.cfg.activeProfile);
}
void raw_touch_frames_and_sleep_do_not_fake_actions() {
  World w;
  auto h = *std::find_if(w.ui.targets().begin(), w.ui.targets().end(),
                         [](const Hit &h) { return h.label == "Menu"; });
  const unsigned x = h.x + 10, y = h.y + 10;
  const uint8_t press[] = {
      0x67, 0,  static_cast<uint8_t>(x), 0, static_cast<uint8_t>(y), 1, 255,
      255,  255};
  for (unsigned i = 0; i < 8; i++)
    w.ui.receive(press[i]);
  TEST_ASSERT_EQUAL(static_cast<int>(Page::Home),
                    static_cast<int>(w.ui.page()));
  w.ui.receive(255);
  uint8_t release[] = {
      0x67, 0,  static_cast<uint8_t>(x), 0, static_cast<uint8_t>(y), 0, 255,
      255,  255};
  for (auto b : release)
    w.ui.receive(b);
  w.draw();
  TEST_ASSERT_EQUAL(static_cast<int>(Page::Menu),
                    static_cast<int>(w.ui.page()));
  w.ui.returnHome();
  w.draw();
  release[0] = 0x68;
  for (auto b : release)
    w.ui.receive(b);
  TEST_ASSERT_EQUAL(static_cast<int>(Page::Home),
                    static_cast<int>(w.ui.page()));
}
void manual_slider_and_original_graph_are_retained() {
  World w;
  w.tap("Menu");
  w.tap("Manual flow");
  TEST_ASSERT_EQUAL(0, w.ui.manualTarget());
  const uint8_t v[] = {0x71, 57, 0, 0, 0, 255, 255, 255};
  for (auto b : v)
    w.ui.receive(b);
  TEST_ASSERT_EQUAL(57, w.ui.manualTarget());
  w.live.brewing = true;
  w.draw();
  TEST_ASSERT_EQUAL(static_cast<int>(Page::Manual),
                    static_cast<int>(w.ui.page()));
  w.live.brewing = false;
  w.draw();
  w.tap("Auto");
  w.live.brewing = true;
  auto out = w.draw();
  TEST_ASSERT_EQUAL(static_cast<int>(Page::Graph),
                    static_cast<int>(w.ui.page()));
  TEST_ASSERT_TRUE(std::find(out.begin(), out.end(), "page brewGraph") !=
                   out.end());
}
void persistent_warning_does_not_restart_render_queue() {
  World w;
  w.ui.message("TEMP READ ERROR");
  w.ui.tick(500, w.live);
  TEST_ASSERT_FALSE(w.ui.empty());
  w.ui.pop();
  const auto *before = w.ui.front();
  const auto front = *before;
  w.ui.message("TEMP READ ERROR");
  TEST_ASSERT_EQUAL_STRING(front.c_str(), w.ui.front()->c_str());
}

void recipe_initialization_is_once_and_preserves_other_slots() {
  auto defaults = config(), c = defaults;
  c.activeProfile = 4;
  c.scalesF1 = -4000;
  c.profiles[3].setpoint = 87;
  TEST_ASSERT_TRUE(initializeIndigoUpgrade(c, defaults, 12));
  TEST_ASSERT_EQUAL(0, c.activeProfile);
  TEST_ASSERT_EQUAL(-4000, c.scalesF1);
  TEST_ASSERT_EQUAL(87, c.profiles[3].setpoint);
  c.activeProfile = 2;
  c.profiles[0].setpoint = 89;
  TEST_ASSERT_FALSE(initializeIndigoUpgrade(c, defaults, 13));
  TEST_ASSERT_EQUAL(2, c.activeProfile);
  TEST_ASSERT_EQUAL(89, c.profiles[0].setpoint);
  const auto before = settingsChecksum(&c, sizeof(c));
  c.scalesF1++;
  TEST_ASSERT_NOT_EQUAL(before, settingsChecksum(&c, sizeof(c)));
}
void brew_mode_toggle_keeps_pressure_and_flow_values() {
  World w;
  w.tap("Menu");
  w.tap("Profiles");
  w.ui.touch(280, 56, true);
  w.ui.touch(280, 56, false);
  w.draw();
  w.tap("Brew profile");
  w.tap("Pressure / flow");
  TEST_ASSERT_TRUE(w.cfg.profiles[0].mfProfileState);
  TEST_ASSERT_TRUE(
      std::any_of(w.ui.targets().begin(), w.ui.targets().end(),
                  [](const Hit &h) { return h.label == "mfProfileStart"; }));
  w.tap("Flow / pressure");
  TEST_ASSERT_FALSE(w.cfg.profiles[0].mfProfileState);
  TEST_ASSERT_EQUAL_FLOAT(6, w.cfg.profiles[0].mpProfilingStart);
}
void nonfinite_temperature_does_not_break_drawing() {
  World w;
  w.sensor.temperature = std::numeric_limits<float>::quiet_NaN();
  w.ui.invalidate();
  w.draw();
  TEST_ASSERT_EQUAL(static_cast<int>(Page::Home),
                    static_cast<int>(w.ui.page()));
}

void held_switch_cannot_commit_keyboard_changes() {
  World w;
  w.tap("Temperature");
  w.tap("Temperature C: 92.0");
  w.tap("8");
  w.tap("8");
  w.live.physicalBrew = true;
  w.draw();
  w.tap("OK");
  TEST_ASSERT_EQUAL(92, w.cfg.profiles[0].setpoint);
  TEST_ASSERT_EQUAL(static_cast<int>(Page::Message),
                    static_cast<int>(w.ui.page()));
}
void native_errors_and_restart_are_reported_without_actions() {
  World w;
  const uint8_t error[] = {0x1a, 255, 255, 255};
  for (auto b : error)
    w.ui.receive(b);
  TEST_ASSERT_EQUAL(1, w.ui.displayErrors());
  TEST_ASSERT_EQUAL(0x1a, w.ui.displayError());
  w.ui.tick(10000, w.live);
  while (!w.ui.empty())
    w.ui.pop();
  const uint8_t restart[] = {0x88, 255, 255, 255};
  for (auto b : restart)
    w.ui.receive(b);
  TEST_ASSERT_TRUE(static_cast<uint32_t>(10000 - w.ui.lastContact()) >= 1500);
  TEST_ASSERT_EQUAL(static_cast<int>(Page::Home),
                    static_cast<int>(w.ui.page()));
}
void original_ten_second_return_delay_is_preserved() {
  World w;
  w.cfg.homeOnShotFinish = true;
  w.live.brewing = true;
  w.draw();
  w.live.brewing = false;
  w.ui.tick(1000, w.live);
  while (!w.ui.empty())
    w.ui.pop();
  w.ui.tick(10999, w.live);
  TEST_ASSERT_EQUAL(static_cast<int>(Page::Graph),
                    static_cast<int>(w.ui.page()));
  while (!w.ui.empty())
    w.ui.pop();
  w.ui.tick(11000, w.live);
  TEST_ASSERT_EQUAL(static_cast<int>(Page::Home),
                    static_cast<int>(w.ui.page()));
}
void water_warning_clears_after_refill() {
  World w;
  w.live.waterValid = true;
  w.sensor.waterLvl = 9;
  auto warning = w.draw();
  // Advance enough for the periodic native Home update.
  w.ui.tick(30000, w.live);
  warning.clear();
  while (const auto *command = w.ui.front()) {
    warning.push_back(*command);
    w.ui.pop();
  }
  TEST_ASSERT_TRUE(
      std::any_of(warning.begin(), warning.end(), [](const std::string &s) {
        return s.find("LOW WATER") != std::string::npos;
      }));
  w.sensor.waterLvl = 100;
  w.ui.tick(31000, w.live);
  std::vector<std::string> restored;
  while (const auto *command = w.ui.front()) {
    restored.push_back(*command);
    w.ui.pop();
  }
  TEST_ASSERT_TRUE(std::find(restored.begin(), restored.end(),
                             "fill 0,0,320,39,4326") != restored.end());
  TEST_ASSERT_TRUE(
      std::any_of(restored.begin(), restored.end(), [](const std::string &s) {
        return s.find("Turbo") != std::string::npos;
      }));
  TEST_ASSERT_FALSE(
      std::any_of(restored.begin(), restored.end(), [](const std::string &s) {
        return s.find("LOW WATER") != std::string::npos;
      }));
}
void run() {
  RUN_TEST(water_warning_clears_after_refill);
  RUN_TEST(held_switch_cannot_commit_keyboard_changes);
  RUN_TEST(native_errors_and_restart_are_reported_without_actions);
  RUN_TEST(original_ten_second_return_delay_is_preserved);
  RUN_TEST(recipe_initialization_is_once_and_preserves_other_slots);
  RUN_TEST(brew_mode_toggle_keeps_pressure_and_flow_values);
  RUN_TEST(nonfinite_temperature_does_not_break_drawing);
  RUN_TEST(checksum_and_validation);
  RUN_TEST(migrate_real_schema12);
  RUN_TEST(save_scopes_are_independent);
  RUN_TEST(tank_full_does_not_move_low_limit);
  RUN_TEST(decimal_ratio_and_fixed_weight);
  RUN_TEST(native_home_uses_original_assets_and_bounded_commands);
  RUN_TEST(editing_and_naming_do_not_select);
  RUN_TEST(all_step_banks_remain_reachable);
  RUN_TEST(numeric_edit_retains_target_and_validates);
  RUN_TEST(raw_touch_frames_and_sleep_do_not_fake_actions);
  RUN_TEST(manual_slider_and_original_graph_are_retained);
  RUN_TEST(persistent_warning_does_not_restart_render_queue);
}
} // namespace IndigoTest
