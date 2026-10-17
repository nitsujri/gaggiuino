#pragma once
#include "settings.h"
#include <stddef.h>
namespace Indigo {
enum class Kind : uint8_t { Boolean, Byte, U16, I32, Float, Name };
enum class Role : uint8_t { Value, Enable, Mode, Curve, Name };
struct Field {
  const char *key;
  const char *label;
  uint16_t offset;
  Kind kind;
  uint8_t group, bank;
  Role role;
};
extern const Field profileFields[];
extern const unsigned profileFieldCount;
extern const Field systemFields[];
extern const unsigned systemFieldCount;
double readField(const void *record, const Field &field);
bool writeField(void *record, const Field &field, double value);
} // namespace Indigo
