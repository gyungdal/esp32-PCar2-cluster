#ifndef __UTILS_HPP__
#define __UTILS_HPP__

#include <inttypes.h>

template <typename T>
double kmHToHz(T speed) {
  // 200 = 50hz
  if (speed <= 60) {
    speed = 60;
  }
  speed -= 60;
  int64_t speedHzOutput = map(static_cast<int64_t>(speed), 0, 180, 0, 125) + 40;
  return speedHzOutput;
}

template <typename T>
double RPMToHz(T rpm) {
  // 1000 = 30.9hz
  return static_cast<double>(rpm) / 30;
}

#endif