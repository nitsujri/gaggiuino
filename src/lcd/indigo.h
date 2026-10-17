#pragma once
#include "indigo/ui.h"
void indigoBegin(eepromValues_t &, SensorState &, Indigo::Ui::Callback);
bool indigoActive();
Indigo::Ui *indigoUi();
void indigoListen();
void indigoTick(const Indigo::Telemetry &);
