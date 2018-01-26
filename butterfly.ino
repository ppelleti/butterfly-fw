#include <Adafruit_NeoPixel.h>

#define N_LEDS 18
#define LED_PIN 3
#define POT_PIN A2
Adafruit_NeoPixel strip = Adafruit_NeoPixel(N_LEDS, LED_PIN, NEO_RGB + NEO_KHZ800);

struct Pixel {
  uint8_t p[3];
};

Pixel colors[4];
Pixel result[3];
uint8_t fade;

// https://learn.adafruit.com/led-tricks-gamma-correction/the-quick-fix
const uint8_t PROGMEM gamma8[] = {
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,
    1,  1,  1,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,
    2,  3,  3,  3,  3,  3,  3,  3,  4,  4,  4,  4,  4,  5,  5,  5,
    5,  6,  6,  6,  6,  7,  7,  7,  7,  8,  8,  8,  9,  9,  9, 10,
   10, 10, 11, 11, 11, 12, 12, 13, 13, 13, 14, 14, 15, 15, 16, 16,
   17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22, 23, 24, 24, 25,
   25, 26, 27, 27, 28, 29, 29, 30, 31, 32, 32, 33, 34, 35, 35, 36,
   37, 38, 39, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 50,
   51, 52, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 66, 67, 68,
   69, 70, 72, 73, 74, 75, 77, 78, 79, 81, 82, 83, 85, 86, 87, 89,
   90, 92, 93, 95, 96, 98, 99,101,102,104,105,107,109,110,112,114,
  115,117,119,120,122,124,126,127,129,131,133,135,137,138,140,142,
  144,146,148,150,152,154,156,158,160,162,164,167,169,171,173,175,
  177,180,182,184,186,189,191,193,196,198,200,203,205,208,210,213,
  215,218,220,223,225,228,231,233,236,239,241,244,247,249,252,255 };

void setup() {
  // put your setup code here, to run once:

  strip.begin();
  strip.show(); // Initialize all pixels to 'off'

  pinMode(POT_PIN, INPUT);

  for (uint8_t i = 0; i < 4; i++) {
    randomColor(colors[i].p);
  }
}

void loop() {
  // put your main code here, to run repeatedly:

  uint8_t brightness = pgm_read_byte(&gamma8[analogRead(POT_PIN) >> 2]);
  showColors(brightness);
  advance(4);
  delay(20);
}

void makeColor(uint8_t v, uint8_t n1, uint8_t n2, uint8_t rgb[3]) {
  uint8_t i;
  for (i = 0; i < 3; i++) {
    rgb[i] = 0;
  }

  rgb[n1] = v;
  rgb[n2] = 255;
}

void randomColor(uint8_t rgb[3]) {
  uint8_t v, n1, n2;

  v = random (256);
  n1 = random (3);
  do {
    n2 = random (3);
  } while (n1 == n2);

  makeColor (v, n1, n2, rgb);
}

void blendColors(uint8_t brightness, uint8_t blend, const uint8_t rgb1[3], const uint8_t rgb2[3], uint8_t rgb[3]) {
  for (uint8_t i = 0; i < 3; i++) {
    uint32_t accum1, accum2;
    accum1 = rgb1[i];
    accum2 = rgb2[i];
    accum1 *= blend;
    accum2 *= (255 - blend);
    accum1 += accum2;
    accum1 *= brightness;
    accum1 >>= 16;
    rgb[i] = accum1;
  }
}

void computeColors(uint8_t brightness, Pixel out[3]) {
  for (uint8_t i = 0; i < 3; i++) {
    blendColors(brightness, fade, colors[i].p, colors[i+1].p, out[i].p);
  }
}

void advance(uint8_t inc) {
  uint8_t newFade = fade + inc;

  if (newFade < fade) {
    for (int8_t i = 2; i >= 0; i--) {
      for (uint8_t j = 0; j < 3; j++) {
        colors[i+1].p[j] = colors[i].p[j];
      }
    }
    
    randomColor(colors[0].p);
  }
  
  fade = newFade;
}

void setColor(uint8_t pixNo, const uint8_t c[3]) {
  strip.setPixelColor(pixNo, strip.Color(c[0], c[1], c[2]));
}

void px(uint8_t pixNo, uint8_t idx) {
  setColor(pixNo - 1, result[idx].p);
}

void showColors(uint8_t brightness) {
  computeColors(brightness, result);

  for (uint8_t i = 1; i <= N_LEDS; i++) {
    px(i, 1);
  }

  px(1, 0);
  px(4, 0);
  px(10, 0);
  px(13, 0);

  px(3, 2);
  px(6, 2);
  px(8, 2);
  px(12, 2);
  px(15, 2);
  px(17, 2);
  
  strip.show();
}

