#include <Adafruit_NeoPixel.h>

#define N_LEDS 18
#define LED_PIN 3
#define POT_PIN A2
Adafruit_NeoPixel strip = Adafruit_NeoPixel(N_LEDS, LED_PIN, NEO_RGB + NEO_KHZ800);

uint8_t colors[4][3];

void setup() {
  // put your setup code here, to run once:

  strip.begin();
  strip.show(); // Initialize all pixels to 'off'

  pinMode(POT_PIN, INPUT);
}

void loop() {
  // put your main code here, to run repeatedly:

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

  makeColor (v, n1, n2);
}

void blendColors(uint8_t brightness, uint8_t blend, const uint8_t rgb1[3], const uint8_t rgb2[3], uint8_t rgb[]) {
  for (i = 0; i < 3; i++) {
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

