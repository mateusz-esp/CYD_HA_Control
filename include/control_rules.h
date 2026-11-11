#pragma once
#include <math.h>

namespace ControlRules {
inline bool validRange(float minimum, float maximum, float step) {
  return isfinite(minimum) && isfinite(maximum) && isfinite(step) &&
         minimum < maximum && step > 0 && step <= maximum - minimum;
}
inline float adjust(float value, int direction, float minimum, float maximum, float step) {
  if (!validRange(minimum, maximum, step) || !isfinite(value)) return NAN;
  const float index = roundf((value - minimum) / step) + direction;
  return fmaxf(minimum, fminf(maximum, minimum + index * step));
}
inline bool canSave(float value, float minimum, float maximum, float step) {
  if (!validRange(minimum, maximum, step) || !isfinite(value) ||
      value < minimum || value > maximum) return false;
  const float index = (value - minimum) / step;
  return fabsf(index - roundf(index)) <= 0.001f;
}
}
