// LED Blink Example for Dasduino Board definitions
//
// Original library by AdaFruit Industries, modified by Soldered

#include "WS2812-SOLDERED.h"

WS2812 pixels(1, LEDWS_BUILTIN);

void setup()
{
    pixels.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)
}

void loop()
{
    // Turn LED on puple
    pixels.setPixelColor(0, pixels.Color(80, 0, 80));
    pixels.show();
    delay(1000);

    // Turn LED off
    pixels.setPixelColor(0, pixels.Color(0, 0, 0));
    pixels.show();
    delay(1000);
}
