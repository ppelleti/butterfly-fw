#include <Adafruit_NeoPixel.h>

#define N_LEDS 18
#define LED_PIN 3
#define POT_PIN A2
Adafruit_NeoPixel strip = Adafruit_NeoPixel(N_LEDS, LED_PIN, NEO_RGB + NEO_KHZ800);

uint8_t colors[4][3];
uint8_t fade;

void setup() {
  // put your setup code here, to run once:

  strip.begin();
  strip.show(); // Initialize all pixels to 'off'

  pinMode(POT_PIN, INPUT);

  for (uint8_t i = 0; i < 4; i++) {
    randomColor(colors[i]);
  }
}

void loop() {
  // put your main code here, to run repeatedly:

  uint16_t brightness = 128;
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

void blendColors(uint16_t brightness, uint8_t blend, const uint8_t rgb1[3], const uint8_t rgb2[3], uint8_t rgb[]) {
  for (uint8_t i = 0; i < 3; i++) {
    uint32_t accum1, accum2;
    accum1 = rgb1[i];
    accum2 = rgb2[i];
    accum1 *= blend;
    accum2 *= (255 - blend);
    accum1 += accum2;
    accum1 *= brightness;
    accum1 >>= 18;
    rgb[i] = accum1;
  }
}

void computeColors(uint16_t brightness, uint8_t out[3][3]) {
  for (uint8_t i = 0; i < 3; i++) {
    blendColors(brightness, fade, colors[i], colors[i+1], out[i]);
  }
}

void advance(uint8_t inc) {
  uint8_t newFade = fade + inc;

  if (newFade < fade) {
    for (int8_t i = 3; i >= 0; i--) {
      for (uint8_t j = 0; j < 3; j++) {
        colors[i+1][j] = colors[i][j];
      }
      randomColor(colors[0]);
    }

  }
  
  fade = newFade;
}

void setColor(uint8_t pixNo, const uint8_t c[3]) {
  strip.setPixelColor(pixNo, strip.Color(c[0], c[1], c[2]));
}

void showColors(uint16_t brightness) {
  uint8_t result[3][3];
  computeColors(brightness, result);
  
  setColor(2, result[0]);
  setColor(5, result[0]);
  setColor(1, result[1]);
  setColor(6, result[1]);
  setColor(0, result[2]);
  setColor(7, result[2]);
  
  strip.show();
}

