#include "epaper_spi_uc8176.h"

#include <algorithm>

#include "esphome/core/helpers.h"
#include "esphome/core/log.h"

namespace esphome::epaper_spi {

void EPaperUC8176::setup() {
  EPaperBase::setup();
  if (this->is_failed())
    return;
  // Only allocate the software previous-frame copy when partial updates are
  // enabled; skipping it saves a full framebuffer of RAM otherwise.
  if (this->is_using_partial_update_()) {
    RAMAllocator<uint8_t> allocator;
    this->old_data_ = allocator.allocate(this->buffer_length_);
    if (this->old_data_ == nullptr) {
      this->mark_failed(LOG_STR("Failed to allocate old data buffer"));
    }
  }
}

bool EPaperUC8176::initialise(bool partial) {
  this->partial_ = partial;
  this->passes_left_ = partial ? this->passes_ - 1 : 0;
  this->resend_ = false;
  this->held_ = false;
  this->current_data_index_ = 0;
  if (!EPaperBase::initialise(partial)) {  // declarative full init (panel/resolution/VCOM)
    return false;
  }
  if (partial) {
    // Partial init: power/booster/PLL/VCOM-DC + register LUTs (0x20-0x24).
    if (!this->partial_override_.empty()) {
      this->send_init_sequence_(this->partial_override_.data(), this->partial_override_.size());
    } else {
      this->send_init_sequence_(this->partial_sequence_, this->partial_sequence_length_);
    }
  }
  return true;
}

void EPaperUC8176::compute_window_() {
  this->win_x_ = 0;
  this->win_y_ = 0;
  this->win_w_ = this->row_width_;
  this->win_h_ = this->height_;
  if (!this->partial_ || !this->windowed_ || this->flash_ || this->old_data_ == nullptr)
    return;
  // umschließendes Rechteck aller geänderten Bytes
  uint16_t x0 = this->row_width_, x1 = 0, y0 = this->height_, y1 = 0;
  for (uint16_t y = 0; y < this->height_; y++) {
    const uint8_t *o = &this->old_data_[(size_t) y * this->row_width_];
    const uint8_t *n = &this->buffer_[(size_t) y * this->row_width_];
    for (uint16_t x = 0; x < this->row_width_; x++) {
      if (o[x] != n[x]) {
        x0 = std::min(x0, x);
        x1 = std::max(x1, x);
        y0 = std::min(y0, y);
        y1 = y;
      }
    }
  }
  if (x1 < x0)
    return;  // nichts geändert: ganzes Bild
  this->win_x_ = x0;
  this->win_y_ = y0;
  this->win_w_ = x1 - x0 + 1;
  this->win_h_ = y1 - y0 + 1;
}

bool HOT EPaperUC8176::transfer_data() {
  const uint32_t start_time = millis();
  uint8_t bytes_to_send[MAX_TRANSFER_SIZE];

  if (this->current_data_index_ == 0 && !this->resend_) {
    this->compute_window_();
    if (this->partial_) {
      // Partial: enter partial mode and set the window before the data.
      const uint16_t xs = this->win_x_ * 8;
      const uint16_t xe = (this->win_x_ + this->win_w_) * 8 - 1;
      const uint16_t ys = this->win_y_;
      const uint16_t ye = this->win_y_ + this->win_h_ - 1;
      this->command(0x91);  // PARTIAL IN
      this->cmd_data(0x90, {(uint8_t) (xs >> 8), (uint8_t) xs, (uint8_t) (xe >> 8), (uint8_t) xe, (uint8_t) (ys >> 8),
                            (uint8_t) ys, (uint8_t) (ye >> 8), (uint8_t) ye, 0x01});
      ESP_LOGD("epaper_spi.uc8176", "Partial window x %u-%u, y %u-%u%s", xs, xe, ys, ye, this->flash_ ? " (flash)" : "");
    }
  }
  const size_t frame = (size_t) this->win_w_ * this->win_h_;
  // Position im Fensterstrom -> Offset im Bildpuffer, Länge bis Zeilenende
  auto locate = [this](size_t i, size_t &off, size_t &row_left) {
    const size_t row = i / this->win_w_, col = i % this->win_w_;
    off = (size_t) (this->win_y_ + row) * this->row_width_ + this->win_x_ + col;
    row_left = this->win_w_ - col;
  };

  // Phase 1: OLD data (0x10). Full -> 0xFF, partial -> previous frame, flash -> negative of the new frame.
  if (this->current_data_index_ < frame) {
    if (this->current_data_index_ == 0) {
      this->command(0x10);
    }
    this->start_data_();
    while (this->current_data_index_ < frame) {
      size_t off, row_left;
      locate(this->current_data_index_, off, row_left);
      const size_t n = std::min(MAX_TRANSFER_SIZE, row_left);
      if (!this->partial_) {
        std::fill_n(bytes_to_send, n, (uint8_t) 0xFF);
      } else if (this->flash_ && !this->resend_) {
        for (size_t i = 0; i < n; i++)
          bytes_to_send[i] = ~this->buffer_[off + i];
      } else {
        std::copy_n(&this->old_data_[off], n, bytes_to_send);
      }
      this->write_array(bytes_to_send, n);
      this->current_data_index_ += n;
      if (millis() - start_time > MAX_TRANSFER_TIME) {
        this->disable();
        return false;
      }
    }
    this->disable();
  }

  // Phase 2: NEW data (0x13) = framebuffer (un-inverted). Snapshot into old_data_.
  if (this->current_data_index_ < 2 * frame) {
    if (this->current_data_index_ == frame) {
      this->command(0x13);
    }
    this->start_data_();
    while (this->current_data_index_ < 2 * frame) {
      size_t off, row_left;
      locate(this->current_data_index_ - frame, off, row_left);
      const size_t n = std::min(MAX_TRANSFER_SIZE, row_left);
      for (size_t i = 0; i < n; i++) {
        const uint8_t b = this->buffer_[off + i];
        bytes_to_send[i] = b;
        if (this->old_data_ != nullptr)
          this->old_data_[off + i] = b;
      }
      this->write_array(bytes_to_send, n);
      this->current_data_index_ += n;
      if (millis() - start_time > MAX_TRANSFER_TIME) {
        this->disable();
        return false;
      }
    }
    this->disable();
  }

  this->current_data_index_ = 0;
  return true;
}

// Nach Ende eines Teilrefreshs: weitere Durchgänge anhängen bzw. Spannung halten.
bool EPaperUC8176::before_power_off_() {
  if (!this->partial_)
    return true;
  if (this->passes_left_ > 0) {
    // altes Bild == neues Bild: alle Pixel im Fenster werden nur noch in ihre Sollfarbe nachgetrieben
    this->resend_ = true;
    const bool done = this->transfer_data();
    if (!done)
      return false;
    this->resend_ = false;
    this->passes_left_--;
    this->command(0x12);  // DISPLAY REFRESH
    this->next_delay_ = 2;  // BUSY braucht min. 200 µs
    return false;
  }
  if (this->hold_ms_ > 0 && !this->held_) {
    this->held_ = true;
    this->next_delay_ = this->hold_ms_;
    return false;
  }
  return true;
}

void EPaperUC8176::set_partial_waveform(uint8_t t1, uint8_t t2, uint8_t t3, uint8_t t4, uint8_t t5, uint8_t t6,
                                         uint8_t vcom_dc, uint8_t pll) {
  auto &q = this->partial_override_;
  q.clear();
  // Format wie die Init-Sequenzen: Befehl, Anzahl Datenbytes, Daten
  auto cmd = [&q](uint8_t c, std::initializer_list<uint8_t> d) {
    q.push_back(c);
    q.push_back((uint8_t) d.size());
    q.insert(q.end(), d.begin(), d.end());
  };
  auto lut = [&q](uint8_t c, size_t size, uint8_t level, uint8_t last) {
    q.push_back(c);
    q.push_back((uint8_t) size);
    const size_t start = q.size();
    q.insert(q.end(), {level, 0, 0, 0, last, 0x01});
    q.resize(start + size, 0x00);
    return start;
  };
  cmd(0x01, {0x03, 0x00, 0x2B, 0x2B});  // power setting
  cmd(0x06, {0x17, 0x17, 0x17});        // booster soft start
  cmd(0x00, {0x3F});                    // panel setting (LUT from register)
  cmd(0x30, {pll});                     // PLL
  cmd(0x61, {(uint8_t) (this->width_ >> 8), (uint8_t) this->width_, (uint8_t) (this->height_ >> 8),
             (uint8_t) this->height_});  // TRES
  cmd(0x82, {vcom_dc});                  // VCOM DC
  cmd(0x50, {0xD7});                     // VCOM/interval, border floating
  const size_t starts[] = {lut(0x20, 44, 0x00, t4), lut(0x21, 42, 0x02, t5), lut(0x22, 42, 0x5A, t4),
                           lut(0x23, 42, 0x84, t4), lut(0x24, 42, 0x01, t6)};
  for (size_t s : starts) {
    q[s + 1] = t1;
    q[s + 2] = t2;
    q[s + 3] = t3;
  }
  ESP_LOGI("epaper_spi.uc8176", "Partial waveform: T %u/%u/%u/%u/%u/%u VCOM 0x%02X PLL 0x%02X", t1, t2, t3, t4, t5, t6,
           vcom_dc, pll);
}

void EPaperUC8176::power_on() {
  this->command(0x04);  // POWER ON
}

void EPaperUC8176::refresh_screen(bool /*partial*/) {
  this->command(0x12);  // DISPLAY REFRESH
}

void EPaperUC8176::power_off() {
  if (this->partial_) {
    this->command(0x92);  // PARTIAL OUT (after the refresh completed)
  }
  this->command(0x02);  // POWER OFF
}

void EPaperUC8176::deep_sleep() {
  this->cmd_data(0x07, {0xA5});  // DEEP SLEEP with check code
}

}  // namespace esphome::epaper_spi
