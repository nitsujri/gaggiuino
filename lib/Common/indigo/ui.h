#pragma once
#include "fields.h"
#include "sensors_state.h"
#include <deque>
#include <string>
#include <vector>
namespace Indigo {
enum class Page : uint8_t {
  Home,
  Menu,
  Profiles,
  Profile,
  Step,
  System,
  SystemFields,
  Water,
  Tray,
  Cleaning,
  Backflush,
  Descale,
  Manual,
  Graph,
  Preview,
  Keyboard,
  Confirm,
  Message
};
enum class Command : uint8_t {
  SaveProfile,
  SaveSystem,
  SelectProfile,
  Tare,
  Flush,
  CaptureFull,
  CaptureTray,
  SettingsChanged,
  DefaultProfile,
  ResetAll,
  EnterManual,
  ExitManual,
  EnterBackflush,
  EnterDescale,
  ExitCleaning,
  Graph,
  Preview
};
struct Telemetry {
  bool physicalBrew = false;
  bool brewing = false, steam = false, hotWater = false, flushing = false, startup = false,
       waterValid = false;
  uint16_t distance = 0;
  float elapsed = 0, gross = 0;
  uint8_t trayStatus = 3;
  unsigned cleaningCycle = 0;
  uint32_t uptime = 0;
};
struct Hit {
  int x, y, w, h, action, arg;
  std::string label;
};
class Ui {
public:
  using Callback = bool (*)(Command, int);
  Ui(eepromValues_t &c, SensorState &s, Callback cb) : cfg(c), sensors(s), callback(cb) {}
  void begin();
  void tick(uint32_t now, const Telemetry &t);
  void receive(uint8_t byte);
  void touch(int x, int y, bool pressed);
  void show(Page p);
  void message(const char *text);
  void returnHome();
  const std::vector<Hit> &targets() const { return hits; }
  Page page() const { return route; }
  const std::string *front() const { return commands.empty() ? nullptr : &commands.front(); }
  void pop() { commands.pop_front(); }
  unsigned displayErrors() const { return errorCount; }
  uint8_t displayError() const { return errorCode; }
  uint32_t lastContact() const { return contactAt; }
  bool overlay() const { return route != Page::Graph && route != Page::Preview; }
  int manualTarget() const { return manualTenths; }
  int editorProfile() const { return editingProfile; }
  bool empty() const { return commands.empty(); }
  void invalidate() { dirty = true; }

private:
  eepromValues_t &cfg;
  SensorState &sensors;
  Callback callback;
  Telemetry live;
  Page route = Page::Home, keyboardReturn = Page::Home, messageReturn = Page::Home;
  std::vector<Hit> hits;
  std::deque<std::string> commands;
  unsigned group = 0, section = 0, editingProfile = 0;
  int manualTenths = 0;
  bool dirty = true, wasBrewing = false, down = false, firstKey = true, nameDigits = false,
       scaleReadout = true, warmupDone = false;
  uint32_t lastDraw = 0, finishedAt = 0, contactAt = 0, clockNow = 0, heartbeatAt = 0, packetAt = 0;
  int ringProgress = -1;
  const Field *editingField = nullptr;
  bool editingSystem = false, renaming = false, uppercase = true;
  std::string input, popup;
  int confirmedAction = 0, confirmedArg = 0;
  uint8_t packet[16] = {};
  unsigned used = 0, needed = 0, errorCount = 0;
  uint8_t errorCode = 0;
  int pressX = 0, pressY = 0;
  void emit(const char *format, ...);
  void fill(int x, int y, int w, int h, int color);
  void text(const std::string &s, int x, int y, int w, int h, int font = 0, int color = 65535, int bg = -1,
            int align = 0);
  void button(const std::string &label, int x, int y, int w, int h, int action, int arg = 0,
              bool selected = false);
  void hit(const std::string &label, int x, int y, int w, int h, int action, int arg = 0);
  void render();
  void home(bool full);
  void header(const char *title, bool save = false);
  void step();
  void fields(bool system);
  void keyboard();
  void activate(int action, int arg);
  void edit(const Field &f, bool system);
  void confirm(const char *message, int action, int arg = 0);
  bool invoke(Command c, int arg = 0) { return callback ? callback(c, arg) : true; }
  eepromValues_t::profile_t &profile() { return cfg.profiles[editingProfile]; }
};
} // namespace Indigo
