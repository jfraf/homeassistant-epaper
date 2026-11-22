// Test für APA106-F8 / P9823 LEDs (WS2812-kompatibel) am Arduino Nano, hier zwei LEDs in Reihe.
//
// Verkabelung (ohne Zusatzbauteile):
//   LED 1 VDD  -> Nano 5V          LED 2 VDD  -> Nano 5V
//   LED 1 GND  -> Nano GND         LED 2 GND  -> Nano GND
//   LED 1 DIN  -> Nano D6          LED 2 DIN  -> LED 1 DOUT
//                                  LED 2 DOUT -> frei lassen
//
// Bedienung über den seriellen Monitor (115200 Baud, Zeilenende NL/CR). "?" zeigt die Hilfe.
// Nach dem Start läuft einmal der Farbreihenfolge-Test für jede LED, danach die Demo.

#include <Adafruit_NeoPixel.h>

#define LED_PIN 6
#define NUM_LEDS 2
// APA106/P9823 erwarten meist RGB-Reihenfolge. Zeigt der Test "rot" grün an, NEO_GRB eintragen.
#define LED_ORDER NEO_RGB

Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, LED_ORDER + NEO_KHZ800);

enum Mode { STATIC, DEMO, RAINBOW, FADE, BLINK, WECHSEL };
Mode mode = DEMO;

struct Led { uint8_t r, g, b, bri; };
Led leds[NUM_LEDS];
uint8_t sel = 0;      // 0 = alle, sonst LED-Nummer 1..NUM_LEDS
bool gammaOn = true;  // Gammakorrektur für gleichmäßigere Stufen
uint32_t lastStep = 0;
uint16_t stepIdx = 0;

struct Named { const char *name; uint8_t r, g, b; };
const Named COLORS[] = {
    {"rot", 255, 0, 0},       {"gruen", 0, 255, 0},     {"blau", 0, 0, 255},
    {"weiss", 255, 255, 255}, {"gelb", 255, 160, 0},    {"orange", 255, 60, 0},
    {"cyan", 0, 255, 255},    {"magenta", 255, 0, 255}, {"lila", 120, 0, 255},
    {"warmweiss", 255, 140, 40},
};
const uint8_t NCOLORS = sizeof(COLORS) / sizeof(COLORS[0]);

bool selected(uint8_t i) { return sel == 0 || sel == i + 1; }

uint8_t scale(uint8_t v, uint8_t b) {
  uint16_t x = (uint16_t) v * b / 255;
  return gammaOn ? Adafruit_NeoPixel::gamma8(x) : x;
}

void setPixel(uint8_t i, uint8_t r, uint8_t g, uint8_t b, uint8_t brightness) {
  strip.setPixelColor(i, strip.Color(scale(r, brightness), scale(g, brightness), scale(b, brightness)));
}

void showStatic() {
  for (uint8_t i = 0; i < NUM_LEDS; i++) setPixel(i, leds[i].r, leds[i].g, leds[i].b, leds[i].bri);
  strip.show();
}

void printState() {
  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    Serial.print(selected(i) ? F("* ") : F("  "));
    Serial.print(F("LED ")); Serial.print(i + 1);
    Serial.print(F(": R=")); Serial.print(leds[i].r);
    Serial.print(F(" G=")); Serial.print(leds[i].g);
    Serial.print(F(" B=")); Serial.print(leds[i].b);
    Serial.print(F("  Helligkeit=")); Serial.println(leds[i].bri);
  }
  Serial.print(F("  Gamma=")); Serial.println(gammaOn ? F("an") : F("aus"));
}

void help() {
  Serial.println(F("\n--- APA106 LED-Test (2 LEDs) ---"));
  Serial.println(F("led 1 | led 2 | led alle   Ziel fuer Farb-/Helligkeitsbefehle waehlen (* = gewaehlt)"));
  Serial.println(F("rot gruen blau weiss gelb orange cyan magenta lila warmweiss  Farbe setzen"));
  Serial.println(F("c R G B      eigene Farbe, z. B. 'c 255 80 0'"));
  Serial.println(F("h N          Helligkeit 0..255, z. B. 'h 10' oder 'h 255'"));
  Serial.println(F("+ / -        Helligkeit verdoppeln / halbieren"));
  Serial.println(F("demo         Farben der Reihe nach, LED 2 eine Farbe versetzt (je 2 s)"));
  Serial.println(F("regenbogen   Farbkreis, LED 2 um 180 Grad versetzt"));
  Serial.println(F("fade         eingestellte Farben 0..max..0 dimmen"));
  Serial.println(F("blink        beide gleichzeitig blinken (1 Hz)"));
  Serial.println(F("wechsel      abwechselnd blinken (testet die Kette)"));
  Serial.println(F("order        Test der Farbreihenfolge je LED"));
  Serial.println(F("gamma        Gammakorrektur an/aus"));
  Serial.println(F("aus          gewaehlte LEDs aus"));
  Serial.println(F("?            diese Hilfe"));
  printState();
}

void orderTest() {
  Serial.println(F("Farbreihenfolge: jede LED sollte nacheinander ROT, GRUEN, BLAU leuchten, die andere ist aus."));
  const char *names[] = {"ROT", "GRUEN", "BLAU"};
  for (uint8_t n = 0; n < NUM_LEDS; n++) {
    for (uint8_t c = 0; c < 3; c++) {
      Serial.print(F("  LED ")); Serial.print(n + 1);
      Serial.print(F(": ")); Serial.println(names[c]);
      strip.clear();
      setPixel(n, c == 0 ? 255 : 0, c == 1 ? 255 : 0, c == 2 ? 255 : 0, 64);
      strip.show();
      delay(1200);
    }
  }
  strip.clear();
  strip.show();
  Serial.println(F("Stimmt etwas nicht, LED_ORDER im Sketch anpassen (NEO_RGB / NEO_GRB)."));
  Serial.println(F("Bleibt LED 2 immer dunkel: Verbindung DOUT (LED 1) -> DIN (LED 2) pruefen."));
}

void setColor(uint8_t r, uint8_t g, uint8_t b) {
  for (uint8_t i = 0; i < NUM_LEDS; i++)
    if (selected(i)) { leds[i].r = r; leds[i].g = g; leds[i].b = b; }
  mode = STATIC;
  showStatic();
  printState();
}

void setBri(int v) {
  for (uint8_t i = 0; i < NUM_LEDS; i++)
    if (selected(i)) leds[i].bri = constrain(v, 0, 255);
}

void handle(String cmd) {
  cmd.trim();
  cmd.toLowerCase();
  if (cmd.length() == 0) return;

  for (uint8_t i = 0; i < NCOLORS; i++)
    if (cmd == COLORS[i].name) { setColor(COLORS[i].r, COLORS[i].g, COLORS[i].b); return; }

  if (cmd.startsWith("led ")) {
    const String arg = cmd.substring(4);
    const int n = arg.toInt();
    if (arg == "alle") sel = 0;
    else if (n >= 1 && n <= NUM_LEDS) sel = n;
    else { Serial.println(F("Format: led 1 | led 2 | led alle")); return; }
    printState();
  } else if (cmd.startsWith("c ")) {
    int r, g, b;
    if (sscanf(cmd.c_str() + 2, "%d %d %d", &r, &g, &b) == 3)
      setColor(constrain(r, 0, 255), constrain(g, 0, 255), constrain(b, 0, 255));
    else
      Serial.println(F("Format: c R G B"));
  } else if (cmd.startsWith("h ")) {
    setBri(cmd.substring(2).toInt());
    if (mode == STATIC) showStatic();
    printState();
  } else if (cmd == "+" || cmd == "-") {
    for (uint8_t i = 0; i < NUM_LEDS; i++) {
      if (!selected(i)) continue;
      uint8_t &b = leds[i].bri;
      if (cmd == "+") b = b == 0 ? 1 : min(255, b * 2);
      else b /= 2;
    }
    if (mode == STATIC) showStatic();
    printState();
  } else if (cmd == "demo") {
    mode = DEMO; stepIdx = 0; lastStep = 0;
  } else if (cmd == "regenbogen") {
    mode = RAINBOW; stepIdx = 0;
  } else if (cmd == "fade") {
    mode = FADE; stepIdx = 0;
  } else if (cmd == "blink") {
    mode = BLINK; stepIdx = 0;
  } else if (cmd == "wechsel") {
    mode = WECHSEL; stepIdx = 0;
  } else if (cmd == "order") {
    orderTest();
    if (mode == STATIC) showStatic();
  } else if (cmd == "gamma") {
    gammaOn = !gammaOn;
    if (mode == STATIC) showStatic();
    printState();
  } else if (cmd == "aus") {
    for (uint8_t i = 0; i < NUM_LEDS; i++)
      if (selected(i)) leds[i].r = leds[i].g = leds[i].b = 0;
    mode = STATIC;
    showStatic();
    printState();
  } else if (cmd == "?") {
    help();
  } else {
    Serial.print(F("Unbekannt: ")); Serial.println(cmd);
  }
}

void animate() {
  const uint32_t now = millis();
  switch (mode) {
    case DEMO:
      if (lastStep == 0 || now - lastStep >= 2000) {
        lastStep = now;
        Serial.print(F("Demo:"));
        for (uint8_t i = 0; i < NUM_LEDS; i++) {
          const Named &c = COLORS[(stepIdx + i) % NCOLORS];
          leds[i].r = c.r; leds[i].g = c.g; leds[i].b = c.b;
          Serial.print(F(" LED ")); Serial.print(i + 1); Serial.print('=');
          Serial.print(c.name);
        }
        Serial.println();
        showStatic();
        stepIdx++;
      }
      break;
    case RAINBOW:
      if (now - lastStep >= 20) {
        lastStep = now;
        for (uint8_t i = 0; i < NUM_LEDS; i++) {
          const uint32_t c = Adafruit_NeoPixel::ColorHSV((uint16_t) (stepIdx * 256 + i * 65536UL / NUM_LEDS));
          setPixel(i, (c >> 16) & 0xFF, (c >> 8) & 0xFF, c & 0xFF, leds[i].bri);
        }
        strip.show();
        stepIdx = (stepIdx + 1) % 256;
      }
      break;
    case FADE:
      if (now - lastStep >= 10) {
        lastStep = now;
        const uint16_t p = stepIdx % 510;  // 0..254 hoch, 255..509 runter
        const uint8_t level = p < 255 ? p : 509 - p;
        for (uint8_t i = 0; i < NUM_LEDS; i++)
          setPixel(i, leds[i].r, leds[i].g, leds[i].b, (uint16_t) level * leds[i].bri / 255);
        strip.show();
        stepIdx++;
      }
      break;
    case BLINK:
    case WECHSEL:
      if (now - lastStep >= 500) {
        lastStep = now;
        const bool phase = stepIdx++ % 2;
        for (uint8_t i = 0; i < NUM_LEDS; i++) {
          const bool on = (mode == BLINK) ? !phase : (phase == (i % 2));
          setPixel(i, leds[i].r, leds[i].g, leds[i].b, on ? leds[i].bri : 0);
        }
        strip.show();
      }
      break;
    case STATIC:
      break;
  }
}

String line;

void setup() {
  Serial.begin(115200);
  for (uint8_t i = 0; i < NUM_LEDS; i++) leds[i] = {255, 255, 255, 64};
  strip.begin();
  strip.show();  // aus
  delay(200);
  help();
  orderTest();
  Serial.println(F("Demo startet. Befehl eingeben, um sie zu beenden."));
}

void loop() {
  while (Serial.available()) {
    const char ch = Serial.read();
    if (ch == '\n' || ch == '\r') {
      handle(line);
      line = "";
    } else if (line.length() < 32) {
      line += ch;
    }
  }
  animate();
}
