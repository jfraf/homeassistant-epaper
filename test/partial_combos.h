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
// Runde 2: Favoriten aus Runde 1 waren 2 (VCOM 0x12), 5 (ausgeglichen 60) und 11 (ausgeglichen, 0x12, 50 Hz).
// Runde 1 (zum Nachschlagen): 1 20/20/60/60/10/10 1A 3A · 2 dto. VCOM 12 · 3 dto. VCOM 22 · 4 40x4 ·
// 5 60x4 · 6 40/40/60/60 · 7 Nachtreiben 20 · 8 Nachtreiben 30 · 9 20/20/90/90 · 10 50 Hz ·
// 11 40x4 VCOM 12 50 Hz · 12 0/0/60/60
static const Combo COMBOS[] = {
    {{60, 60, 60, 60, 10, 10}, 0x12, 0x3A},  //  1 Favorit 5 + VCOM von 2
    {{60, 60, 60, 60, 10, 10}, 0x0E, 0x3A},  //  2 VCOM -0,8 V
    {{60, 60, 60, 60, 10, 10}, 0x16, 0x3A},  //  3 VCOM -1,2 V
    {{50, 50, 50, 50, 10, 10}, 0x12, 0x3A},  //  4 etwas kürzer
    {{80, 80, 80, 80, 10, 10}, 0x12, 0x3A},  //  5 länger
    {{60, 60, 60, 60, 5, 5}, 0x12, 0x3A},    //  6 schwächer nachtreiben
    {{60, 60, 60, 60, 20, 20}, 0x12, 0x3A},  //  7 stärker nachtreiben
    {{30, 30, 30, 30, 10, 10}, 0x12, 0x3C},  //  8 wie 1, aber 50 Hz (gleiche Dauer)
    {{40, 40, 40, 40, 10, 10}, 0x12, 0x3C},  //  9 = Favorit 11 aus Runde 1
    {{60, 60, 60, 60, 10, 10}, 0x1A, 0x3A},  // 10 = Favorit 5 aus Runde 1
    {{20, 20, 60, 60, 10, 10}, 0x12, 0x3A},  // 11 = Favorit 2 aus Runde 1
};

static const int NUM_COMBOS = 1 + sizeof(COMBOS) / sizeof(COMBOS[0]);

inline Combo combo(int i, int t1, int t2, int t3, int t4, int t5, int t6, int vcom, int pll) {
  if (i <= 0 || i >= NUM_COMBOS)
    return Combo{{(uint8_t) t1, (uint8_t) t2, (uint8_t) t3, (uint8_t) t4, (uint8_t) t5, (uint8_t) t6},
                 (uint8_t) vcom, (uint8_t) pll};
  return COMBOS[i - 1];
}

// Refresh-Modi für Teilrefreshes (Taste 3)
struct Mode {
  const char *name;
  uint8_t passes;    // Anzahl Durchgänge
  bool flash;        // "Einmal-Blinken": jeder Pixel wird komplett neu geschrieben
  bool windowed;     // nur das Rechteck um die geänderten Pixel ansteuern
  uint16_t hold_ms;  // Spannung nach dem Refresh noch halten
};

static const Mode MODES[] = {
    {"A einfach", 1, false, false, 0},
    {"B doppelt", 2, false, false, 0},
    {"C dreifach", 3, false, false, 0},
    {"D Einmal-Blinken", 1, true, false, 0},
    {"E Blinken + doppelt", 2, true, false, 0},
    {"F Fenster", 1, false, true, 0},
    {"G Fenster doppelt", 2, false, true, 0},
    {"H halten 2 s", 1, false, false, 2000},
};
static const int NUM_MODES = sizeof(MODES) / sizeof(MODES[0]);

}  // namespace hd_test
