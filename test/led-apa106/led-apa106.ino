// Test für eine einzelne APA106-F8 / P9823 LED (WS2812-kompatibel) am Arduino Nano.
//
// Verkabelung (ohne Zusatzbauteile):
//   LED VDD  -> Nano 5V
//   LED GND  -> Nano GND
//   LED DIN  -> Nano D6
//   LED DOUT -> frei lassen
//
// Bedienung über den seriellen Monitor (115200 Baud, Zeilenende beliebig). "?" zeigt die Hilfe.
// Nach dem Start läuft einmal der Farbreihenfolge-Test, danach die Demo.

#include <Adafruit_NeoPixel.h>

#define LED_PIN 6
// APA106/P9823 erwarten meist RGB-Reihenfolge. Zeigt der Test "rot" grün an, NEO_GRB eintragen.
#define LED_ORDER NEO_RGB

Adafruit_NeoPixel led(1, LED_PIN, LED_ORDER + NEO_KHZ800);

enum Mode { STATIC, DEMO, RAINBOW, FADE, BLINK };
Mode mode = DEMO;
uint8_t cr = 255, cg = 255, cb = 255;  // aktuelle Farbe (vor Helligkeit)
uint8_t bri = 64;                      // Helligkeit 0..255
bool gammaOn = true;                   // Gammakorrektur für gleichmäßigere Stufen
uint32_t lastStep = 0;
uint16_t stepIdx = 0;

struct Named { const char *name; uint8_t r, g, b; };
const Named COLORS[] = {
    {"rot", 255, 0, 0},       {"gruen", 0, 255, 0},    {"blau", 0, 0, 255},
    {"weiss", 255, 255, 255}, {"gelb", 255, 160, 0},   {"orange", 255, 60, 0},
    {"cyan", 0, 255, 255},    {"magenta", 255, 0, 255}, {"lila", 120, 0, 255},
    {"warmweiss", 255, 140, 40},
};
const uint8_t NCOLORS = sizeof(COLORS) / sizeof(COLORS[0]);

uint8_t scale(uint8_t v, uint8_t b) {
  uint16_t x = (uint16_t) v * b / 255;
  return gammaOn ? Adafruit_NeoPixel::gamma8(x) : x;
}

void show(uint8_t r, uint8_t g, uint8_t b, uint8_t brightness) {
  led.setPixelColor(0, led.Color(scale(r, brightness), scale(g, brightness), scale(b, brightness)));
  led.show();
}

void showCurrent() { show(cr, cg, cb, bri); }

void printState() {
  Serial.print(F("Farbe R=")); Serial.print(cr);
  Serial.print(F(" G=")); Serial.print(cg);
  Serial.print(F(" B=")); Serial.print(cb);
  Serial.print(F("  Helligkeit=")); Serial.print(bri);
  Serial.print(F("  Gamma=")); Serial.println(gammaOn ? F("an") : F("aus"));
}

void help() {
  Serial.println(F("\n--- APA106 LED-Test ---"));
  Serial.println(F("rot gruen blau weiss gelb orange cyan magenta lila warmweiss  Farbe setzen"));
  Serial.println(F("c R G B      eigene Farbe, z. B. 'c 255 80 0'"));
  Serial.println(F("h N          Helligkeit 0..255, z. B. 'h 10' oder 'h 255'"));
  Serial.println(F("+ / -        Helligkeit verdoppeln / halbieren"));
  Serial.println(F("demo         Farben der Reihe nach (je 2 s)"));
  Serial.println(F("regenbogen   Farbkreis durchlaufen"));
  Serial.println(F("fade         aktuelle Farbe 0..max..0 dimmen"));
  Serial.println(F("blink        aktuelle Farbe blinken (1 Hz)"));
  Serial.println(F("order        Test der Farbreihenfolge (rot, gruen, blau)"));
  Serial.println(F("gamma        Gammakorrektur an/aus"));
  Serial.println(F("aus          LED aus"));
  Serial.println(F("?            diese Hilfe"));
  printState();
}

void orderTest() {
  Serial.println(F("Farbreihenfolge: es sollte nacheinander ROT, GRUEN, BLAU leuchten."));
  const char *names[] = {"ROT", "GRUEN", "BLAU"};
  for (uint8_t i = 0; i < 3; i++) {
    Serial.print(F("  jetzt: ")); Serial.println(names[i]);
    show(i == 0 ? 255 : 0, i == 1 ? 255 : 0, i == 2 ? 255 : 0, 64);
    delay(1500);
  }
  Serial.println(F("Stimmt etwas nicht, LED_ORDER im Sketch anpassen (NEO_RGB / NEO_GRB)."));
}

void setStatic(uint8_t r, uint8_t g, uint8_t b) {
  cr = r; cg = g; cb = b;
  mode = STATIC;
  showCurrent();
  printState();
}

void handle(String cmd) {
  cmd.trim();
  cmd.toLowerCase();
  if (cmd.length() == 0) return;

  for (uint8_t i = 0; i < NCOLORS; i++)
    if (cmd == COLORS[i].name) { setStatic(COLORS[i].r, COLORS[i].g, COLORS[i].b); return; }

  if (cmd.startsWith("c ")) {
    int r, g, b;
    if (sscanf(cmd.c_str() + 2, "%d %d %d", &r, &g, &b) == 3) {
      setStatic(constrain(r, 0, 255), constrain(g, 0, 255), constrain(b, 0, 255));
    } else {
      Serial.println(F("Format: c R G B"));
    }
  } else if (cmd.startsWith("h ")) {
    bri = constrain(cmd.substring(2).toInt(), 0, 255);
    if (mode == STATIC) showCurrent();
    printState();
  } else if (cmd == "+") {
    bri = bri == 0 ? 1 : min(255, bri * 2);
    if (mode == STATIC) showCurrent();
    printState();
  } else if (cmd == "-") {
    bri /= 2;
    if (mode == STATIC) showCurrent();
    printState();
  } else if (cmd == "demo") {
    mode = DEMO; stepIdx = 0; lastStep = 0;
  } else if (cmd == "regenbogen") {
    mode = RAINBOW; stepIdx = 0;
  } else if (cmd == "fade") {
    mode = FADE; stepIdx = 0;
  } else if (cmd == "blink") {
    mode = BLINK; stepIdx = 0;
  } else if (cmd == "order") {
    orderTest();
    if (mode == STATIC) showCurrent();
  } else if (cmd == "gamma") {
    gammaOn = !gammaOn;
    if (mode == STATIC) showCurrent();
    printState();
  } else if (cmd == "aus") {
    mode = STATIC;
    show(0, 0, 0, 0);
    Serial.println(F("LED aus"));
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
        const Named &c = COLORS[stepIdx % NCOLORS];
        cr = c.r; cg = c.g; cb = c.b;
        showCurrent();
        Serial.print(F("Demo: ")); Serial.println(c.name);
        stepIdx++;
      }
      break;
    case RAINBOW:
      if (now - lastStep >= 20) {
        lastStep = now;
        uint32_t c = Adafruit_NeoPixel::ColorHSV(stepIdx * 256);  // 256 Schritte für einen Umlauf
        show((c >> 16) & 0xFF, (c >> 8) & 0xFF, c & 0xFF, bri);
        stepIdx = (stepIdx + 1) % 256;
      }
      break;
    case FADE:
      if (now - lastStep >= 10) {
        lastStep = now;
        const uint16_t p = stepIdx % 510;  // 0..254 hoch, 255..509 runter
        const uint8_t level = p < 255 ? p : 509 - p;
        show(cr, cg, cb, (uint16_t) level * bri / 255);
        stepIdx++;
      }
      break;
    case BLINK:
      if (now - lastStep >= 500) {
        lastStep = now;
        if (stepIdx++ % 2) show(0, 0, 0, 0); else showCurrent();
      }
      break;
    case STATIC:
      break;
  }
}

String line;

void setup() {
  Serial.begin(115200);
  led.begin();
  led.show();  // aus
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
