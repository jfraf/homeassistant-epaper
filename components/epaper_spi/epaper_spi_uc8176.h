#pragma once

#include "epaper_spi.h"

#include <vector>

namespace esphome::epaper_spi {

/**
 * UC8176 (IL0398) monochrome e-paper controller (e.g. Good Display GDEW042M01,
 * 400x300).
 *
 * Buffer: 1 bit per pixel, single plane (bit 0 = black/ink, bit 1 = white/paper,
 * per the base is_on() convention), written un-inverted to NEW RAM (0x13).
 *
 * Full refresh writes OLD (0x10) = 0xFF and uses the OTP waveform. Partial
 * refresh runs a panel-specific "partial init" sequence (power / booster / PLL /
 * VCOM-DC and the register LUTs 0x20-0x24), sets a partial window (0x91/0x90),
 * and drives a differential update from the previous frame (software old_data_).
 *
 * BUSY is active-LOW; the driver flags that itself (busy_invert_).
 */
class EPaperUC8176 : public EPaperBase {
 public:
  EPaperUC8176(const char *name, uint16_t width, uint16_t height, const uint8_t *init_sequence,
               size_t init_sequence_length, const uint8_t *partial_sequence, size_t partial_sequence_length)
      : EPaperBase(name, width, height, init_sequence, init_sequence_length, DISPLAY_TYPE_BINARY),
        partial_sequence_(partial_sequence),
        partial_sequence_length_(partial_sequence_length) {
    this->buffer_length_ = this->row_width_ * height;  // single 1-bpp plane
    this->busy_invert_ = true;                         // UC8176 BUSY is active-LOW
  }

 // Lokale Ergänzung (homeassistant-epaper): Teilrefresh-Waveform zur Laufzeit setzen, für Abstimmungstests.
  // Pegel wie GDEW042M01, Phasenzeiten T1-T4 für alle Tabellen, T5 = WW-Nachtreiben, T6 = BB-Nachtreiben.
  void set_partial_waveform(uint8_t t1, uint8_t t2, uint8_t t3, uint8_t t4, uint8_t t5, uint8_t t6, uint8_t vcom_dc,
                            uint8_t pll);
  // Nächstes Update als Voll-Refresh ausführen.
  void force_full_update() { this->update_count_ = 0; }
  bool next_update_is_full() const { return this->update_count_ == 0; }
  // Nächstes Teil-Update einmalig als "Einmal-Blinken" ausführen (siehe set_partial_options, flash).
  void request_flash() { this->flash_once_ = true; }
  // Optionen für Teilrefreshes:
  //   passes    Anzahl Durchgänge (1 = normal; weitere Durchgänge treiben alle Pixel nochmals in ihre Sollfarbe)
  //   flash     "Einmal-Blinken": als altes Bild wird das Negativ gesendet, jeder Pixel wird komplett neu
  //             geschrieben (Bildschirm invertiert genau einmal)
  //   windowed  nur das umschließende Rechteck der geänderten Pixel ansteuern
  //   hold_ms   Treiberspannung nach dem letzten Durchgang noch so lange anlassen
  void set_partial_options(uint8_t passes, bool flash, bool windowed, uint16_t hold_ms) {
    this->passes_ = passes < 1 ? 1 : passes;
    this->flash_ = flash;
    this->windowed_ = windowed;
    this->hold_ms_ = hold_ms;
  }

 protected:
  void setup() override;
  bool initialise(bool partial) override;
  bool transfer_data() override;
  void power_on() override;
  void refresh_screen(bool partial) override;
  void power_off() override;
  void deep_sleep() override;
  bool before_power_off_() override;
  void compute_window_();

  const uint8_t *partial_sequence_;
  size_t partial_sequence_length_;
  std::vector<uint8_t> partial_override_;
  uint8_t *old_data_{nullptr};
  bool partial_{false};
  uint8_t passes_{1};
  bool flash_{false};
  bool flash_once_{false};
  bool flash_now_{false};  // gilt für das laufende Update
  bool windowed_{false};
  uint16_t hold_ms_{0};
  uint8_t passes_left_{0};
  bool resend_{false};  // weiterer Durchgang im selben Fenster
  bool held_{false};
  // aktuelles Fenster in Bytes (x) bzw. Zeilen (y)
  uint16_t win_x_{0}, win_y_{0}, win_w_{0}, win_h_{0};
};

}  // namespace esphome::epaper_spi
