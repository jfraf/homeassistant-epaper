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

## Konfigurationen

- `fenster-display.yaml`: PoC, der den Zustand aller Fenster und der Haustür nach Stockwerken anzeigt (offen / gekippt / zu).

## Build & Flash

```sh
python3 -m venv .venv && .venv/bin/pip install esphome
cp secrets.yaml.example secrets.yaml   # ausfüllen; API-Key: openssl rand -base64 32
.venv/bin/esphome compile fenster-display.yaml
```

Beim ersten Mal `.esphome/build/fenster-display/build/firmware.factory.bin` per USB flashen (z. B. über https://web.esphome.io). Danach geht es per OTA: `.venv/bin/esphome run fenster-display.yaml`.
