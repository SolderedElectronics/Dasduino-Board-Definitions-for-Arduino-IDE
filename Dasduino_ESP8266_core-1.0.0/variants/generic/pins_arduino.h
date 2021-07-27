/*
  pins_arduino.h - Pin definition functions for Arduino
  Part of Arduino - http://www.arduino.cc/

  Copyright (c) 2007 David A. Mellis
  Modified for ESP8266 platform by Ivan Grokhotkov, 2014-2015.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General
  Public License along with this library; if not, write to the
  Free Software Foundation, Inc., 59 Temple Place, Suite 330,
  Boston, MA  02111-1307  USA

  $Id: wiring.h 249 2007-02-03 16:52:51Z mellis $
*/

#ifndef Pins_Arduino_h
#define Pins_Arduino_h

#define PIN_WIRE_SDA (4)
#define PIN_WIRE_SCL (5)

static const uint8_t SDA = PIN_WIRE_SDA;
static const uint8_t SCL = PIN_WIRE_SCL;

/*****************************
 * Pins for dasduino connect
 */
//GPIO
#define PA0             (0)
#define PA2             (2)
#define PA4             (4)
#define PA5             (5)
#define PA12            (12)
#define PA13            (13)
#define PA14            (14)
#define PA15            (15)
#define PA16            (16)

//Comunication
#define I2C_SCL         PIN_WIRE_SCL
#define I2C_SDA         PIN_WIRE_SDA

#define SPI_SS          PIN_SPI_SS
#define SPI_MOSI        PIN_SPI_MOSI
#define SPI_MISO        PIN_SPI_MISO
#define SPI_SCK         PIN_SPI_SCK

//Miscelaneus
#define USER_BUTTON     PA0
#define USER_LED        PA2

/*****************************/

#ifndef LED_BUILTIN
#define LED_BUILTIN 1
#endif

#include "common.h"

#endif /* Pins_Arduino_h */
