#include <FastLED.h>
#include <FastLED_NeoMatrix.h>

const uint16_t TOTAL_WIDTH = 8;
const uint16_t TOTAL_HEIGHT = 32;

const int PIN_MATRIX = 13; // D7

// mqttBri << BRIGHTNESS_SCALE -> full scale seems to be available
#define BRIGHTNESS_SCALE 0

const uint16_t TOTAL_PIXELS = TOTAL_WIDTH * TOTAL_HEIGHT;
CRGB leds[TOTAL_PIXELS];

//   NEO_MATRIX_TOP, NEO_MATRIX_BOTTOM, NEO_MATRIX_LEFT, NEO_MATRIX_RIGHT:
//     Position of the FIRST LED in the matrix; pick two, e.g.
//     NEO_MATRIX_TOP + NEO_MATRIX_LEFT for the top-left corner.
//   NEO_MATRIX_ROWS, NEO_MATRIX_COLUMNS: LEDs are arranged in horizontal
//     rows or in vertical columns, respectively; pick one or the other.
//   NEO_MATRIX_PROGRESSIVE, NEO_MATRIX_ZIGZAG: all rows/columns proceed
//     in the same order, or alternate lines reverse direction; pick one.
FastLED_NeoMatrix matrix = FastLED_NeoMatrix(leds, TOTAL_WIDTH, TOTAL_HEIGHT,
  NEO_MATRIX_TOP     + NEO_MATRIX_RIGHT +
  NEO_MATRIX_ROWS    + NEO_MATRIX_ZIGZAG);

void matrix_setup(uint8_t brightness)
{
    FastLED.addLeds<NEOPIXEL, PIN_MATRIX>(leds, TOTAL_PIXELS);
    FastLED.setBrightness(brightness);
    matrix.begin();
}

void matrix_brightness(uint8_t brightness)
{
    FastLED.setBrightness(brightness);
}

void matrix_update()
{
    matrix.show();

    for (uint16_t i = 0; i < TOTAL_WIDTH * TOTAL_HEIGHT; i++)
    {
		uint8_t r = leds[i].r * 0.99;
		uint8_t g = leds[i].g * 0.99;
		uint8_t b = leds[i].b * 0.99;

        leds[i] = CRGB(r, g, b);
    }

}

void matrix_fill(uint8_t red, uint8_t green, uint8_t blue)
{
    for (uint16_t i = 0; i < TOTAL_WIDTH * TOTAL_HEIGHT; i++)
    {
        leds[i] = CRGB(red, green, blue);
    }
}

void matrix_pixel(uint16_t x, uint16_t y, uint8_t red, uint8_t green, uint8_t blue)
{
    auto i = matrix.XY(x, y);
    leds[i] = CRGB(red, green, blue);
}
