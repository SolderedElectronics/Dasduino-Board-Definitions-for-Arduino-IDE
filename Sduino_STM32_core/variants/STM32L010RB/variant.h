/*
 *******************************************************************************
 * Copyright (c) 2019, STMicroelectronics
 * All rights reserved.
 *
 * This software component is licensed by ST under BSD 3-Clause license,
 * the "License"; You may not use this file except in compliance with the
 * License. You may obtain a copy of the License at:
 *                        opensource.org/licenses/BSD-3-Clause
 *
 *******************************************************************************
 */

#ifndef _VARIANT_ARDUINO_STM32_
#define _VARIANT_ARDUINO_STM32_

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/*----------------------------------------------------------------------------
 *        Pins
 *----------------------------------------------------------------------------*/

// Define pin names to match digital pin number --> Dx
// It could be used with preprocessor tests (e.g. #if PXn == 3)
// so an enum will not work.
// !!!
// !!! Copy the digitalPin[] array from the variant.cpp
// !!! and remove all '_': PX_n --> PXn
// !!! For NC, comment the line to warn x pin number is NC
// !!! // x is NC
// !!! For duplicated pin name, comment the line to warn x pin number
// !!! is PXn which is already defined with y pin number
// !!! // x is PXn (y)
// !!! Ex:
// !!! ...
// !!! #define PA4  20 // A14 <-- if NUM_ANALOG_FIRST not defined
// !!! or
// !!! #define PA4  A14 // 20 <-- if NUM_ANALOG_FIRST defined
// !!! #define PB4  21
// !!! #define PB5  22
// !!! #define PB3  23
// !!! // 24 is PA4 (20)
// !!! // 25 is PB4 (21)
// !!! #define PA2  26 // A15 <-- if NUM_ANALOG_FIRST not defined
// !!! or
// !!! #define PA2  A15 // 26 <-- if NUM_ANALOG_FIRST defined
// !!! ...
//#define PXn x
//Left side
#define PB14  0
#define PB13  1
#define PB12  2
#define PB11  3
#define PB10  4
#define PB2	5
#define PB1 6	// A9
#define PB0 7	// A8
#define PA7 8	// A7  //SPI MOSI
#define PA6 9   // A6  //SPI MISO
#define PA5 10  // A5  //SPI SCK
#define PA3 11  // A3
#define PA2 12  // A2
#define PA1  13 // A1
#define PA0  14 // A0
#define PC3  15 // A13
#define PC2  16 // A12
#define PC1  17 // A11
#define PC0  18 // A10
#define PC13 19 

//Right side
#define PB15  20  
#define PC6  21  
#define PC7  22 
#define PC8  23
#define PC9  24
#define PA8  25
#define PA9  26
#define PA10  27
#define PA11  28
#define PA12  29
#define PA15  30
#define PC10  31
#define PC11  32
#define PC12  33
#define PD2  34
#define PB3  35
#define PB4  36
#define PB5  37
#define PB6  38
#define PB7  39
#define PB8  40
#define PB9  41

// Other
#define PA13 42 // SWDI0
#define PA14 43 // SWCLK
#define PA4  44 // NSS LORA    //A4 not use
#define PC4  45 // RESET LORA  //A14 not use
#define PC5	 46	// DIO0 LORA   //A15 not use
// This must be a literal
#define NUM_DIGITAL_PINS        63
// This must be a literal with a value less than or equal to to MAX_ANALOG_INPUTS
#define NUM_ANALOG_INPUTS       16
#define NUM_ANALOG_FIRST        47


// Below ADC and PWM definitions already done in the core
// Could be redefined here if needed
// ADC resolution is 10 bits
//#define ADC_RESOLUTION          10

// PWM resolution
//#define PWM_RESOLUTION          8
//#define PWM_FREQUENCY           1000
//#define PWM_MAX_DUTY_CYCLE      255

// On-board LED pin number
// STM32L010RB_LORA 
//#define LED_BUILTIN             13
//#define LED_GREEN               LED_BUILTIN


// Below SPI and I2C definitions already done in the core
// Could be redefined here if differs from the default one
// SPI Definitions
//#define PIN_SPI_SS              10 // Default for Arduino connector compatibility
#define PIN_SPI_MOSI            8 // Default for Arduino connector compatibility
#define PIN_SPI_MISO            9 // Default for Arduino connector compatibility
#define PIN_SPI_SCK             10 // Default for Arduino connector compatibility

// I2C Definitions
#define PIN_WIRE_SDA            27 // Default for Arduino connector compatibility
#define PIN_WIRE_SCL            26 // Default for Arduino connector compatibility

// I2C timing definitions (optional), avoid time spent to compute if defined
// * I2C_TIMING_SM for Standard Mode (100kHz)
// * I2C_TIMING_FM for Fast Mode (400kHz)
// * I2C_TIMING_FMP for Fast Mode Plus (1000kHz)
//#define I2C_TIMING_SM           0x00000000
//#define I2C_TIMING_FM           0x00000000
//#define I2C_TIMING_FMP          0x00000000

// Timer Definitions (optional)
// Use TIM6/TIM7 when possible as servo and tone don't need GPIO output pin
#define TIMER_TONE              TIM2
#define TIMER_SERVO             TIM22

// UART Definitions
// Define here Serial instance number to map on Serial generic name
#define SERIAL_UART_INSTANCE    2 //ex: 2 for Serial2 (USART2)
// DEBUG_UART could be redefined to print on another instance than 'Serial'
//#define DEBUG_UART              ((USART_TypeDef *) U(S)ARTX) // ex: USART3
// DEBUG_UART baudrate, default: 9600 if not defined
//#define DEBUG_UART_BAUDRATE     x
// DEBUG_UART Tx pin name, default: the first one found in PinMap_UART_TX for DEBUG_UART
//#define DEBUG_PINNAME_TX        PX_n // PinName used for TX

// Default pin used for 'Serial' instance (ex: ST-Link)
// Mandatory for Firmata
#define PIN_SERIAL_RX           11
#define PIN_SERIAL_TX           12

// Optional PIN_SERIALn_RX and PIN_SERIALn_TX where 'n' is the U(S)ART number
// Used when user instanciate a hardware Serial using its peripheral name.
// Example: HardwareSerial mySerial(USART3);
// will use PIN_SERIAL3_RX and PIN_SERIAL3_TX if defined.
//#define PIN_SERIALn_RX          x // For U(S)ARTn RX
//#define PIN_SERIALn_TX          x // For U(S)ARTn TX
//#define PIN_SERIALLP1_RX        x // For LPUART1 RX
//#define PIN_SERIALLP1_TX        x // For LPUART1 TX

// SD card slot Definitions
// SD detect signal can be defined if required
//#define SD_DETECT_PIN           x
// SD Read/Write timeout, default value defined in STM32SD library
//#define SD_DATATIMEOUT          x

#ifdef __cplusplus
} // extern "C"
#endif
/*----------------------------------------------------------------------------
 *        Arduino objects - C++ only
 *----------------------------------------------------------------------------*/

#ifdef __cplusplus
// These serial port names are intended to allow libraries and architecture-neutral
// sketches to automatically default to the correct port name for a particular type
// of use.  For example, a GPS module would normally connect to SERIAL_PORT_HARDWARE_OPEN,
// the first hardware serial port whose RX/TX pins are not dedicated to another use.
//
// SERIAL_PORT_MONITOR        Port which normally prints to the Arduino Serial Monitor
//
// SERIAL_PORT_USBVIRTUAL     Port which is USB virtual serial
//
// SERIAL_PORT_LINUXBRIDGE    Port which connects to a Linux system via Bridge library
//
// SERIAL_PORT_HARDWARE       Hardware serial port, physical RX & TX pins.
//
// SERIAL_PORT_HARDWARE_OPEN  Hardware serial ports which are open for use.  Their RX & TX
//                            pins are NOT connected to anything by default.
#define SERIAL_PORT_MONITOR     Serial
#define SERIAL_PORT_HARDWARE    Serial
#endif

#endif /* _VARIANT_ARDUINO_STM32_ */
