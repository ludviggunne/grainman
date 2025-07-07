#ifndef GRAIN_H
#define GRAIN_H

struct grain {
  unsigned int length;
  unsigned int cursor;
  unsigned int offset;
  unsigned int cooldown;
  float        multiplier;
  int          reverse;
  float        gain;
};

#endif
