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
#include <tinyNeoPixel.h>

class WS2812 : public tinyNeoPixel
{
public:
  WS2812(int n, int m) : tinyNeoPixel(n, m, NEO_GRB + NEO_KHZ800)
  {
  }
};

#endif
