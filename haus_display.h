// Hilfsfunktionen für haus-display.yaml (ohne Abhängigkeit von ESPHome-IDs)
#pragma once
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>

namespace hd {

// ISO-8601-Zeitstempel ("2026-11-21T10:35:47+00:00", auch mit "Z" oder ohne Offset) -> Unix-Epoch (UTC).
// Rückgabe -1 bei Fehler.
inline int64_t parse_iso_utc(const std::string &s) {
  int Y, M, D, h, m, sec = 0, oh = 0, om = 0;
  char sign = '+';
  int n = sscanf(s.c_str(), "%d-%d-%dT%d:%d:%d%c%d:%d", &Y, &M, &D, &h, &m, &sec, &sign, &oh, &om);
  if (n < 5) return -1;
  // days_from_civil (H. Hinnant)
  Y -= M <= 2;
  const int era = (Y >= 0 ? Y : Y - 399) / 400;
  const unsigned yoe = (unsigned) (Y - era * 400);
  const unsigned doy = (153 * (M + (M > 2 ? -3 : 9)) + 2) / 5 + D - 1;
  const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  const int64_t days = (int64_t) era * 146097 + (int64_t) doe - 719468;
  int64_t t = days * 86400 + h * 3600 + m * 60 + sec;
  if (n >= 9 && (sign == '+' || sign == '-')) {
    const int off = (oh * 60 + om) * 60;
    t += (sign == '-') ? off : -off;
  }
  return t;
}

// Letztes Segment eines Home-Connect-Enums ("BSH.Common.EnumType.OperationState.Run" -> "Run")
inline std::string last_segment(const std::string &s) {
  const auto i = s.rfind('.');
  return i == std::string::npos ? s : s.substr(i + 1);
}

// Home-Connect-Programmschlüssel -> deutscher Kurzname
inline std::string wm_program_name(const std::string &p) {
  struct Map { const char *key; const char *name; };
  static const Map MAP[] = {
      {"Eco4060", "Eco 40-60"},          {"Super153045", "Super 15/30"},
      {"Cotton", "Baumwolle"},           {"EasyCare", "Pflegeleicht"},
      {"DelicatesSilk", "Fein/Seide"},   {"Wool", "Wolle"},
      {"DrumClean", "Trommelreinigung"}, {"DarkWash", "Dunkle Wäsche"},
      {"ShirtsBlouses", "Hemden/Blusen"}, {"WaterProof", "Outdoor"},
      {"Mix", "Mix"},
  };
  for (const auto &e : MAP)
    if (p.find(e.key) != std::string::npos) return e.name;
  return p.empty() ? "" : last_segment(p);
}

// Zahl mit einer Nachkommastelle und deutschem Dezimalkomma
inline std::string fmt1(float v) {
  char b[16];
  snprintf(b, sizeof(b), "%.1f", v);
  for (char *c = b; *c; c++)
    if (*c == '.') *c = ',';
  return b;
}

inline bool ha_missing(const std::string &s) { return s.empty() || s == "unavailable" || s == "unknown"; }

}  // namespace hd
