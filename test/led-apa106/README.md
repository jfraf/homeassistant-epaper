# LED-Test APA106-F8 / P9823 (Arduino Nano)

Interaktiver Test einer einzelnen LED: Farben, Helligkeit, Farbreihenfolge.

## Verkabelung

| LED-Bein | Nano |
|---|---|
| VDD (+5 V) | 5V |
| GND | GND |
| DIN | D6 |
| DOUT | frei |

Bein-Erkennung (Anschlussreihenfolge 1–4: DIN, VDD, GND, DOUT): Das **längste Bein ist GND**, das zweitlängste VDD.
Das kurze Bein neben VDD ist DIN, das kurze neben GND ist DOUT. Vor dem Einschalten mit dem Datenblatt des Händlers
abgleichen – Verpolung zerstört die LED.

## Mac (Homebrew)

```sh
brew install arduino-cli
arduino-cli core update-index
arduino-cli core install arduino:avr
arduino-cli lib install "Adafruit NeoPixel"

arduino-cli board list                       # Port finden, z. B. /dev/cu.usbserial-1420
arduino-cli compile --fqbn arduino:avr:nano test/led-apa106
arduino-cli upload  --fqbn arduino:avr:nano -p /dev/cu.usbserial-1420 test/led-apa106
arduino-cli monitor -p /dev/cu.usbserial-1420 -c baudrate=115200
```

Nano-Klone mit altem Bootloader (Upload-Fehler `not in sync`): FQBN `arduino:avr:nano:cpu=atmega328old`.

## Bedienung

Im Monitor Befehle eingeben und mit Enter abschicken (`?` zeigt die Hilfe):
`rot`, `gruen`, `blau`, `weiss`, `warmweiss` …, `c 255 80 0`, `h 10`, `+`, `-`, `demo`, `regenbogen`, `fade`,
`blink`, `order`, `gamma`, `aus`.
