#include "indigo.h"
#include "lcd.h"
#include "log.h"
#include "pindef.h"
static_assert(SERIAL_TX_BUFFER_SIZE >= 195,
              "Indigo commands must fit the nonblocking UART buffer");
namespace {
Indigo::Ui *ui = nullptr;
}
void indigoBegin(eepromValues_t &cfg, SensorState &sensor,
                 Indigo::Ui::Callback cb) {
#ifdef INDIGO_UI
  static Indigo::Ui instance(cfg, sensor, cb);
  ui = &instance;
  ui->begin();
#endif
}
bool indigoActive() { return ui != nullptr; }
Indigo::Ui *indigoUi() { return ui; }
void indigoListen() {
  if (!ui)
    return;
  while (USART_LCD.available())
    ui->receive(USART_LCD.read());
  static unsigned reported = 0;
  if (ui->displayErrors() != reported) {
    reported = ui->displayErrors();
    LOG_ERROR("Indigo display command error: 0x%02X (count %u)",
              ui->displayError(), reported);
  }
  using Indigo::Page;
  const auto page = ui->page();
  lcdCurrentPageId = page == Page::Home        ? NextionPage::Home
                     : page == Page::Graph     ? NextionPage::BrewGraph
                     : page == Page::Preview   ? NextionPage::GraphPreview
                     : page == Page::Manual    ? NextionPage::BrewManual
                     : page == Page::Backflush ? NextionPage::Flush
                     : page == Page::Descale   ? NextionPage::Descale
                                               : NextionPage::SettingsSystem;
  lcdLastCurrentPageId = lcdCurrentPageId;
}
void indigoTick(const Indigo::Telemetry &data) {
  if (!ui)
    return;
  ui->tick(millis(), data);
  // Never wait for display TX space in the machine's control/safety loop.
  for (unsigned i = 0; i < 2; i++) {
    const auto *s = ui->front();
    if (!s || USART_LCD.availableForWrite() < static_cast<int>(s->size() + 3))
      break;
    USART_LCD.write(reinterpret_cast<const uint8_t *>(s->data()), s->size());
    const uint8_t end[3] = {255, 255, 255};
    USART_LCD.write(end, 3);
    ui->pop();
  }
}
