# homeassistant-epaper

E-Paper-Dashboard für Home Assistant auf Basis von ESPHome. Die Hardware stammt aus dem früheren Projekt [fritzbox-calllist](https://github.com/jfraf/fritzbox-calllist).

## Hardware

- LOLIN D32 (ESP32-WROOM-32)
- Waveshare 4.2" e-Paper-Modul V1 (UC8176, 400×300, s/w)
- 4 Taster nach +3V3 (interne Pulldowns)

| Funktion | GPIO |
|---|---|
| e-Paper BUSY / RST / DC / CS | 4 / 16 / 17 / 5 |
| e-Paper CLK / DIN | 18 / 23 |
| Taste 1 (links) | 12 (Strapping-Pin, beim Booten nicht drücken) |
| Taste 2 | 14 |
| Taste 3 | 13 |
| Taste 4 (rechts) | 15 (Strapping-Pin) |
| 2 × LED APA106-F8 / P9823 (Data) | 27 (Versorgung 5 V) |

## Konfigurationen

- `haus-display.yaml` + `haus_display.h`: Übersichtsseite (hochkant, 300×400) mit
  - Fenster & Türen (nur offene / gekippte / unklare Räume)
  - Garage und Auto (Tore, Türen, Fenster offen?)
  - Waschmaschine (Programm, Fortschritt, Restlaufzeit; nur wenn erreichbar)
  - Auto: Ladestand, Reichweite, Ladelimit, Lademodus (openWB), Ladeleistung
  - PV: aktuelle Leistung, Ertrag heute, Restprognose (Forecast.Solar, 2 Flächen) bzw. Prognose morgen
  - zwei RGB-LEDs als Lichter `LED 1` / `LED 2` in Home Assistant
- `test/led-apa106/`: Arduino-Nano-Testprogramm für die LEDs
- `components/epaper_spi/`: Kopie von [parkghost/esphome-epaper](https://github.com/parkghost/esphome-epaper) (Commit `cf145a7`, Apache-2.0), ergänzt um die Modelle `waveshare-4.2in-v1` und `waveshare-4.2in-v1-flash` mit der Teilrefresh-Waveform aus [GxEPD2](https://github.com/ZinggJM/GxEPD2) (`models/uc8176.py`)
- `test/epaper-partial.yaml`: Teilrefresh-Test mit diesem `epaper_spi`, Modell per Substitution `model` wählbar; `test/secrets.yaml` ist ein Symlink auf `../secrets.yaml`
- `mockup/dashboard.html`: HTML-Skizze der Übersichtsseite

Für das ESPHome-Dashboard muss `haus_display.h` neben der YAML liegen. Schrift: Arimo (Google Fonts), normal und fett.

Sicherheitsrelevante Zustände (Fenster, Türen, Garage, Auto, Waschmaschinenstatus) aktualisieren das Display sofort.
Langsam veränderliche Werte (PV, Akkustand, Restlaufzeit …) werden alle 5 Minuten geprüft und lösen nur dann einen
Refresh aus, wenn sich der angezeigte Wert ändert. Das Panel kann in ESPHome nur vollständig aktualisiert werden.

## Build & Flash

```sh
python3 -m venv .venv && .venv/bin/pip install esphome
cp secrets.yaml.example secrets.yaml   # ausfüllen; API-Key: openssl rand -base64 32
.venv/bin/esphome compile haus-display.yaml
```

Beim ersten Mal `.esphome/build/haus-display/build/firmware.factory.bin` per USB flashen (z. B. über https://web.esphome.io). Danach geht es per OTA: `.venv/bin/esphome run haus-display.yaml`.
