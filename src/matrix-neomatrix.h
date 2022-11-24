#include <NeoPixelBus.h>

const uint16_t TOTAL_WIDTH = 8;
const uint16_t TOTAL_HEIGHT = 32;

const uint16_t TOTAL_PIXELS = TOTAL_WIDTH * TOTAL_HEIGHT;

// Efficient connection via DMA on pin RDX0 GPIO3 RX
// See <https://github.com/Makuna/NeoPixelBus/wiki/FAQ-%231>
NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod> strip(TOTAL_PIXELS);

// bitbanging (Fallback)
// const int PIN_MATRIX = 13; // D7
// NeoPixelBus<NeoGrbFeature, NeoEsp8266BitBang800KbpsMethod> strip(TOTAL_PIXELS, PIN_MATRIX);

NeoTopology<RowMajorAlternatingLayout> topo(TOTAL_WIDTH, TOTAL_HEIGHT);

struct ColorBufferColor
{
	float r;
	float g;
	float b;
};
struct ColorBufferColor colorBuffer[TOTAL_PIXELS];

uint8_t globalBrightness = 255;

void matrix_setup(uint8_t brightness)
{
	strip.Begin();
	globalBrightness = brightness;
}

void matrix_brightness(uint8_t brightness)
{
	globalBrightness = brightness;
}

void matrix_update()
{
	for (uint16_t i = 0; i < TOTAL_WIDTH * TOTAL_HEIGHT; i++)
	{
		auto color = RgbColor(colorBuffer[i].r, colorBuffer[i].g, colorBuffer[i].b);
		strip.SetPixelColor(i, color.Dim(globalBrightness));
	}

	strip.Show();

	for (uint16_t i = 0; i < TOTAL_WIDTH * TOTAL_HEIGHT; i++)
	{
		colorBuffer[i].r *= 0.99;
		colorBuffer[i].g *= 0.99;
		colorBuffer[i].b *= 0.99;
	}
}

void matrix_fill(uint8_t red, uint8_t green, uint8_t blue)
{
	struct ColorBufferColor color = {(float)red, (float)green, (float)blue};
	for (uint16_t i = 0; i < TOTAL_WIDTH * TOTAL_HEIGHT; i++)
	{
		colorBuffer[i] = color;
	}
}

void matrix_pixel(uint16_t x, uint16_t y, uint8_t red, uint8_t green, uint8_t blue)
{
	auto i = topo.Map(TOTAL_WIDTH - x - 1, y);
	struct ColorBufferColor color = {(float)red, (float)green, (float)blue};
	colorBuffer[i] = color;
}
