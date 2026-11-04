#ifndef LCD_BOILER_FILL_H
#define LCD_BOILER_FILL_H

// The stock popup's local tm0 returns to the previous page after one second.
// Filling owns its lifetime instead; the popup itself reports no page change.
class BoilerFillPopup {
  bool visible = false;
public:
  template <typename Display>
  void update(Display &display, bool pumping, bool onHome) {
    if (pumping && onHome) {
      if (visible) return;
      display.writeStr("popupMSG.t0.txt", "Filling boiler!");
      display.writeStr("page popupMSG");
      // tm0 is local to the popup: enter the page before addressing its timer.
      display.writeNum("tm0.en", 0);
      visible = true;
    } else if (visible) {
      // A genuine navigation event must keep its destination page.
      if (onHome) {
        display.writeNum("tm0.en", 1);
        display.writeStr("page home");
      }
      visible = false;
    }
  }
};

#endif
