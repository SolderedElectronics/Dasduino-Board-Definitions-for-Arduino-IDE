/**
 **************************************************
 *
 * @file        WS2812-SOLDERED.h
 * @brief       Header file for library functions
 *
 *
 * @copyright GNU General Public License v3.0
 * @authors     @ soldered.com
 ***************************************************/

#ifndef __WS2812_SOLDERED__
#define __WS2812_SOLDERED__

#include "Arduino.h"
#include "libs/Adafruit_NeoPixel/Adafruit_NeoPixel.h"

class WS2812 : public Adafruit_NeoPixel
{
  public:
    WS2812(int n, int m) : Adafruit_NeoPixel(n, m, NEO_GRB + NEO_KHZ800)
    {
    }
};

#endif
