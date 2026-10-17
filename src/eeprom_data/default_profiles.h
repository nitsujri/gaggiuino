/* 09:32 15/03/2023 - change triggering comment */
#ifndef DEFAULT_PROFILES_H
#define DEFAULT_PROFILES_H

#include <Arduino.h>
#include "eeprom_data.h"

namespace {
  using profileDefaults_t=eepromValues_t::profile_t;

  // DONE:: Fully customised default profiles in line wiuth the names
  const profileDefaults_t defaultsProfile[MAX_PROFILES] = {
    {"Turbo",/*pi*/false, false, 0, 0.f, 0.f, 0, 0.f, 0.f, 0.f, false, 0.f,/*sk*/false, 0, 0, 0.f, 0.f, 0.f, 0.f, 0.f, 0, 0,/*tp*/false, false, 0.f, 0.f, 0, 0.f, 0, 0, 0.f, 0.f, 0.f, 0, 0.f, 0, 0, 0.f,/*pf*/true, false, 6.f, 6.f, 0, 0, 6.f, 0.f, 0.f, 0, 0, 0.f,/*other*/ 92, true, 15.f, 45.f, 3},  // profile 0
    {"SOUP",/*pi*/true,true,0,0.f,2.5f,2,1.f,0.f,0.f,false,0.f,/*sk*/true,8,8,1.f,2.5f,0.f,0.f,0.f,0,4,/*tp*/false,false,0.f,0.f,0,0.f,0,0,0.f,0.f,0.f,0,0.f,0,0,0.f,/*pf*/true,true,0.f,0.f,0,0,0.f,6.f,6.f,0,3,1.f,/*other*/86,true,17.f,0.f,3}, // profile 1
    {"Adaptive",/*pi*/true, true, 0, 0.f, 7.f, 20, 3.f, 0.f, 600, true, 0.f,/*sk*/true, 0, 6, 3.f, 0.f, 0.f, 0.f, 0.f, 0, 3,/*tp*/true, false, 0.f, 9.f, 0, 0.f, 6, 0, 0.f, 0.f, 2.f, 0, 9.f, 6, 0, 9.f,/*pf*/true, true, 0.f, 0.f, 30, 0, 3.f, 0.f, 2.5f, 30, 0, 9.f,/*other*/ 95, true, 18.f, 0.f, 2},  // profile 2
    {"Filter 2.1",/*pi*/true, true, 0, 0.f, 4.f, 15, 1.f, 0.f, 600, true, 0.f,/*sk*/true, 0, 90, 0.f, 0.2f, 0.f, 0.f, 45.f, 0, 0,/*tp*/true, true, 0.f, 0.f, 0, 0.f, 0, 0, 0.f, 0.2f, 3.f, 0, 0.f, 10, 0, 9.f,/*pf*/true, true, 0.f, 0.f, 0, 0, 0.f, 3.f, 3.f, 5, 0, 9.f,/*other*/ 89, true, 18.f, 0.f, 2},  // profile 3
    {"Blooming espresso",/*pi*/true, true, 0, 0.f, 4.f, 20, 7.f, 0.f, 650, true, 0.f,/*sk*/true, 0, 30, 0.f, 0.f, 0.6f, 0.f, 5.f, 5, 2,/*tp*/true, true, 0.f, 0.f, 0, 0.f, 0, 0, 0.f, 2.f, 2.f, 1, 9.f, 0, 4, 9.f,/*pf*/true, true, 0.f, 0.f, 0, 0, 0.f, 2.f, 2.f, 15, 0, 6.f,/*other*/ 93, true, 18.f, 0.f, 2}  // profile 4
  };
}

#endif
