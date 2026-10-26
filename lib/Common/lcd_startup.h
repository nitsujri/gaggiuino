#ifndef LCD_STARTUP_H
#define LCD_STARTUP_H

// The stock splash owns its countdown and build label. Verify that the display
// has accepted the label before allowing that countdown to leave the page.
// Querying the live field works on both cold boot and an MCU-only reset; a
// power-on banner can be lost while the serial library initializes its buffer.
template <typename Display, typename Wait>
void initializeLcdSplash(Display &display, const char *buildVersion, Wait wait) {
  display.begin(115200);
  // Reset retained UI modes as well when only the controller was restarted.
  // Let reset take effect before querying; slow cold boots are retried below.
  display.writeStr("rest");
  wait(500);
  for (;;) {
    display.writeStr("page splash");
    display.writeNum("splash.tm0.en", 0);
    display.writeNum("splash.loadBar.val", 0);
    display.writeStr("splash.build_version.txt", buildVersion);
    if (display.readStr("splash.build_version.txt") == buildVersion) break;
    // The library's read has a timeout. Retry if the display is still booting
    // or the first write was lost, while setup keeps the outputs switched off.
    wait(50);
  }
  display.writeNum("splash.tm0.en", 1);
}

#endif
