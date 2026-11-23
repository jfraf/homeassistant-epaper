// Parameter-Kombinationen für den Teilrefresh-Test (test/epaper-partial.yaml)
#pragma once
#include <cstdint>

namespace hd_test {

struct Combo {
  uint8_t t[6];  // T1-T4 Phasenzeiten, T5 WW-Nachtreiben, T6 BB-Nachtreiben (Frames)
  uint8_t vcom;  // 0x82 VCOM-DC: -0,1 V - 0,05 V * Wert
  uint8_t pll;   // 0x30 PLL: 0x3A = 100 Hz, 0x3C = 50 Hz
};

// Kombi 0 kommt aus den Substitutionen, 1..N aus dieser Liste.
static const Combo COMBOS[] = {
    {{20, 20, 60, 60, 10, 10}, 0x1A, 0x3A},  //  1 bisher bester Stand
    {{20, 20, 60, 60, 10, 10}, 0x12, 0x3A},  //  2 VCOM -1,0 V
    {{20, 20, 60, 60, 10, 10}, 0x22, 0x3A},  //  3 VCOM -1,8 V
    {{40, 40, 40, 40, 10, 10}, 0x1A, 0x3A},  //  4 ausgeglichen (vorher = nachher)
    {{60, 60, 60, 60, 10, 10}, 0x1A, 0x3A},  //  5 ausgeglichen, lang
    {{40, 40, 60, 60, 10, 10}, 0x1A, 0x3A},  //  6 längerer Ausgleich vorab
    {{20, 20, 60, 60, 20, 20}, 0x1A, 0x3A},  //  7 stärker nachtreiben
    {{20, 20, 60, 60, 30, 30}, 0x1A, 0x3A},  //  8 noch stärker nachtreiben
    {{20, 20, 90, 90, 10, 10}, 0x1A, 0x3A},  //  9 sehr langer Farbwechsel
    {{20, 20, 60, 60, 10, 10}, 0x1A, 0x3C},  // 10 50 Hz (alle Phasen doppelt so lang)
    {{40, 40, 40, 40, 10, 10}, 0x12, 0x3C},  // 11 ausgeglichen, VCOM -1,0 V, 50 Hz
    {{0, 0, 60, 60, 10, 10}, 0x1A, 0x3A},    // 12 ohne Ausgleichsphase
};

static const int NUM_COMBOS = 1 + sizeof(COMBOS) / sizeof(COMBOS[0]);

inline Combo combo(int i, int t1, int t2, int t3, int t4, int t5, int t6, int vcom, int pll) {
  if (i <= 0 || i >= NUM_COMBOS)
    return Combo{{(uint8_t) t1, (uint8_t) t2, (uint8_t) t3, (uint8_t) t4, (uint8_t) t5, (uint8_t) t6},
                 (uint8_t) vcom, (uint8_t) pll};
  return COMBOS[i - 1];
}

}  // namespace hd_test
