// LED Fade Example for Dasduino Board definitions
//
// Original library by AdaFruit Industries, modified by Soldered

#include "WS2812-SOLDERED.h"

WS2812 pixels(1, LEDWS_BUILTIN);

int brightness = 0; // how bright the LED is
int fadeAmount = 5; // how many points to fade the LED by

void setup()
{
    pixels.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)
}

void loop()
{
    pixels.setPixelColor(0, pixels.Color(brightness, 0, brightness)); // Set the color of built-in WS2812 RGB LED
    pixels.show();

    brightness = brightness + fadeAmount; // Change the brightness for next time through the loop:

    if (brightness <= 0 || brightness >= 255) // Reverse the direction of the fading at the ends of the fade:
    {
        fadeAmount = -fadeAmount;
    }

    delay(30); // Wait for 30 milliseconds to see the dimming effect
}
