#ifndef  PARTS_H_
// *INDENT-OFF* The indentation that it demands =is stupid and wrong.
  #define  PARTS_H_

  #include <avr/io.h>

  #if (defined(__AVR_ATmega16__)    || defined(__AVR_ATmega32__)    || defined(__AVR_ATmega64__)    || defined(__AVR_ATmega128__)   || \
         defined(__AVR_ATmega164A__)  || defined(__AVR_ATmega164PA__) || \
         defined(__AVR_ATmega324A__)  || defined(__AVR_ATmega324PA__) || defined(__AVR_ATmega324PB__) || defined(__AVR_ATmega644A__)  || \
         defined(__AVR_ATmega644PA__) || defined(__AVR_ATmega1284__)  || defined(__AVR_ATmega1284P__))

    #define __AVR_ATmega_Mighty__


  #elif  (defined(__AVR_ATmega641__) || defined(__AVR_ATmega1281__) || defined(__AVR_ATmega2561__) || \
          defined(__AVR_ATmega640__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__))

    #define __AVR_ATmega_Mega__


  #elif (defined(__AVR_ATmega328__) || defined(__AVR_ATmega328P__) || defined(__AVR_ATmega328PB__) || \
         defined(__AVR_ATmega168__) || defined(__AVR_ATmega168P__) || defined(__AVR_ATmega168PB__) || defined(__AVR_ATmega168A__)  || defined(__AVR_ATmega168PA__) || \
         defined(__AVR_ATmega88__)  || defined(__AVR_ATmega88P__)  || defined(__AVR_ATmega88PB__)  || \
         defined(__AVR_ATmega8__) || (defined(__ABR_ATtiny88__) && F_CPU==16000000) /* MHET Tiny88 */)

    #define __AVR_ATmega_Mini__
    #ifndef TIMER0_TYPICAL
      #define TIMER0_TYPICAL              (1)
      /* Yes, timer0 is 8-bit timer with or without PWM */
      #define PIN_TIMER_OC0A              (PIN_PD5)
      #define PIN_TIMER_OC0B              (PIN_PD6)
      #define PIN_TIMER_T0                (PIN_PD4)

      #define TIMER1_TYPICAL              (1)
      /* Yes, timer1 is 16-bit PWM timer */
      #define PIN_TIMER_ICP1              (PIN_PB0)
      #define PIN_TIMER_OC1A              (9)
      #define PIN_TIMER_OC1B              (10)
      #define PIN_TIMER_T1                (PIN_PD5)

      #define TIMER2_TYPICAL              (1)
      /* Yes, timer2 is 8-bit async timer */
      #define PIN_TIMER_OC2A              (PIN_PD3)
      #define PIN_TIMER_OC2B              (PIN_PB3)
    #endif
    #define CONFIG_OK
  #endif
  // we can handle the others without mess like what is seen below.

  /*
  #elif (defined(__AVR_ATmega808__) ||defined(__AVR_ATmega1608__) ||defined(__AVR_ATmega3208__) ||defined(__AVR_ATmega4808__) || \
         defined(__AVR_ATmega809__) ||defined(__AVR_ATmega1609__) ||defined(__AVR_ATmega3209__) ||defined(__AVR_ATmega4809__) )

    #define __AVR_ATmega_Zero__

  #else
    #if !(defined(MEGATINYCORE) || defined(DXCORE) || defined(ATTINYCORE))
      // Well then what the heck is it?
      #if  (defined(__AVR_ATtiny3217__) || defined(__AVR_ATtiny1617__) || defined(__AVR_ATtiny1604__) || defined(__AVR_ATtiny1607__) || \
          defined(__AVR_ATtiny3216__) || defined(__AVR_ATtiny1616__) || defined(__AVR_ATtiny1606__) || defined(__AVR_ATtiny1614__))

        #define __AVR_ATtiny_Zero_One__

      #elif  (defined(__AVR_ATtiny3227__) || defined(__AVR_ATtiny1627__) || defined(__AVR_ATtiny3224__) || defined(__AVR_ATtiny1624__) || \
          defined(__AVR_ATtiny3226__) || defined(__AVR_ATtiny1626__) || defined(__AVR_ATtiny827__) || defined(__AVR_ATtiny826__) || defined(__AVR_ATtiny827__))

        #define __AVR_ATtiny_Two__


      #elif (defined(__AVR_AVR128DA28__) ||defined(__AVR_AVR128DA32__) ||defined(__AVR_AVR128DA48__) ||defined(__AVR_AVR128DA64__) || \
           defined(__AVR_AVR64DA28__)  ||defined(__AVR_AVR64DA32__)  ||defined(__AVR_AVR64DA48__)  ||defined(__AVR_AVR64DA64__)  || \
           defined(__AVR_AVR32DA28__)  ||defined(__AVR_AV324DA32__)  ||defined(__AVR_AVR32DA48__)  )

        #define __AVR_DA__

      #elif (defined(__AVR_AVR128DB28__) ||defined(__AVR_AVR128DB32__) ||defined(__AVR_AVR128DB48__) ||defined(__AVR_AVR128DB64__) || \
             defined(__AVR_AVR64DB28__)  ||defined(__AVR_AVR64DB32__)  ||defined(__AVR_AVR64DB48__)  ||defined(__AVR_AVR64DB64__)  || \
             defined(__AVR_AVR32DB28__)  ||defined(__AVR_AV324DB32__)  ||defined(__AVR_AVR32DB48__)  )

        #define __AVR_DB__

      #elif (defined(__AVR_AVR16DD28__)  ||defined(__AVR_AVR16DD32__)  ||defined(__AVR_AVR16DD20__)  ||defined(__AVR_AVR16DD14__) || \
             defined(__AVR_AVR64DD28__)  ||defined(__AVR_AVR6DDA32__)  ||defined(__AVR_AVR64DD20__)  ||defined(__AVR_AVR64DD14__)  || \
             defined(__AVR_AVR32DD28__)  ||defined(__AVR_AV32DDA32__)  ||defined(__AVR_AVR32DD20__)  ||defined(__AVR_AVR64DD14__))

        #define __AVR_DD__
      #else
        error "Unknown or unsupported part"
      #endif
    #else //DXCore, megaTinyCore, ATTinyCore
      #if (CLOCK_SOURCE & 0x0F) != 1 && ((CLOCK_SOURCE & 0x0F) != 2)
        #error "This is a TIMING REFERENCE. Using the internal oscillator would defeat the whole point!"
      #endif
    #endif // for the what if it's not a classic mega
  #endif
  #ifndef CONFIG_OK
    #error "Part unknown, or known but unsupported."
  #endif
*/
#endif
