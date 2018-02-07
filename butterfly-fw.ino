/*
 * Copyright 2018 Patrick Pelletier
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * This is the firmware for my butterfly nightlight:
 * https://github.com/ppelleti/butterfly-hw
 *
 * The firmware is meant to run on an ATtiny85 microcontroller,
 * programmed with the Arduino IDE.  Only two pins on the microcontroller
 * are used.  LED_PIN is connected to 18 "NeoPixel" (WS2812) LEDs.
 * POT_PIN is connected to a potentiometer (labeled "POT1" on the back of
 * the butterfly board.)  The potentiometer is used to control the
 * brightness.
 *
 * The firmware produces bands of random color which slowly move outward.
 * The random seed is stored in EEPROM, and is updated each time the
 * program starts, so that the random sequence is different every time.
 *
 * This program requires the Adafruit NeoPixel library:
 * https://github.com/adafruit/Adafruit_NeoPixel
 *
 * and requires support for the ATtiny microcontroller:
 * https://github.com/damellis/attiny
 */

#include <Adafruit_NeoPixel.h>
#include <EEPROM.h>

#define N_LEDS 18
#define LED_PIN 3
#define POT_PIN A2
#define UNUSED_ANALOG_PIN A1

#define DITHERING_BITS 3
#define DITHERING_SHIFT (8 - DITHERING_BITS)

#define FADE_INC 3
#define FADE_BITS 11
#define FADE_MAX ((1 << FADE_BITS) - 1)

#define SEED_ADDR 0

// Interface to NeoPixel LEDs.
Adafruit_NeoPixel strip = Adafruit_NeoPixel(N_LEDS, LED_PIN, NEO_RGB + NEO_KHZ800);

// Four bands of color.  (The three bands displayed are sampled
// in between these four.)
uint8_t colors[4][3];

// Our position (0-FADE_MAX) "in between" the color bands in colors[].
uint16_t fade;

// Incremented by one each refresh.  Used for dithering.
uint8_t dither;

// Gamma correct the potentiometer reading so that halfway is
// about half subjective brightness.  Also, the minimum brightness
// is 8, because less than that doesn't look very good.
const uint8_t PROGMEM pot_gamma[] = {
    8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,
    8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  9,  9,  9,  9,
    9,  9,  9,  9,  9,  9,  9,  9,  9,  9, 10, 10, 10, 10, 10, 10,
   10, 10, 11, 11, 11, 11, 11, 11, 12, 12, 12, 12, 12, 13, 13, 13,
   13, 13, 14, 14, 14, 14, 15, 15, 15, 15, 16, 16, 16, 17, 17, 17,
   18, 18, 18, 19, 19, 19, 20, 20, 21, 21, 21, 22, 22, 23, 23, 24,
   24, 24, 25, 25, 26, 26, 27, 28, 28, 29, 29, 30, 30, 31, 31, 32,
   33, 33, 34, 35, 35, 36, 37, 37, 38, 39, 39, 40, 41, 42, 42, 43,
   44, 45, 45, 46, 47, 48, 49, 50, 50, 51, 52, 53, 54, 55, 56, 57,
   58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 72, 73, 74,
   75, 76, 77, 79, 80, 81, 82, 84, 85, 86, 87, 89, 90, 91, 93, 94,
   95, 97, 98,100,101,103,104,106,107,109,110,112,113,115,116,118,
  120,121,123,125,126,128,130,131,133,135,137,138,140,142,144,146,
  148,150,151,153,155,157,159,161,163,165,167,169,171,173,176,178,
  180,182,184,186,189,191,193,195,198,200,202,205,207,209,212,214,
  216,219,221,224,226,229,231,234,236,239,242,244,247,250,252,255 };

void setup() {
  // put your setup code here, to run once:

  strip.begin();
  strip.show(); // Initialize all pixels to 'off'

  pinMode(POT_PIN, INPUT);
  pinMode(UNUSED_ANALOG_PIN, INPUT);

  initialize_seed();

  for (uint8_t i = 0; i < 4; i++) {
    randomColor(colors[i]);
  }
}

void loop() {
  // put your main code here, to run repeatedly:

  uint8_t brightness = pgm_read_byte(&pot_gamma[analogRead(POT_PIN) >> 2]);
  showColors(brightness);
  advance();
}

// Given v (0-255), n1 (0-2), and n2 (0-2), where n1 != n2,
// produce a fully saturated RGB color.
void makeColor(uint8_t v, uint8_t n1, uint8_t n2, uint8_t rgb[3]) {
  uint8_t i;
  for (i = 0; i < 3; i++) {
    rgb[i] = 0;
  }

  rgb[n1] = v;
  rgb[n2] = 255;
}

// Choose a random (but fully saturated) RGB color.
void randomColor(uint8_t rgb[3]) {
  uint8_t v, n1, n2;

  v = random (256);
  n1 = random (3);
  do {
    n2 = random (3);
  } while (n1 == n2);

  makeColor (v, n1, n2, rgb);
}

// Given two color bands rgb1[] and rgb2[], sample in between them and
// put the result in rgb[].
// brightness (0-255) specifies the overall brightness.
// blend (0-FADE_MAX) specifies where in between the two bands to sample
// the color.
void blendColors(uint8_t brightness, uint16_t blend, const uint8_t rgb1[3],
                 const uint8_t rgb2[3], uint16_t rgb[3]) {
  for (uint8_t i = 0; i < 3; i++) {
    uint32_t accum1, accum2;
    accum1 = rgb1[i];
    accum2 = rgb2[i];
    accum1 *= blend;
    accum2 *= (FADE_MAX - blend);
    accum1 += accum2;
    accum1 *= brightness;
    accum1 >>= FADE_BITS;
    rgb[i] = accum1;
  }
}

// Compute three color bands, sampled from in between the four color
// bands in the global variable colors[], and put the result in out[].
// brightness (0-255) specifies the overall brightness.
void computeColors(uint8_t brightness, uint16_t out[3][3]) {
  for (uint8_t i = 0; i < 3; i++) {
    blendColors(brightness, fade, colors[i], colors[i+1], out[i]);
  }
}

// Increment the sampling position between the color bands by
// one.  This increments the global variable fade, but
// when it wraps around, the color bands in the global variable
// colors[] are moved up by one, and a new random color is stored
// in colors[0].
void advance() {
  fade += FADE_INC;

  if (fade > FADE_MAX) {
    for (int8_t i = 2; i >= 0; i--) {
      for (uint8_t j = 0; j < 3; j++) {
        colors[i+1][j] = colors[i][j];
      }
    }

    randomColor(colors[0]);
    fade &= FADE_MAX;
  }

  dither++;
}

// Round a 16-bit number to an 8-bit number, with dithering.
uint8_t handleRounding(uint8_t threshold, uint16_t value) {
  uint8_t hi, lo;
  hi = value >> 8;
  lo = value;
  if (hi < 255 && (lo >> DITHERING_SHIFT) > (threshold >> DITHERING_SHIFT)) {
    return hi + 1;
  } else {
    return hi;
  }
}

// Reverse the bits in a byte.
uint8_t reverse_bits(uint8_t x) {
  x = ((x & 0x55) << 1) | ((x & 0xaa) >> 1);
  x = ((x & 0x33) << 2) | ((x & 0xcc) >> 2);
#ifdef __BUILTIN_AVR_SWAP
  x = __builtin_avr_swap(x);
#else
  x = ((x & 0x0f) << 4) | ((x & 0xf0) >> 4);
#endif
  return x;
}

// Given a pixel number (0-17), writes the specified color to that pixel.
void setColor(uint8_t pixNo, const uint8_t c[3]) {
  strip.setPixelColor(pixNo, strip.Color(c[0], c[1], c[2]));
}

// Given a pixel number (1-18), writes the specified color band from
// result[] to that pixel.
void px(const uint16_t result[3][3], uint8_t pixNo, uint8_t idx) {
  uint8_t c[3];
  uint8_t p0 = pixNo - 1;
  uint8_t threshold = reverse_bits(p0 + dither);
  for (uint8_t i = 0; i < 3; i++) {
    c[i] = handleRounding(threshold, result[idx][i]);
  }
  setColor(p0, c);
}

// Given the contents of the global variables colors[] and fade,
// and the brightness (0-255) passed as an argument, compute the
// colors that should be shown on the LEDs, and then write the colors
// out to the physical LEDs.
void showColors(uint8_t brightness) {
  // The result of sampling three bands of color from in between the
  // four bands in colors[].
  uint16_t result[3][3];

  computeColors(brightness, result);

  px(result,  1, 0);
  px(result,  2, 1);
  px(result,  3, 2);
  px(result,  4, 0);
  px(result,  5, 1);
  px(result,  6, 2);
  px(result,  7, 1);
  px(result,  8, 2);
  px(result,  9, 1);
  px(result, 10, 0);
  px(result, 11, 1);
  px(result, 12, 2);
  px(result, 13, 0);
  px(result, 14, 1);
  px(result, 15, 2);
  px(result, 16, 1);
  px(result, 17, 2);
  px(result, 18, 1);

  strip.show();
}

// Read a couple of analog inputs to get some randomness.
uint32_t get_entropy() {
  uint16_t pot = analogRead(POT_PIN);
  uint16_t x = analogRead(UNUSED_ANALOG_PIN);
  uint32_t ret = pot;
  ret <<= 10;
  ret ^= x;
  return ret;
}

// The 32-bit finalizer from the Murmur3 hash function.
uint32_t murmur3_finalizer(uint32_t h) {
  h ^= h >> 16;
  h *= 0x85ebca6b;
  h ^= h >> 13;
  h *= 0xc2b2ae35;
  h ^= h >> 16;

  return h;
}

// Initialize the random number generator, using the seed stored in
// EEPROM, and some analog inputs.  Then update the seed stored in EEPROM.
void initialize_seed() {
  uint32_t oldSeed, newSeed;

  EEPROM.get(SEED_ADDR, oldSeed);
  newSeed = murmur3_finalizer(oldSeed + get_entropy());

  randomSeed(newSeed);
  EEPROM.put(SEED_ADDR, newSeed);
}
