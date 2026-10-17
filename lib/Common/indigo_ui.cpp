#include "indigo/ui.h"
#include <algorithm>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
namespace Indigo {
namespace {
constexpr int BG = 0x10e6, FG = 0xef9f, PANEL = 0x21aa, LINE = 0x42ef,
              MUTED = 0xadfa, ACCENT = 0x9d7f, WATER = 0x76bb;
enum {
  ToggleScale = 102,
  HomeTemp = 101,
  BackNav = 100,
  Home = 1,
  Menu,
  Profiles,
  EditProfile,
  SelectProfile,
  Step,
  PrevStep,
  NextStep,
  Save,
  Rename,
  Preview,
  Defaults,
  System,
  SystemGroup,
  Water,
  Tray,
  Cleaning,
  Backflush,
  Descale,
  Manual,
  Auto,
  Tare,
  Flush,
  FieldEdit,
  Curve,
  StopRatio,
  StopWeight,
  SetFull,
  SetTray,
  Previous,
  Next,
  Key,
  Okay,
  Cancel,
  Confirm,
  Reset,
  CloseMessage
};
const char *stepNames[] = {"Temp & dose", "Preinfusion", "Soak",
                           "Ramp",        "Transition",  "Brew profile"};
const char *systemNames[] = {"Boiler", "Scales & pump", "Screen & warmup",
                             "Brew preferences", "Power & lighting"};
std::string number(double v, unsigned decimals = 1) {
  char b[32];
  std::snprintf(b, sizeof(b), "%.*f", decimals, v);
  return b;
}
std::string safe(std::string s) {
  for (auto &c : s)
    if (c == '"' || c == '\\' || static_cast<unsigned char>(c) < 32 ||
        static_cast<unsigned char>(c) > 126)
      c = ' ';
  return s;
}
} // namespace
void Ui::emit(const char *f, ...) {
  char b[192];
  va_list a;
  va_start(a, f);
  int n = std::vsnprintf(b, sizeof(b), f, a);
  va_end(a);
  if (n > 0 && n < static_cast<int>(sizeof(b)) && commands.size() < 400)
    commands.emplace_back(b);
}
void Ui::fill(int x, int y, int w, int h, int c) {
  emit("fill %d,%d,%d,%d,%d", x, y, w, h, c);
}
void Ui::text(const std::string &s, int x, int y, int w, int h, int font,
              int color, int bg, int align) {
  emit("xstr %d,%d,%d,%d,%d,%d,%d,%d,1,1,\"%s\"", x, y, w, h, font, color,
       bg < 0 ? BG : bg, align, safe(s).c_str());
}
void Ui::hit(const std::string &s, int x, int y, int w, int h, int action,
             int arg) {
  if (x >= 0 && y >= 0 && x + w <= 320 && y + h <= 240 && hits.size() < 48)
    hits.push_back({x, y, w, h, action, arg, s});
}
void Ui::button(const std::string &s, int x, int y, int w, int h, int action,
                int arg, bool selected) {
  fill(x, y, w, h, selected ? WATER : PANEL);
  text(s, x + 2, y + 1, w - 4, h - 2, 0, selected ? BG : FG,
       selected ? WATER : PANEL, 1);
  hit(s, x, y, w, h, action, arg);
}
void Ui::begin() {
  editingProfile = cfg.activeProfile;
  show(Page::Home);
}
void Ui::show(Page p) {
  if (route == Page::Manual && p != Page::Manual) {
    if (!invoke(Command::ExitManual))
      return;
    manualTenths = 0;
  }
  if ((route == Page::Backflush || route == Page::Descale) && p != route) {
    if (!invoke(Command::ExitCleaning))
      return;
  }
  route = p;
  section = 0;
  commands.clear();
  hits.clear();
  dirty = true;
  down = false;
  ringProgress = -1;
}
void Ui::returnHome() { show(Page::Home); }
void Ui::message(const char *s) {
  if (route == Page::Message && popup == s)
    return;
  if (route != Page::Message)
    messageReturn = route;
  popup = s;
  route = Page::Message;
  commands.clear();
  hits.clear();
  dirty = true;
  down = false;
}
void Ui::confirm(const char *s, int action, int arg) {
  messageReturn = route;
  popup = s;
  confirmedAction = action;
  confirmedArg = arg;
  route = Page::Confirm;
  commands.clear();
  hits.clear();
  dirty = true;
}
void Ui::header(const char *title, bool save) {
  button("< Back", 4, 3, 70, 28, BackNav);
  text(title, 80, 3, save ? 174 : 236, 28);
  if (save)
    button("Save", 260, 3, 56, 28, Save);
  fill(4, 34, 312, 1, LINE);
}
void Ui::home(bool full) {
  if (full) {
    fill(0, 0, 320, 240, BG);
    emit("cirs 102,116,74,%d", LINE);
    emit("cirs 102,116,62,%d", PANEL);
    ringProgress = 0;
    text("WEIGHT", 196, 54, 80, 18, 0, MUTED);
    text("PRESSURE", 196, 117, 85, 18, 0, MUTED);
    text("WATER", 278, 35, 40, 18, 0, MUTED, BG, 2);
    fill(288, 58, 16, 110, LINE);
    fill(4, 199, 312, 1, LINE);
    text("Tare", 8, 203, 88, 32, 0, FG, BG, 1);
    hit("Tare", 8, 201, 88, 38, Tare);
    text("Flush", 112, 203, 53, 32, 0, MUTED, BG, 2);
    hit("Flush", 112, 201, 96, 38, Flush);
    button("Menu", 227, 203, 82, 33, Menu);
    fill(227, 203, 82, 33, PANEL);
    for (unsigned i = 0; i < 3; i++)
      emit("line 240,%u,249,%u,%d", 213 + i * 4, 213 + i * 4, MUTED);
    text("Menu", 255, 203, 50, 33, 0, FG, PANEL, 1);
    hit("Temperature", 28, 42, 148, 148, HomeTemp, 0);
    hit("Scales", 191, 51, 87, 51, SystemGroup, 1);
    hit("Water", 283, 49, 26, 143, Water);
  }
  fill(0, 0, 320, 39, BG);
  if (live.waterValid && sensors.waterLvl < 10)
    text("LOW WATER - FILL TANK", 0, 0, 320, 35, 0, BG, ACCENT, 1);
  else {
    text("PROFILE", 10, 2, 150, 16, 0, MUTED);
    text(cfg.profiles[cfg.activeProfile].name, 10, 19, 300, 20);
  }
  const int progress = std::max(
      0, std::min(120,
                  static_cast<int>(std::lround(
                      120 *
                      (std::isfinite(sensors.temperature) ? sensors.temperature
                                                          : 0) /
                      cfg.profiles[cfg.activeProfile].setpoint))));
  const int start = std::min(ringProgress, progress),
            end = std::max(ringProgress, progress);
  for (int i = start; i <= end; i++) {
    const double angle = -1.570796327 + i * 6.283185307 / 120;
    emit("cirs %d,%d,6,%d",
         102 + static_cast<int>(std::lround(std::cos(angle) * 68)),
         116 + static_cast<int>(std::lround(std::sin(angle) * 68)),
         progress >= ringProgress ? ACCENT : LINE);
  }
  ringProgress = progress;
  const char *status =
      !live.startup && !live.flushing                                         ? "STARTING"
      : live.flushing                                       ? "FLUSHING"
      : live.steam                                          ? "STEAM"
      : cfg.warmupState && !warmupDone && live.uptime < 900 ? "WARMUP"
      : sensors.temperature < cfg.profiles[cfg.activeProfile].setpoint - 2
          ? "HEATING"
          : "READY";
  text(status, 64, 76, 76, 18, 0, MUTED, PANEL, 1);
  text(std::isfinite(sensors.temperature) ? number(sensors.temperature) : "--",
       46, 94, 112, 44, 6, FG, PANEL, 1);
  text("C", 80, 137, 44, 18, 0, FG, PANEL, 1);
  text("SET " +
           number(live.steam ? cfg.steamSetPoint
                             : cfg.profiles[cfg.activeProfile].setpoint,
                  0) +
           " C",
       70, 153, 64, 16, 0, MUTED, PANEL, 1);
  text(!scaleReadout           ? "OFF"
       : sensors.scalesPresent ? number(sensors.weight) + "g"
                               : "-- g",
       194, 75, 85, 32, 1);
  text(number(sensors.smoothedPressure) + " bar", 194, 135, 88, 30, 1);
  fill(290, 60, 12, 106, PANEL);
  if (live.waterValid) {
    const int h = std::lround(sensors.waterLvl * 1.06);
    fill(290, 166 - h, 12, h, sensors.waterLvl < 10 ? ACCENT : WATER);
  }
  text(live.waterValid ? number(sensors.waterLvl, 0) + "%" : "--", 278, 174, 40,
       20, 0, WATER, BG, 2);
  emit("cirs 180,219,5,%d", live.flushing ? ACCENT : BG);
  if (!live.flushing)
    emit("cir 180,219,5,%d", MUTED);
}
void Ui::step() {
  const unsigned g = group;
  button("All", 4, 3, 39, 29, EditProfile, editingProfile);
  button("<", 46, 3, 28, 29, PrevStep);
  text(stepNames[g], 79, 3, 154, 29, 0, FG, BG, 1);
  button(">", 236, 3, 28, 29, NextStep);
  button("Save", 269, 3, 47, 29, Save);
  fill(4, 35, 312, 1, LINE);
  if (g == 0) {
    unsigned y = 42;
    for (const char *key : {"setpoint", "shotDose"}) {
      for (unsigned i = 0; i < profileFieldCount; i++)
        if (std::strcmp(profileFields[i].key, key) == 0) {
          const auto &f = profileFields[i];
          button(std::string(f.label) + ": " + number(readField(&profile(), f)),
                 8, y, 304, 31, FieldEdit, i);
          y += 36;
        }
    }
    button(std::string("Stop on weight: ") +
               (profile().stopOnWeightState ? "ON" : "OFF"),
           8, 114, 304, 29, FieldEdit, 3, profile().stopOnWeightState);
    // Find the descriptor rather than depending on its ordering.
    hits.back().arg = 0;
    for (unsigned i = 0; i < profileFieldCount; i++)
      if (std::strcmp(profileFields[i].key, "stopOnWeightState") == 0)
        hits.back().arg = i;
    const bool fixed = profile().shotStopOnCustomWeight > 0;
    button("Ratio 1:x", 8, 149, 148, 29, StopRatio, 0, !fixed);
    button("Weight g", 164, 149, 148, 29, StopWeight, 0, fixed);
    const char *key = fixed ? "shotStopOnCustomWeight" : "shotRatioX100";
    for (unsigned i = 0; i < profileFieldCount; i++)
      if (std::strcmp(profileFields[i].key, key) == 0) {
        const double v = readField(&profile(), profileFields[i]);
        button(fixed ? number(v) + " g" : "1:" + number(v / 100.), 8, 183, 304,
               30, FieldEdit, i);
      }
    text(number(profile().shotDose) + " g in > " +
             number(profileStopTarget(profile())) + " g out",
         8, 219, 304, 18, 0, MUTED, BG, 1);
    return;
  }
  const bool flow = g == 1 || g == 2 ? profile().preinfusionFlowState
                    : g == 4         ? profile().tpType
                                     : profile().mfProfileState;
  const Field *curve = nullptr;
  unsigned cells = 0;
  for (unsigned i = 0; i < profileFieldCount; i++) {
    const auto &f = profileFields[i];
    if (f.group != g || (f.bank && f.bank != (flow ? 2 : 1)))
      continue;
    if (f.role == Role::Enable) {
      button(readField(&profile(), f) ? "ON" : "OFF", 8, 41, 68, 28, FieldEdit,
             i, readField(&profile(), f));
      continue;
    }
    if (f.role == Role::Mode) {
      button(flow ? "Flow / pressure" : "Pressure / flow", 84, 41, 228, 28,
             FieldEdit, i, flow);
      continue;
    }
    if (f.role == Role::Curve) {
      curve = &f;
      continue;
    }
    const unsigned x = 8 + cells % 2 * 156, y = 76 + cells / 2 * 41;
    fill(x, y, 148, 36, PANEL);
    text(f.label, x + 4, y + 1, 140, 16, 0, MUTED, PANEL);
    text(f.kind == Kind::Boolean
             ? (readField(&profile(), f) ? "ON" : "OFF")
             : number(readField(&profile(), f), f.kind == Kind::U16 ? 0 : 1),
         x + 4, y + 17, 140, 18, 0, FG, PANEL);
    hit(f.key, x, y, 148, 36, FieldEdit, i);
    cells++;
  }
  if (g == 2)
    text(flow ? "Flow PI time bank" : "Pressure PI time bank", 84, 41, 228, 28,
         0, MUTED);
  if (g == 3)
    text("PI > extraction ramp", 8, 42, 304, 25, 0, MUTED);
  if (curve) {
    const int index = curve - profileFields;
    const char *labels[] = {"In/out", "In", "Out", "Linear", "Instant"};
    for (unsigned i = 0; i < 5; i++)
      button(labels[i], 8 + i * 62, 204, 56, 29, Curve, index * 5 + i,
             readField(&profile(), *curve) == i);
  }
}
void Ui::fields(bool system) {
  const auto *table = system ? systemFields : profileFields;
  const auto count = system ? systemFieldCount : profileFieldCount;
  header(system ? systemNames[group] : stepNames[group], true);
  std::vector<unsigned> indices;
  for (unsigned i = 0; i < count; i++)
    if (table[i].group == group)
      indices.push_back(i);
  const unsigned pages = (indices.size() + 3) / 4;
  if (section >= pages)
    section = 0;
  for (unsigned r = 0; r < 4 && section * 4 + r < indices.size(); r++) {
    const unsigned i = indices[section * 4 + r];
    const auto &f = table[i];
    const double v = readField(&cfg, f);
    button(std::string(f.label) + ": " +
               (f.kind == Kind::Boolean
                    ? (v ? "ON" : "OFF")
                    : number(v, f.kind == Kind::Float ? 4 : 0)),
           8, 42 + r * 38, 304, 32, FieldEdit, 1000 + i);
  }
  if (system && group == 1) {
    button(scaleReadout ? "Weight ON" : "Weight OFF", 8, 166, 148, 32,
           ToggleScale);
    button("Tare", 164, 166, 148, 32, Tare);
  }
  if (pages > 1) {
    button("<", 8, 204, 70, 29, Previous);
    text(number(section + 1, 0) + " / " + number(pages, 0), 90, 204, 140, 29, 0,
         MUTED, BG, 1);
    button(">", 242, 204, 70, 29, Next);
  }
}
void Ui::keyboard() {
  text(renaming ? "Profile name" : editingField->label, 8, 4, 304, 22, 0,
       MUTED);
  fill(8, 31, 304, 36, PANEL);
  text(input, 12, 32, 296, 34, 1, FG, PANEL);
  if (renaming) {
    const char *rows[] = {nameDigits ? "1234567890" : "QWERTYUIOP",
                          nameDigits ? "-_." : "ASDFGHJKL",
                          nameDigits ? "" : "ZXCVBNM"};
    for (unsigned y = 0; y < 3; y++)
      for (unsigned x = 0; rows[y][x]; x++) {
        char c = rows[y][x];
        if (!uppercase && c >= 'A' && c <= 'Z')
          c += 32;
        button(std::string(1, c), 6 + x * 31, 77 + y * 35, 29, 31, Key, c);
      }
    button(uppercase ? "abc" : "ABC", 8, 184, 62, 28, Key, -6);
    button("123", 76, 184, 54, 28, Key, -7);
    button("Space", 136, 184, 70, 28, Key, ' ');
    button("Del", 212, 184, 44, 28, Key, -1);
    button("OK", 262, 184, 50, 28, Okay);
    button("Cancel", 8, 218, 80, 20, Cancel);
  } else {
    const char *keys = "1234567890.-";
    for (unsigned i = 0; i < 12; i++)
      button(std::string(1, keys[i]), 8 + i % 3 * 77, 78 + i / 3 * 37, 71, 32,
             Key, keys[i]);
    button("Del", 241, 78, 71, 32, Key, -1);
    button("Clear", 241, 115, 71, 32, Key, -2);
    button("Cancel", 241, 152, 71, 32, Cancel);
    button("OK", 241, 189, 71, 32, Okay);
  }
}
void Ui::render() {
  hits.clear();
  if (route == Page::Graph) {
    emit("sendxy=0");
    emit("page brewGraph");
    emit("mode_timer.en=0"); // Controller owns the original ten-second return
                             // delay.
    emit("tsw 255,1");
    return;
  }
  if (route == Page::Preview) {
    emit("sendxy=0");
    emit("page graphPreview");
    return;
  }
  // Freeze the stock page's visual timers and touch handlers. Its global
  // variables/assets remain available to the original brew graph.
  emit("page %s", route == Page::Manual ? "bM" : "home");
  if (route == Page::Manual) {
    emit("tm0.en=0");
    emit("tm1.en=0");
    emit("tm2.en=0");
  } else
    emit("tm0.en=0");
  emit("bkcmd=2");
  emit("tsw 255,0");
  emit("vis 255,0");
  emit("sendxy=1");
  emit("thsp=%u", cfg.lcdSleep * 60u);
  emit("thup=1");
  emit("dim=%u", cfg.brightness);
  fill(0, 0, 320, 240, BG);
  switch (route) {
  case Page::Home:
    home(true);
    break;
  case Page::Menu: {
    header("Menu");
    const char *names[] = {"Profiles",   "Manual flow", "Machine settings",
                           "Water tank", "Cleaning",    "Tray guard"};
    int actions[] = {Profiles, Manual, System, Water, Cleaning, Tray};
    for (int i = 0; i < 6; i++)
      button(names[i], 8 + i % 2 * 156, 43 + i / 2 * 57, 148, 49, actions[i]);
    break;
  }
  case Page::Profiles:
    header("Profiles");
    for (unsigned i = 0; i < 5; i++) {
      button(cfg.profiles[i].name, 8, 40 + i * 38, 232, 33, SelectProfile, i,
             cfg.activeProfile == i);
      button("Edit", 247, 40 + i * 38, 65, 33, EditProfile, i);
    }
    break;
  case Page::Profile:
    header(profile().name, true);
    for (unsigned i = 0; i < 6; i++)
      button(stepNames[i], 8 + i % 2 * 156, 41 + i / 2 * 51, 148, 45, Step, i);
    button("Rename", 8, 200, 96, 33, Rename);
    button("Preview", 112, 200, 96, 33, Preview);
    button("Defaults", 216, 200, 96, 33, Defaults);
    break;
  case Page::Step:
    step();
    break;
  case Page::System:
    header("Settings");
    text("Up " + number(live.uptime / 3600, 0) + ":" +
             number(live.uptime / 60 % 60, 0),
         170, 3, 146, 28, 0, MUTED, BG, 2);
    for (unsigned i = 0; i < 5; i++)
      button(systemNames[i], 8, 40 + i * 33, 304, 28, SystemGroup, i);
    button("Reset ALL settings", 8, 209, 304, 27, Reset);
    break;
  case Page::SystemFields:
    fields(true);
    break;
  case Page::Water:
    header("Water tank", true);
    text(live.waterValid ? number(sensors.waterLvl, 0) + "%"
                         : "No valid reading",
         8, 45, 304, 40, 1, WATER);
    text("Range: " + number(live.distance, 0) + " mm", 8, 90, 304, 24);
    text("Full: " + number(cfg.tankFullDistanceMm, 0) + " mm", 8, 119, 304, 24,
         0, MUTED);
    button("Set current full tank to 100%", 8, 153, 304, 35, SetFull);
    for (unsigned i = 0; i < systemFieldCount; i++)
      if (std::strcmp(systemFields[i].key, "tankFullDistanceMm") == 0)
        button("Enter full distance", 8, 197, 178, 32, FieldEdit, 1000 + i);
    text("Low >125mm", 188, 197, 124, 32, 0, MUTED);
    break;
  case Page::Tray:
    header("Tray guard", true);
    text(std::isfinite(live.gross) ? number(live.gross) + " g gross"
                                   : "No fresh hardware load",
         8, 46, 304, 36, 1);
    text(live.trayStatus == 0   ? "Load present"
         : live.trayStatus == 1 ? "Place tray or container"
         : live.trayStatus == 2 ? "Hardware scales unavailable"
                                : "Guard needs calibration",
         8, 91, 304, 26, 0, MUTED);
    button("Capture NO-TRAY baseline", 8, 134, 304, 39, SetTray);
    for (unsigned i = 0; i < systemFieldCount; i++)
      if (std::strcmp(systemFields[i].key, "trayMinWeightG") == 0)
        button("Minimum: " + number(cfg.trayMinWeightG) + " g", 8, 187, 304, 36,
               FieldEdit, 1000 + i);
    break;
  case Page::Cleaning:
    header("Cleaning");
    button(live.flushing ? "Stop group flush" : "Group flush", 8, 48, 304, 43,
           Flush);
    button("Backflush", 8, 105, 148, 48, Backflush);
    button("Descale", 164, 105, 148, 48, Descale);
    text("Physical brew switch starts cycles", 8, 180, 304, 35, 0, MUTED, BG,
         1);
    break;
  case Page::Backflush:
  case Page::Descale:
    header(route == Page::Backflush ? "Backflush" : "Descale");
    text("Brew switch starts / stops", 8, 61, 304, 35, 1);
    text("Use normal cleaning procedure", 8, 111, 304, 30, 0, MUTED);
    text("Guard remains active", 8, 151, 304, 30, 0, MUTED);
    text("Cycle " + number(live.cleaningCycle, 0), 8, 192, 304, 30, 0, WATER);
    break;
  case Page::Manual: {
    header("Manual flow");
    button("Auto", 250, 3, 66, 28, Auto);
    const char *names[] = {"TEMP C",    "PRESSURE", "TIME s",
                           "FLOW ml/s", "WEIGHT g", "TARGET ml/s"};
    for (unsigned i = 0; i < 6; i++)
      text(names[i], 8 + i % 3 * 105, 43 + i / 3 * 60, 102, 18, 0, MUTED);
    text("Use brew switch to start / stop", 8, 160, 304, 20, 0, MUTED);
    emit("h0.val=%d", manualTenths);
    emit("vis h0,1");
    emit("tsw h0,1");
    hit("Manual flow slider", 15, 181, 288, 40, Key, -10);
    break;
  }
  case Page::Keyboard:
    keyboard();
    break;
  case Page::Message:
  case Page::Confirm:
    text(route == Page::Confirm ? "Confirm" : "Notice", 8, 32, 304, 32, 1);
    {
      std::string rest = popup;
      unsigned y = 80;
      while (!rest.empty() && y < 170) {
        unsigned n = std::min<size_t>(36, rest.size());
        if (n < rest.size()) {
          auto space = rest.rfind(' ', n);
          if (space != std::string::npos && space > 0)
            n = space;
        }
        text(rest.substr(0, n), 12, y, 296, 23);
        rest.erase(0, n);
        if (!rest.empty() && rest.front() == ' ')
          rest.erase(0, 1);
        y += 24;
      }
    }
    if (route == Page::Confirm) {
      button("Cancel", 8, 190, 145, 36, CloseMessage);
      button("Confirm", 167, 190, 145, 36, Confirm);
    } else
      button("OK", 60, 190, 200, 36, CloseMessage);
    break;
  default:
    break;
  }
}
void Ui::edit(const Field &f, bool system) {
  if (live.brewing || live.flushing || live.steam || live.hotWater) {
    message("Stop the active mode before editing");
    return;
  }
  void *record =
      system ? static_cast<void *>(&cfg) : static_cast<void *>(&profile());
  if (f.kind == Kind::Boolean) {
    writeField(record, f, !readField(record, f));
    invoke(Command::SettingsChanged, system ? -1 : editingProfile);
    dirty = true;
    return;
  }
  editingField = &f;
  editingSystem = system;
  renaming = false;
  keyboardReturn = route;
  input = number(readField(record, f), f.kind == Kind::Float ? 4 : 0);
  if (std::strcmp(f.key, "shotRatioX100") == 0)
    input = number(readField(record, f) / 100., 2);
  firstKey = true;
  show(Page::Keyboard);
}
void Ui::activate(int action, int arg) {
  const bool edits = action == FieldEdit || action == Curve ||
                     action == StopRatio || action == StopWeight ||
                     action == Rename || action == Okay;
  if (edits && (live.brewing || live.physicalBrew || live.flushing ||
                live.steam || live.hotWater)) {
    message("Turn physical switches OFF before editing");
    return;
  }

  switch (action) {
  case BackNav:
    show(route == Page::Profile                               ? Page::Profiles
         : route == Page::SystemFields                        ? Page::System
         : route == Page::Backflush || route == Page::Descale ? Page::Cleaning
         : route == Page::Menu                                ? Page::Home
                                                              : Page::Menu);
    break;
  case ToggleScale:
    scaleReadout = !scaleReadout;
    dirty = true;
    break;
  case HomeTemp:
    editingProfile = cfg.activeProfile;
    group = 0;
    show(Page::Step);
    break;
  case Home:
    show(Page::Home);
    break;
  case Menu:
    show(Page::Menu);
    break;
  case Profiles:
    show(Page::Profiles);
    break;
  case SelectProfile:
    if (!live.brewing && !live.flushing && !live.steam && !live.hotWater) {
      if (invoke(Command::SelectProfile, arg)) {
        cfg.activeProfile = arg;
        dirty = true;
      }
    } else
      message("Stop the active mode before selecting");
    break;
  case EditProfile:
    editingProfile = arg;
    show(Page::Profile);
    break;
  case Step:
    group = arg;
    show(Page::Step);
    break;
  case PrevStep:
    group = (group + 5) % 6;
    dirty = true;
    break;
  case NextStep:
    group = (group + 1) % 6;
    dirty = true;
    break;
  case Save:
    invoke(route == Page::Profile || route == Page::Step ? Command::SaveProfile
                                                         : Command::SaveSystem,
           editingProfile);
    break;
  case Rename:
    if (live.brewing) {
      message("Stop brewing before editing");
      break;
    }
    renaming = true;
    editingSystem = false;
    editingField = nullptr;
    keyboardReturn = route;
    input = profile().name;
    firstKey = true;
    show(Page::Keyboard);
    break;
  case Preview:
    if (invoke(Command::Preview, editingProfile))
      show(Page::Preview);
    break;
  case Defaults:
    if (arg == -1) {
      route = messageReturn;
      invoke(Command::DefaultProfile, editingProfile);
      dirty = true;
    } else
      confirm("Replace this profile with its default?", Defaults, -1);
    break;
  case System:
    show(Page::System);
    break;
  case SystemGroup:
    group = arg;
    show(Page::SystemFields);
    break;
  case Water:
    show(Page::Water);
    break;
  case Tray:
    show(Page::Tray);
    break;
  case Cleaning:
    show(Page::Cleaning);
    break;
  case Backflush:
    if (invoke(Command::EnterBackflush))
      show(Page::Backflush);
    break;
  case Descale:
    if (invoke(Command::EnterDescale))
      show(Page::Descale);
    break;
  case Manual:
    if (invoke(Command::EnterManual)) {
      manualTenths = 0;
      show(Page::Manual);
    }
    break;
  case Auto:
    if (live.brewing) {
      message("Turn brew switch OFF before Auto");
      break;
    }
    show(Page::Home);
    break;
  case Tare:
    invoke(Command::Tare);
    break;
  case Flush:
    invoke(Command::Flush);
    dirty = true;
    break;
  case FieldEdit:
    if (arg >= 1000) {
      const auto &f = systemFields[arg - 1000];
      edit(f, true);
    } else
      edit(profileFields[arg], false);
    break;
  case Curve:
    if (live.brewing) {
      message("Stop brewing before editing");
      break;
    }
    writeField(&profile(), profileFields[arg / 5], arg % 5);
    invoke(Command::SettingsChanged, editingProfile);
    dirty = true;
    break;
  case StopRatio:
    if (live.brewing) {
      message("Stop brewing before editing");
      break;
    }
    profile().shotStopOnCustomWeight = 0;
    invoke(Command::SettingsChanged, editingProfile);
    dirty = true;
    break;
  case StopWeight:
    if (live.brewing) {
      message("Stop brewing before editing");
      break;
    }
    profile().shotStopOnCustomWeight = profileStopTarget(profile());
    invoke(Command::SettingsChanged, editingProfile);
    dirty = true;
    break;
  case SetFull:
    if (arg == -1) {
      route = messageReturn;
      invoke(Command::CaptureFull);
      dirty = true;
    } else
      confirm("Tank full? Capture current distance as 100%?", SetFull, -1);
    break;
  case SetTray:
    if (arg == -1) {
      route = messageReturn;
      invoke(Command::CaptureTray);
      dirty = true;
    } else
      confirm(
          "Remove tray and all containers. Capture settled no-tray baseline?",
          SetTray, -1);
    break;
  case Reset:
    if (arg == -1) {
      route = messageReturn;
      invoke(Command::ResetAll);
      dirty = true;
    } else
      confirm("Reset ALL recipes AND machine settings?", Reset, -1);
    break;
  case Previous:
    if (section)
      section--;
    dirty = true;
    break;
  case Next:
    section++;
    dirty = true;
    break;
  case Key:
    if (arg == -10)
      break;
    if (arg == -1) {
      if (!input.empty())
        input.pop_back();
      firstKey = false;
    } else if (arg == -2) {
      input.clear();
      firstKey = false;
    } else if (arg == -6)
      uppercase = !uppercase;
    else if (arg == -7)
      nameDigits = !nameDigits;
    else {
      if (firstKey) {
        input.clear();
        firstKey = false;
      }
      if (input.size() < (renaming ? 24u : 12u)) {
        if (arg == '-' && !input.empty())
          break;
        if (arg == '.' && input.find('.') != std::string::npos)
          break;
        input += static_cast<char>(arg);
      }
    }
    dirty = true;
    break;
  case Okay: {
    bool valid = false;
    if (renaming) {
      valid = !input.empty() && input.size() <= 24;
      if (valid)
        std::snprintf(profile().name, sizeof(profile().name), "%s",
                      input.c_str());
    } else if (editingField) {
      char *end = nullptr;
      double n = std::strtod(input.c_str(), &end);
      if (std::strcmp(editingField->key, "shotRatioX100") == 0)
        n = std::lround(n * 100);
      valid = !input.empty() && end && !*end &&
              writeField(editingSystem ? static_cast<void *>(&cfg)
                                       : static_cast<void *>(&profile()),
                         *editingField, n);
    }
    if (valid) {
      invoke(Command::SettingsChanged, editingSystem ? -1 : editingProfile);
      show(keyboardReturn);
    } else {
      popup = "Invalid value";
      text("Invalid value", 8, 68, 304, 18, 0, ACCENT);
    }
    break;
  }
  case Cancel:
    show(keyboardReturn);
    break;
  case Confirm: {
    const int a = confirmedAction, b = confirmedArg;
    route = messageReturn;
    activate(a, b);
    break;
  }
  case CloseMessage:
    show(messageReturn);
    break;
  default:
    break;
  }
}
void Ui::touch(int x, int y, bool pressed) {
  if (!overlay() || x < 0 || y < 0 || x >= 320 || y >= 240 || !commands.empty())
    return;
  if (pressed) {
    down = true;
    pressX = x;
    pressY = y;
    return;
  }
  if (!down)
    return;
  down = false;
  for (const auto &h : hits)
    if (x >= h.x && x < h.x + h.w && y >= h.y && y < h.y + h.h &&
        pressX >= h.x && pressX < h.x + h.w && pressY >= h.y &&
        pressY < h.y + h.h) {
      activate(h.action, h.arg);
      return;
    }
}
void Ui::receive(uint8_t byte) {
  if (used && static_cast<uint32_t>(clockNow - packetAt) > 100)
    used = 0;
  if (used == 0) {
    if (byte == 0x67 || byte == 0x68)
      needed = 9;
    else if (byte == 0x71)
      needed = 8;
    else if (byte == 0x66)
      needed = 5;
    else if (byte == 0x87 || byte == 0x88 || byte == 0x00 || byte == 0x02 ||
             byte == 0x03 || byte == 0x04 || byte == 0x05 || byte == 0x11 ||
             byte == 0x12 || byte == 0x1a || byte == 0x1b || byte == 0x1c ||
             byte == 0x1d || byte == 0x1e)
      needed = 4;
    else if (byte == '#')
      needed = 4;
    else
      return;
    packetAt = clockNow;
    packet[used++] = byte;
    return;
  }
  packet[used++] = byte;
  if (used < needed)
    return;
  used = 0;
  if (packet[0] == '#') {
    if (packet[1] != 2)
      return;
    if (packet[2] == 0x50) {
      if (packet[3] == 0 && (!overlay()))
        show(Page::Home);
    } else if (packet[2] == 0x54 && route == Page::Graph && packet[3] == 4)
      invoke(Command::Tare, 1);
    return;
  }
  if (packet[needed - 1] != 255 || packet[needed - 2] != 255 ||
      packet[needed - 3] != 255)
    return;
  if (packet[0] == 0x87) {
    contactAt = clockNow;
    if (overlay()) {
      dirty = true;
      commands.clear();
    }
    return;
  }
  if (packet[0] == 0x88) {
    contactAt = clockNow - 1500u;
    show(Page::Home);
    return;
  }
  if (needed == 4) {
    errorCode = packet[0];
    errorCount++;
    return;
  }
  contactAt = clockNow;
  if (packet[0] == 0x67)
    touch(packet[1] * 256 + packet[2], packet[3] * 256 + packet[4],
          packet[5] != 0);
  else if (packet[0] == 0x71 && route == Page::Manual) {
    const uint32_t v = static_cast<uint32_t>(packet[1]) |
                       static_cast<uint32_t>(packet[2]) << 8 |
                       static_cast<uint32_t>(packet[3]) << 16 |
                       static_cast<uint32_t>(packet[4]) << 24;
    if (v <= 130)
      manualTenths = v;
  }
}
void Ui::tick(uint32_t now, const Telemetry &t) {
  live = t;
  if (t.brewing ||
      (sensors.temperature >= cfg.profiles[cfg.activeProfile].setpoint - 2 &&
       t.uptime % 60 <= 20))
    warmupDone = true;
  clockNow = now;
  if (t.brewing && !wasBrewing && route != Page::Manual &&
      route != Page::Backflush && route != Page::Descale &&
      route != Page::Message && route != Page::Confirm) {
    invoke(Command::Graph);
    show(Page::Graph);
  }
  if (!t.brewing && wasBrewing)
    finishedAt = now;
  wasBrewing = t.brewing;
  if (route == Page::Graph && !t.brewing && cfg.homeOnShotFinish &&
      finishedAt && static_cast<uint32_t>(now - finishedAt) >= 10000)
    show(Page::Home);
  if (dirty) {
    commands.clear();
    render();
    dirty = false;
    lastDraw = now;
    return;
  }
  if (!commands.empty() || static_cast<uint32_t>(now - lastDraw) < 250)
    return;
  lastDraw = now;
  if (static_cast<uint32_t>(now - heartbeatAt) >= 250) {
    emit("sendme");
    heartbeatAt = now;
  }
  if (route == Page::Home)
    home(false);
  if (route == Page::Manual) {
    emit("get h0.val");
    const std::string values[] = {number(sensors.temperature),
                                  number(sensors.smoothedPressure),
                                  number(t.elapsed),
                                  number(sensors.smoothedPumpFlow),
                                  sensors.scalesPresent ? number(sensors.weight)
                                                        : "--",
                                  number(manualTenths / 10.f)};
    for (unsigned i = 0; i < 6; i++)
      text(values[i], 8 + i % 3 * 105, 61 + i / 3 * 60, 102, 30, 1);
  }
}
} // namespace Indigo
