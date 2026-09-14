#ifndef ARDUINO_H
#define ARDUINO_H

#define ARDUINO 100

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <algorithm>

#include "Print.h"

typedef uint8_t boolean;
typedef uint8_t byte;

// --- DUMMY ARDUINO STRING ---
class __FlashStringHelper;
class String {
public:
    String() {}
    String(const char* s) {}
    size_t length() const { return 0; }
    const char* c_str() const { return ""; }
    char operator[](unsigned int index) const { return 0; }
};

// --- PROGMEM & PGM MACROS ---
#define PROGMEM
#define pgm_read_byte(addr)    (*(const uint8_t *)(addr))
#define pgm_read_word(addr)    (*(const uint16_t *)(addr))
#define pgm_read_dword(addr)   (*(const uint32_t *)(addr))

#ifndef pgm_read_pointer
#define pgm_read_pointer(addr) ((void *)pgm_read_dword(addr))
#endif

// --- MATH UTILS ARDUINO ---
#define DEG_TO_RAD 0.017453292519943295769236907684886
#define RAD_TO_DEG 57.295779513082320876798154814105

inline float radians(float deg) { return deg * DEG_TO_RAD; }
inline float degrees(float rad) { return rad * RAD_TO_DEG; }
inline float sq(float x) { return x * x; }

using std::min;
using std::max;

#endif