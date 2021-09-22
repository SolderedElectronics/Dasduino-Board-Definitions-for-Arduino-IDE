/*
  wiring.c - Partial implementation of the Wiring API for the ATmega8.
  Part of Arduino - http://www.arduino.cc/

  Copyright (c) 2005-2006 David A. Mellis

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
*/

#include "wiring_private.h"
#include "util/delay.h"
/* Declared in wiring_private.h
 * PeripheralControl is used to mark peripherals as being "taken over" by the user
 * 0x40 = TIMERD0
 * 0x10 = TIMERA0
 * Implementation and use is not portable between cores - tradeoffs are made which
 * trade generalizability for low resource use
 */
uint8_t PeripheralControl = 0xFF;

// the prescaler is set so that timer ticks every 64 clock cycles, and the
// the overflow handler is called every 256 ticks.


inline uint16_t clockCyclesPerMicrosecond() {
  return ((F_CPU) / 1000000L);
}

inline unsigned long clockCyclesToMicroseconds(unsigned long cycles) {
  return (cycles / clockCyclesPerMicrosecond());
}

inline unsigned long microsecondsToClockCycles(unsigned long microseconds) {
  return (microseconds * clockCyclesPerMicrosecond());
}


#ifndef MILLIS_USE_TIMERNONE


//volatile uint16_t microseconds_per_timer_overflow;
//volatile uint16_t microseconds_per_timer_tick;

#if (defined(MILLIS_USE_TIMERB0)  || defined(MILLIS_USE_TIMERB1) ) //Now TCB as millis source does not need fraction
  volatile uint32_t timer_millis = 0; //That's all we need to track here

#elif !defined(MILLIS_USE_TIMERRTC) //all of this stuff is not used when the RTC is used as the timekeeping timer
  static uint16_t timer_fract = 0;
  uint16_t fract_inc;
  volatile uint32_t timer_millis = 0;
  #define FRACT_MAX (1000)
  #define FRACT_INC (millisClockCyclesToMicroseconds(TIME_TRACKING_CYCLES_PER_OVF)%1000);
  #define MILLIS_INC (millisClockCyclesToMicroseconds(TIME_TRACKING_CYCLES_PER_OVF)/1000);
  volatile uint32_t timer_overflow_count = 0;
#else
  volatile uint16_t timer_overflow_count = 0;
#endif

//overflow count is tracked for all timer options, even the RTC


#if !defined(MILLIS_USE_TIMERRTC)


// when TCD0 is used as millis source, this will be different from above, but 99 times out of 10, when a piece of code asks for clockCyclesPerMicrosecond(), they're asking about CLK_PER/CLK_MAIN/etc, not the unprescaled TCD0!
inline uint16_t millisClockCyclesPerMicrosecond() {
  #ifdef MILLIS_USE_TIMERD0
    #if (F_CPU==20000000UL || F_CPU==10000000UL ||F_CPU==5000000UL)
      return (20);   //this always runs off the 20MHz oscillator
    #else
      return (16);
    #endif
  #else
    return ((F_CPU) / 1000000L);
  #endif
}


inline unsigned long millisClockCyclesToMicroseconds(unsigned long cycles) {
  return (cycles / millisClockCyclesPerMicrosecond());
}

inline unsigned long microsecondsToMillisClockCycles(unsigned long microseconds) {
  return (microseconds * millisClockCyclesPerMicrosecond());
}


#if defined(MILLIS_USE_TIMERD0)
  #ifndef TCD0
    #error "Selected millis timer, TCD0, only exists on 1-series parts"
  #endif
#elif !defined(MILLIS_USE_TIMERA0)
  static volatile TCB_t *_timer =
  #if defined(MILLIS_USE_TIMERB0)
    &TCB0;
  #elif defined(MILLIS_USE_TIMERB1)
    #ifndef TCB1
      #error "Selected millis timer, TCB1 does not exist on this part."
    #endif
    &TCB1;
  #else  //it's not TCB0, TCB1, TCD0, TCA0, or RTC
    #error "No millis timer selected, but not disabled - can't happen!".
  #endif
#endif

#endif //end #if !defined(MILLIS_USE_TIMERRTC)


#if defined(MILLIS_USE_TIMERA0)
  ISR(TCA0_HUNF_vect)
#elif defined(MILLIS_USE_TIMERD0)
  ISR(TCD0_OVF_vect)
#elif defined(MILLIS_USE_TIMERRTC)
  ISR(RTC_CNT_vect)
#elif defined(MILLIS_USE_TIMERB0)
  ISR(TCB0_INT_vect)
#elif defined(MILLIS_USE_TIMERB1)
  ISR(TCB1_INT_vect)
#else
  #error "No millis timer selected, but not disabled - cannot determine millis vector"
#endif
{
  // copy these to local variables so they can be stored in registers
  // (volatile variables must be read from memory on every access)

  #if (defined(MILLIS_USE_TIMERB0)|defined(MILLIS_USE_TIMERB1))
    #if(F_CPU>1000000)
      timer_millis++; //that's all we need to do!
    #else
      timer_millis += 2;
    #endif
  #else
    #if !defined(MILLIS_USE_TIMERRTC) //TCA0 or TCD0
      uint32_t m = timer_millis;
      uint16_t f = timer_fract;
      m += MILLIS_INC;
      f += FRACT_INC;
      if (f >= FRACT_MAX) {

        f -= FRACT_MAX;
        m += 1;
      }
      timer_fract = f;
      timer_millis = m;
    #endif
    //if RTC is used as timer, we only increment the overflow count
    timer_overflow_count++;
  #endif
  /* Clear flag */
  #if defined(MILLIS_USE_TIMERA0)
    TCA0.SPLIT.INTFLAGS = TCA_SPLIT_HUNF_bm;
  #elif defined(MILLIS_USE_TIMERD0)
    TCD0.INTFLAGS = TCD_OVF_bm;
  #elif defined(MILLIS_USE_TIMERRTC)
    RTC.INTFLAGS = RTC_OVF_bm;
  #else //timerb
    _timer->INTFLAGS = TCB_CAPT_bm;
  #endif
}

unsigned long millis() {
  //return timer_overflow_count; //for debugging timekeeping issues where these variables are out of scope from the sketch
  unsigned long m;

  // disable interrupts while we read timer0_millis or we might get an
  // inconsistent value (e.g. in the middle of a write to timer0_millis)
  uint8_t oldSREG = SREG;
  cli();
  #if defined(MILLIS_USE_TIMERRTC)
    uint16_t rtccount=RTC.CNT;
    m = timer_overflow_count;
    if (RTC.INTFLAGS & RTC_OVF_bm) {
      /* There has just been an overflow that hasn't been accounted for by the interrupt. Check if the high bit of counter is set.
       * We just basically need to make sure that it didn't JUST roll over at the last couple of clocks. But this merthod is
       * implemented very efficiently (just an sbrs) so it is more efficient than other approaches. If user code is leaving
       * interrupts off nearly long enough for this to be wrong, they shouldn't expect millis to work right. */
      if (!(rtccount & 0x8000)) m++;
    }
    SREG = oldSREG;
    m = (m << 16);
    m += rtccount;
    m = m - (m >> 5) + (m >> 7);
    /* the compiler is incorrigible - it cannot be convinced not to copy m twice, shifting one 7 times and the other 5 times
     * and wasting 35 clock cycles and several precious instruction words.
     * This ends up with the exact same implementation
    uint32_t n = (m >> 5);
    m -= n;
    n = n >> 2;
    m += n;
     * What one would want to do is, first load the overflowcount into the high bytes in the first place, (4 words 6 clocks)
     * followed by the count (4 & 6) Do the check for recent overflow as it's done now essentially (7 & 9). Then, we'd move
     * to the 1024->1000-ifier. So: movw the 4 bytes into 4 new registers... and clear the register after it, (3 clocks 3 words).
     * This would form a 5 byte register, in effect. Then leftshift those one each, and shift the carry bit into a fifth register
     * like it was all one (5 clocks 5 words). It now contain m >> 7, add to m (4 & 4).
     * Repeat the leftshift twice more (17 clocks 9 words) giving m >> 5, subtract (4 & 4)  and you're done.
     * This not only is more efficient in and of itself, but it  ALSO saves you 4 words and 6 clocks in the prologue and epilogue
     * because you don't need to save and restore r16 and r17 because you don't piss away 4 registers. */
  #else
    m = timer_millis;
    SREG = oldSREG;
  #endif
  return m;
}
#ifndef MILLIS_USE_TIMERRTC

unsigned long micros() {
  unsigned long overflows, microseconds;
  #if (defined(MILLIS_USE_TIMERD0) || defined(MILLIS_USE_TIMERB0) || defined(MILLIS_USE_TIMERB1))
    uint16_t ticks;
  #else
    uint8_t ticks;
  #endif
  uint8_t flags;
  /* Save current state and disable interrupts */
  uint8_t oldSREG = SREG;
  cli(); /* INTERRUPTS OFF */
  #if defined(MILLIS_USE_TIMERA0)
    ticks = TCA0.SPLIT.HCNT;
    flags = TCA0.SPLIT.INTFLAGS;
  #elif defined(MILLIS_USE_TIMERD0)
    TCD0.CTRLE = TCD_SCAPTUREA_bm;
    while (!(TCD0.STATUS & TCD_CMDRDY_bm)); //wait for sync - should be only one iteration of this loop
    flags = TCD0.INTFLAGS;
    ticks = TCD0.CAPTUREA;
  #else
    ticks = _timer->CNT;
    flags = _timer->INTFLAGS;
  #endif //end getting ticks
  /* If the timer overflow flag is raised, and the ticks we read are low, then the timer has rolled over but
   * ISR has not fired. If we already read a high value of ticks, either we read it just before the overflow,
   * so we shouldn't increment overflows, or interrupts are disabled and micros isn't expected to work so it
   * doesn't matter.
   * Get current number of overflows and timer count */
  #if !(defined(MILLIS_USE_TIMERB0) || defined(MILLIS_USE_TIMERB1))
    overflows = timer_overflow_count;
  #else
    overflows = timer_millis;
  #endif
  /* Turn interrupts back on, assuming they were on when micros was called. */
  SREG = oldSREG; /* INTERRUPTS ON */
  #if defined(MILLIS_USE_TIMERD0)
    if ((flags & TCD_OVF_bm) && (ticks < 0x07)) {
  #elif defined(MILLIS_USE_TIMERA0)
    ticks = (TIME_TRACKING_TIMER_PERIOD) - ticks;
    if ((flags & TCA_SPLIT_HUNF_bm) && (ticks < 0x4 )) {
  #else //timerb
    if ((flags & TCB_CAPT_bm) && !(ticks & 0xFF00)) {
  #endif
    #if ((defined(MILLIS_USE_TIMERB0) || defined(MILLIS_USE_TIMERB1)) && (F_CPU <= 1000000))
      overflows += 2;
    #else
      overflows++;
    #endif
  } //end getting ticks

  #if defined(MILLIS_USE_TIMERD0)
    #if (F_CPU==20000000UL || F_CPU==10000000UL || F_CPU==5000000UL)
      uint8_t ticks_l = ticks >> 1;
      ticks = ticks + ticks_l + ((ticks_l >> 2) - (ticks_l >> 4) + (ticks_l >> 7));
      // + ticks +(ticks>>1)+(ticks>>3)-(ticks>>5)+(ticks>>8))
      // speed optimization via doing math with smaller datatypes, since we know high byte is 1 or 0.
      microseconds =   overflows * (TIME_TRACKING_CYCLES_PER_OVF / 20) + ticks; //ticks value corrected above.
    #else
      microseconds = ((overflows * (TIME_TRACKING_CYCLES_PER_OVF / 16))
                        + (ticks * (TIME_TRACKING_CYCLES_PER_OVF / 16 / TIME_TRACKING_TIMER_PERIOD)));
    #endif
    #if defined(CLOCK_TUNE_INTERNAL) && !(F_CPU == 16000000UL || F_CPU ==  20000000UL || F_CPU ==  8000000UL || F_CPU ==  10000000UL || F_CPU ==  4000000UL || F_CPU ==  5000000UL)
      #warning "TCD is not supported as a millis timing source when the oscillator is tuned to a frequency other than 16 or 20 MHz. Timing results will be wrong - use TCA0 or a TCB."
    #endif
  #elif (defined(MILLIS_USE_TIMERB0)||defined(MILLIS_USE_TIMERB1))
    // ticks is 0 ~ F_CPU/2000 - 1
    // we shift 1, 2, 3, or 4 times it to the right ball park
    // I wonder how much could be gained from doing it stepwise (copy ticks to a union with byte[2], rightshift by 2, subtract
    // rightshift 2, add (we know high byte is 0, but it doesn't help because carry), rightshift low byte 2 more (low-only
    // saves 2 clocks), subtract low byte... wonder if it would be enough to do the final term faster than we currently do the
    // 4 term approximation? I think that would be exact! We'd also have equal + and -, reaping the reduced rounding noise
    // currently we end up with, it looks like, 0-995 (+/- 1) instead of 0-999? which I think we'd have with the last term.
    // Looked at generated assembly, no clue how they get there from here!
    #if   (F_CPU  == 30000000UL)
      ticks = ticks >> 4;
      microseconds = overflows * 1000 + (ticks + (ticks >> 3) - (ticks >> 4) + (ticks >> 8)); // Damned near perfect.
    #elif (F_CPU  == 25000000UL)
      ticks = ticks >> 4;
      microseconds = overflows * 1000 + (ticks + (ticks >> 2) - (ticks >> 5)); // Damned near perfect.
    #elif (F_CPU  == 20000000UL)
      ticks = ticks >> 3;
      microseconds = overflows * 1000 + (ticks - (ticks >> 2) + (ticks >> 4) - (ticks >> 6)); // + (ticks >> 8)
    #elif (F_CPU  == 10000000UL)
      ticks = ticks >> 2;
      microseconds = overflows * 1000 + (ticks - (ticks >> 2) + (ticks >> 4) - (ticks >> 6)); // + (ticks >> 8)
    #elif (F_CPU  ==  5000000UL)
      ticks = ticks >> 1;
      microseconds = overflows * 1000 + (ticks - (ticks >> 2) + (ticks >> 4) - (ticks >> 6)); // + (ticks >> 8)
    #elif (F_CPU  == 24000000UL)
      ticks = ticks >> 4;
      microseconds = overflows * 1000 + (ticks + (ticks >> 2) + (ticks >> 3) - (ticks >> 5)); // - (ticks >> 7)
    #elif (F_CPU  == 24000000UL)
      ticks = ticks >> 4;
      microseconds = overflows * 1000 + (ticks + (ticks >> 2) + (ticks >> 3) - (ticks >> 5)); // - (ticks >> 7)
    #elif (F_CPU  == 12000000UL)
      ticks = ticks >> 3;
      microseconds = overflows * 1000 + (ticks + (ticks >> 2) + (ticks >> 3) - (ticks >> 5)); // - (ticks >> 7)
    #elif (F_CPU  == 32000000UL || F_CPU > 24000000UL)
      microseconds = overflows * 1000 + (ticks >> 4);
    #elif (F_CPU  == 16000000UL || F_CPU > 12000000UL)
      microseconds = overflows * 1000 + (ticks >> 3);
    #elif (F_CPU  ==  8000000UL || F_CPU >  6000000UL)
      microseconds = overflows * 1000 + (ticks >> 2);
    #elif (F_CPU  ==  4000000UL || F_CPU >  3000000UL)
      microseconds = overflows * 1000 + (ticks >> 1);
    #else // (F_CPU  ==  1000000UL || F_CPU  == 2000000UL);
      microseconds = overflows * 1000 + ticks;
    #endif

    #if !(F_CPU == 32000000UL || F_CPU == 30000000UL || F_CPU == 25000000UL || F_CPU == 24000000UL || \
          F_CPU == 20000000UL || F_CPU == 16000000UL || F_CPU == 12000000UL || F_CPU == 10000000UL || \
          F_CPU ==  8000000UL || F_CPU ==  5000000UL || F_CPU ==  4000000UL || F_CPU ==  2000000UL || \
          F_CPU ==  1000000UL)
      #warning "Millis timer (TCBn) at this frequency is unsupported, micros() will return totally bogus values."
    #endif
  #else //TCA0
    #if (F_CPU == 20000000UL && TIME_TRACKING_TICKS_PER_OVF == 255 && TIME_TRACKING_TIMER_DIVIDER == 64)
      microseconds = (overflows * millisClockCyclesToMicroseconds(TIME_TRACKING_CYCLES_PER_OVF))
                   + (ticks * 3 + ((uint16_t)(ticks >> 2) - (ticks >> 4)));
                   // bafflingly, casting to a uint16_t makes the compiler generate more efficient code...  but
                   // casting to uint8_t doesn't! I don't understand it. but I'll take a free 8 bytes and 4 clocks any day.
                   // I also cannot fathom how the compiler generates what it does from this input....
    #elif (F_CPU == 10000000UL && TIME_TRACKING_TICKS_PER_OVF == 255 && TIME_TRACKING_TIMER_DIVIDER == 64)
      microseconds = (overflows * millisClockCyclesToMicroseconds(TIME_TRACKING_CYCLES_PER_OVF))
                   + (ticks * 3 + ((uint16_t)(ticks >> 1) - (ticks >> 3)));
    #elif (F_CPU == 5000000UL && TIME_TRACKING_TICKS_PER_OVF == 255 && TIME_TRACKING_TIMER_DIVIDER == 16)
      microseconds = (overflows * millisClockCyclesToMicroseconds(TIME_TRACKING_CYCLES_PER_OVF))
                   + (ticks * 3 + ((uint16_t)(ticks >> 2) - (ticks >> 4)));
    #else
      #if (TIME_TRACKING_TIMER_DIVIDER%(F_CPU/1000000))
        #warning "Millis timer (TCA0) at this frequency unsupported, micros() will return bogus values."
      #endif
      microseconds = ((overflows * millisClockCyclesToMicroseconds(TIME_TRACKING_CYCLES_PER_OVF))
                    + (ticks * (millisClockCyclesToMicroseconds(TIME_TRACKING_CYCLES_PER_OVF) / TIME_TRACKING_TIMER_PERIOD)));
    #endif
  #endif //end of timer-specific part of micros calculations
  return microseconds;
}
#endif //end of non-RTC micros code
#endif //end of non-MILLIS_USE_TIMERNONE code

/* So what do you WANT in a good delay function?
    First, obviously you want it to delay things. You do not want it to block interruots (then a long one would throw off
    timekeeping, miss inputs, and so on). And you want the compiled size to not be prohibitive for the part.
 * the reason it's so important wrt. interrupts is that in Arduino standard delay(), if an interrupt fires in the middle,
    will still end at the same time - it is "interrupt insensitive". Whenever a delay is using the builtin _delay_ms()
    if that is interrupted it has no way of knowing timke has passed. Now hopefully you're not spending so much time in
    an ISR that this is significant, but it is still undesirable.
 * For the unfortunate souls using 4k and 2k parts, the flash usage becomes a major problem - why is it such a space-hog?
    Because it has to pull in micros(), which is bulky even with the division turned into bitshifts... And with RTC millis,
    millis() is bad for the same reason: the conversion of 1024 to 1000 is a killer....
Now we will use one of three delay() implementations:
 * If you have 16k+ your delay is the standard one, it pulls in micros(), yes, but you may well already have grabbed
    that for your sketch already, and the delay is more accurate and fully interrupt insensitive, and you can afford
    the memory. For RTC users they will get the analogous implementation that is based on millis.
 * Users with millis disabled, or with less than 16k flash and using RTC will get the implementation based on _delay_ms().
 * Everyone else (flash under 16k but millis enabled via non-RTC timer) will get the light version which calls _delay_ms()
    if the delay is under 16 ms to get less flash usage, and calculates the delay using **millis** not micros otherwise,
    saving over 100b of flash. The reason for the split is that the limited granularity of millis introduces an error in
    the delay duration of up to 1ms. That doesn't matter much when you call delay(1000) on an internal clock that's within
    1% on a good day. It matters greatly when you call delay(1);
*/




#if defined(MILLIS_USE_TIMERNONE) || (PROGMEM_SIZE < 16384 && defined(MILLIS_USE_TIMERRTC))
  void delay(uint32_t ms) {
    if (__builtin_constant_p(ms)) {
      _delay_ms(ms);
    } else {
      while (ms--) {
        _delay_ms(1);
      }
    }
  }
#elif (PROGMEM_SIZE >= 16384 && !defined(MILLIS_USE_TIMERRTC))
  void delay(uint32_t ms)
  {
    uint16_t start = (uint16_t)micros();
    while (ms > 0) {
      while (((uint16_t)micros() - start) >= 1000 && ms) {
        ms-- ;
        start += 1000;
      }
    }
  }
#else
  void delay(uint32_t ms) {
    if (__builtin_constant_p(ms) && ms < 16) {
      _delay_ms(ms);
    } else if (ms < 16) {
      while(ms--) {
        _delay_ms(1);
      }
    } else {
      uint32_t start=millis();
      while (millis() - start < ms);
    }
  }
#endif
  /* Historical method without millis
  void delay(uint32_t ms) {
    if (__builtin_constant_p(ms)) {
      _delay_ms(ms);
    } else {
      while (ms--) {
        delayMicroseconds(1000);
      }
    }
  }*/

inline __attribute__((always_inline)) void delayMicroseconds(unsigned int us) {
  // This function gets optimized away, but to what depends on whether us is constant.
  if (__builtin_constant_p(us)) {
    _delay_us(us); // Constant microseconds use the avr-libc _delay_us() which is highly accurate for all values and efficient.
  } else { // If it is not, we have to use the Arduino style implementation.
    _delayMicroseconds(us);
  }
}
/* Delay for the given number of microseconds. This is UGLY AS SIN and explicitly depends on function call overhead for
 * very short delays.
 * First, we get 1 us in and return if that's what was asked for.
 * Then we use a minimal number of bitshifts to calculate the number of passes through the delay loop
 * and subtract the number of loop-cycles of time we burned doing so. But need to be careful that sane values
 * don't get so much bigger that they overflow the unsigned int we're storing it in. To that end, we use
 * a longer loop at faster clock speeds.
 * In the inline assembly, when a delay of 8 clocks or longer is required, we save flash with a clever trick:
 *  "rjmp .+2" "\n\t"     // 2 cycles - jump over the return.
 *  "ret" "\n\t"          // 4 cycles - rjmped over initially...
 *  "rcall .-4" "\n\t"    // 2 cycles - ... but then called here...
 * This exploits the fact that return is a 4-clock instruction (even on AVRxt) by first hopping over a return
 * then immediately calling that return instruction - 8 clocks in 3 words. Once the ret is there, additional
 * rcall instructions can get 6 clocks in a single word, though we only get to take advantage of that once for the 30 MHz case.
 */

#if F_CPU >= 32000000L
  // 16 MHz math, 8-cycle loop
  #define DELAYMICROS_EIGHT
#elif F_CPU >= 30000000L
  // 12 MHz math, 10-cycle loop
  #define DELAYMICROS_TEN
#elif F_CPU >= 28000000L
  // 16 MHz math, 7-cycle loop
  #define DELAYMICROS_SEVEN
#elif F_CPU >= 24000000L
  // 12 MHz math, 8-cycle loop
  #define DELAYMICROS_EIGHT
#endif

__attribute__ ((noinline)) void _delayMicroseconds(unsigned int us) {
  // call = 4 cycles + 4 cycles to init us
  #if F_CPU >= 32000000L
  // here, we only take half a us at the start!
  __asm__ __volatile__ ("rjmp .+0"); //just waiting 2 cycles - so we're half a us in

  // the loop takes 1/4 of a microsecond (8 cycles)
  // per iteration, so execute it four times for each microsecond of
  // delay requested.
  us <<= 2; // x4 us, = 4 cycles

  // account for the time taken in the preceding commands.
  // we only burned ~14 cycles above, subtraction takes another 2 - so we've lost half a us,
  // and only need to drop 2 rounds through the loop!
  us -= 2; // = 2 cycles


#elif F_CPU >= 30000000L
  // for the 30 MHz clock

  // for a one-microsecond delay, burn 14 cycles and return
  __asm__ __volatile__ (
    "rjmp .+2" "\n\t"     // 2 cycles - jump over the return.
    "ret" "\n\t"          // 4 cycles - rjmped over initially...
    "rcall .-4" "\n\t"    // 2 cycles - ... but then called here...
    "rcall .-6");         // 2+4 cycles - ... and here again!
                          // Waiting 14 cycles - in only 4 words!
  if (us <= 1) return; // = 3 cycles, (4 when true)

  // the loop takes 1/3 of a microsecond (10 cycles)
  // per iteration, so execute it three times for each microsecond of
  // delay requested.
  us = (us << 1) + us; // x3 us, = 5 cycles

  // account for the time taken in the preceding commands.
  // we just burned 28 (30) cycles above, remove 3
  us -= 3; //2 cycles

#elif F_CPU >= 28000000L

  // for a one-microsecond delay, burn 12 cycles and return
  __asm__ __volatile__ (
    "rjmp .+2" "\n\t"     // 2 cycles - jump over next instruction.
    "ret" "\n\t"          // 4 cycles - rjmped over initially....
    "rcall .-4" "\n\t"    // 2 cycles - ... but then called here);
    "rjmp .+0" "\n\t"     // 2 cycles
    "rjmp .+0");          // 2 cycles
                          // Wait 12 cycles with 5 words
  if (us <= 1) return; //  = 3 cycles, (4 when true)

  // the loop takes 1/4 of a microsecond (7 cycles)
  // per iteration, so execute it four times for each microsecond of
  // delay requested.
  us <<= 2; // x4 us, = 4 cycles

  // account for the time taken in the preceding commands.
  // we just burned 27 (29) cycles above, remove 4, (7*4=28)
  // us is at least 8 so we can subtract 5
  us -= 4; // = 2 cycles,


#elif F_CPU >= 24000000L
  // for the 24 MHz clock

  // for a one-microsecond delay, burn 8 cycles and return
  __asm__ __volatile__ (
    "rjmp .+2" "\n\t"     // 2 cycles - jump over next instruction.
    "ret" "\n\t"          // 4 cycles - rjmped over initially....
    "rcall .-4");         // 2 cycles - ... but then called here);
                          // wait 8 cycles with 3 words
  if (us <= 1) return;    //  = 3 cycles, (4 when true)

  // the loop takes 1/3 of a microsecond (8 cycles)
  // per iteration, so execute it three times for each microsecond of
  // delay requested.
  us = (us << 1) + us; // x3 us, = 5 cycles

  // account for the time taken in the preceding commands.
  // we just burned 24 (22) cycles above, remove 3
  us -= 3; //2 cycles


#elif F_CPU >= 20000000L
  // for a one-microsecond delay, simply return.  the overhead
  // of the function call takes 18 (20) cycles, which is 1us
  __asm__ __volatile__(
    "rjmp .+0" "\n\t"
    "rjmp .+0" "\n\t"); //just waiting 4 cycles
  if (us <= 1) {
    return;  //  = 3 cycles, (4 when true)
  }

  // the loop takes a 1/5 of a microsecond (4 cycles)
  // per iteration, so execute it five times for each microsecond of
  // delay requested.
  us = (us << 2) + us; // x5 us, = 7 cycles

  // account for the time taken in the preceding commands.
  // we just burned 26 (28) cycles above, remove 7, (7*4=28)
  // us is at least 10 so we can subtract 7
  us -= 7; // 2 cycles

  #elif F_CPU >= 16000000L
  // for the 16 MHz clock on most Arduino boards

  // for a one-microsecond delay, simply return.  the overhead
  // of the function call takes 14 (16) cycles, which is 1us
  if (us <= 1) {
    return;  //  = 3 cycles, (4 when true)
  }

  // the loop takes 1/4 of a microsecond (4 cycles)
  // per iteration, so execute it four times for each microsecond of
  // delay requested.
  us <<= 2; // x4 us, = 4 cycles

  // account for the time taken in the preceding commands.
  // we just burned 19 (21) cycles above, remove 5, (5*4=20)
  // us is at least 8 so we can subtract 5
  us -= 5; // = 2 cycles,

  #elif F_CPU >= 10000000L
  // for 10MHz (20MHz/2)

  // for a 1 microsecond delay, simply return.  the overhead
  // of the function call takes 14 (16) cycles, which is 1.5us
  if (us <= 1) {
    return;  //  = 3 cycles, (4 when true)
  }

  // the loop takes 2/5 of a microsecond (4 cycles)
  // per iteration, so execute it 2.5 times for each microsecond of
  // delay requested.
  us = (us << 1) + (us >> 1); // x2.5 us, = 5 cycles

  // account for the time taken in the preceding commands.
  // we just burned 20 (22) cycles above, remove 5, (5*4=20)
  // us is at least 6 so we can subtract 5
  us -= 5; //2 cycles

  #elif F_CPU >= 8000000L
  // for the 8 MHz internal clock

  // for a 1 and 2 microsecond delay, simply return.  the overhead
  // of the function call takes 14 (16) cycles, which is 2us
  if (us <= 2) {
    return;  //  = 3 cycles, (4 when true)
  }

  // the loop takes 1/2 of a microsecond (4 cycles)
  // per iteration, so execute it twice for each microsecond of
  // delay requested.
  us <<= 1; //x2 us, = 2 cycles

  // account for the time taken in the preceding commands.
  // we just burned 17 (19) cycles above, remove 4, (4*4=16)
  // us is at least 6 so we can subtract 4
  us -= 4; // = 2 cycles
  #elif F_CPU >= 5000000L
  // for 5MHz (20MHz / 4)

  // for a 1 ~ 3 microsecond delay, simply return.  the overhead
  // of the function call takes 14 (16) cycles, which is 3us
  if (us <= 3) {
    return;  //  = 3 cycles, (4 when true)
  }

  // the loop takes 4/5th microsecond (4 cycles)
  // per iteration, so we want to add it to 1/4th of itself
  us += us >> 2;
  us -= 2; // = 2 cycles

  #elif F_CPU >= 4000000L
  // for that unusual 4mhz clock...

  // for a 1 ~ 4 microsecond delay, simply return.  the overhead
  // of the function call takes 14 (16) cycles, which is 4us
  if (us <= 4) {
    return;  //  = 3 cycles, (4 when true)
  }

  // the loop takes 1 microsecond (4 cycles)
  // per iteration, so nothing to do here! \o/

  us -= 2; // = 2 cycles


  #else
  // for the 1 MHz internal clock (default settings for common AVR microcontrollers)
  // the overhead of the function calls is 14 (16) cycles
  if (us <= 16) {
    return;  //= 3 cycles, (4 when true)
  }
  if (us <= 25) {
    return;  //= 3 cycles, (4 when true), (must be at least 25 if we want to subtract 22)
  }

  // compensate for the time taken by the preceding and next commands (about 22 cycles)
  us -= 22; // = 2 cycles
  // the loop takes 4 microseconds (4 cycles)
  // per iteration, so execute it us/4 times
  // us is at least 4, divided by 4 gives us 1 (no zero delay bug)
  us >>= 2; // us div 4, = 4 cycles


  #endif


#if defined(DELAYMICROS_TWELVE)
  __asm__ __volatile__ (
    "1: sbiw %0,1" "\n\t" // 2 cycles
    "rjmp .+2" "\n\t"     // 2 cycles - jump over next instruction.
    "ret" "\n\t"          // 4 cycles - rjmped over initially....
    "rcall .-4" "\n\t"    // 2 cycles - ... but then called here
    "brne 1b" : "=w" (us) : "0" (us) // 2 cycles
  );
#elif defined(DELAYMICROS_TEN)
  __asm__ __volatile__ (
    "1: sbiw %0,1" "\n\t" // 2 cycles
    "rjmp .+0" "\n\t"     // 2 cycles each;
    "rjmp .+0" "\n\t"     // 2 cycles each;
    "rjmp .+0" "\n\t"     // 2 cycles each;
    "brne 1b" : "=w" (us) : "0" (us) // 2 cycles
  );
#elif defined(DELAYMICROS_EIGHT)
  __asm__ __volatile__ (
    "1: sbiw %0,1" "\n\t" // 2 cycles
    "rjmp .+0" "\n\t"     // 2 cycles each;
    "rjmp .+0" "\n\t"     // 2 cycles each;
    "brne 1b" : "=w" (us) : "0" (us) // 2 cycles
  );
#elif defined(DELAYMICROS_SEVEN)
  __asm__ __volatile__ (
    "1: sbiw %0,1" "\n\t" // 2 cycles
    "rjmp .+0" "\n\t"     // 2 cycles
    "nop"      "\n\t"     // 1 cyc;e
    "brne 1b" : "=w" (us) : "0" (us) // 2 cycles
  );
#else
  // the classic 4 cycle delay loop...
  // busy wait
  __asm__ __volatile__ (
    "1: sbiw %0,1" "\n\t" // 2 cycles
    "brne 1b" : "=w" (us) : "0" (us) // 2 cycles
  );
#endif
  // return = 4 cycles
}


void stop_millis()
{ // Disable the interrupt:
  #if defined(MILLIS_USE_TIMERNONE)
    badCall("stop_millis() is only valid with millis time keeping enabled.");
  #else
    #if defined(MILLIS_USE_TIMERA0)
      TCA0.SPLIT.INTCTRL &= (~TCA_SPLIT_HUNF_bm);
    #elif defined(MILLIS_USE_TIMERA1)
      TCA1.SPLIT.INTCTRL &= (~TCA_SPLIT_HUNF_bm);
    #elif defined(MILLIS_USE_TIMERD0)
      TCD0.INTCTRL&=0xFE;
    #elif defined(MILLIS_USE_TIMERRTC)
      RTC.INTCTRL&=0xFE;
      RTC.CTRLA&=0xFE;
    #else
      _timer->INTCTRL &= ~TCB_CAPT_bm;
    #endif
  #endif
}



void restart_millis()
{
  // Call this to restart millis after it has been stopped and/or millis timer has been molested by other routines.
  // This resets key registers to their expected states.
  #if defined(MILLIS_USE_TIMERNONE)
    badCall("restart_millis() is only valid with millis time keeping enabled.");
  #else
    #if defined(MILLIS_USE_TIMERA0)
        /* The type A timers need to be restored to the state they were in at the start of restore  */
      TCA0.SPLIT.CTRLA = 0;                       // timer off (might need that for next steps)
      TCA0.SPLIT.CTRLD    = TCA_SPLIT_SPLITM_bm;  // because this will not work if it's enabled.
      TCA0.SPLIT.HPER     = PWM_TIMER_PERIOD;     // What was left behind
      #if (F_CPU > 25000000) //   use 256 divider when clocked over 25 MHz
        TCA0.SPLIT.CTRLA   = (TCA_SPLIT_CLKSEL_DIV256_gc) | (TCA_SPLIT_ENABLE_bm);
      #elif (F_CPU > 5000000) //  use 64 divider for everything in the middle
        TCA0.SPLIT.CTRLA   =  (TCA_SPLIT_CLKSEL_DIV64_gc) | (TCA_SPLIT_ENABLE_bm);
      #elif (F_CPU > 1000000) // and use 16...
        TCA0.SPLIT.CTRLA   =  (TCA_SPLIT_CLKSEL_DIV16_gc) | (TCA_SPLIT_ENABLE_bm);
      #else                   // or even 8 otherwise for really slow system clocks.
        TCA0.SPLIT.CTRLA   =   (TCA_SPLIT_CLKSEL_DIV8_gc) | (TCA_SPLIT_ENABLE_bm);
      #endif
    #elif defined(MILLIS_USE_TIMERA1)
        /* Use prescale appropriate for system clock speed */

      #if (F_CPU > 25000000) //   use 256 divider when clocked over 25 MHz
        TCA1.SPLIT.CTRLA   = (TCA_SPLIT_CLKSEL_DIV256_gc) | (TCA_SPLIT_ENABLE_bm);
      #elif (F_CPU > 5000000) //  use 64 divider for everything in the middle
        TCA1.SPLIT.CTRLA   =  (TCA_SPLIT_CLKSEL_DIV64_gc) | (TCA_SPLIT_ENABLE_bm);
      #elif (F_CPU > 1000000) // and use 16...
        TCA1.SPLIT.CTRLA   =  (TCA_SPLIT_CLKSEL_DIV16_gc) | (TCA_SPLIT_ENABLE_bm);
      #else                   // or even 8 otherwise for really slow system clocks.
        TCA1.SPLIT.CTRLA   =   (TCA_SPLIT_CLKSEL_DIV8_gc) | (TCA_SPLIT_ENABLE_bm);
      #endif
      TCA1.SPLIT.CTRLD    = TCA_SPLIT_SPLITM_bm;
      TCA1.SPLIT.HPER     = PWM_TIMER_PERIOD;
    #elif defined(MILLIS_USE_TIMERD0)
      TCD0.CTRLA          = 0x00;
      while (TCD0.STATUS & 0x01);
    #elif (defined(MILLIS_USE_TIMERB0) || defined(MILLIS_USE_TIMERB1) || defined(MILLIS_USE_TIMERB2) || defined(MILLIS_USE_TIMERB3) || defined(MILLIS_USE_TIMERB4)) //It's a type b timer
      _timer->CTRLB       = 0;
    #endif
    init_millis();
  #endif
}




void __attribute__((weak)) init_millis()
{
  #if defined(MILLIS_USE_TIMERNONE)
    badCall("init_millis() is only valid with millis time keeping enabled.");
  #else
    #if defined(MILLIS_USE_TIMERA0)
      TCA0.SPLIT.INTCTRL |= TCA_SPLIT_HUNF_bm;
    #elif defined(MILLIS_USE_TIMERA1)
      TCA1.SPLIT.INTCTRL |= TCA_SPLIT_HUNF_bm;
    #elif defined(MILLIS_USE_TIMERD0)
      TCD0.CMPBCLR        = TIME_TRACKING_TIMER_PERIOD; //essentially, this is TOP
      TCD0.CTRLB          = 0x00; // oneramp mode
      TCD0.CTRLC          = 0x80;
      TCD0.INTCTRL        = 0x01; // enable interrupt
      TCD0.CTRLA          = TIMERD0_PRESCALER | 0x01; //set clock source and enable!
    #elif defined(MILLIS_USE_TIMERRTC)
      while(RTC.STATUS); //if RTC is currently busy, spin until it's not.
      // to do: add support for RTC timer initialization
      RTC.PER             = 0xFFFF;
      #ifdef MILLIS_USE_TIMERRTC_XTAL
        _PROTECTED_WRITE(CLKCTRL.XOSC32KCTRLA,0x03);
        RTC.CLKSEL        = 2; //external crystal
      #else
        _PROTECTED_WRITE(CLKCTRL.OSC32KCTRLA,0x02);
        //RTC.CLKSEL=0; this is the power on value
      #endif
      RTC.INTCTRL         = 0x01; //enable overflow interrupt
      RTC.CTRLA           = (RTC_RUNSTDBY_bm|RTC_RTCEN_bm|RTC_PRESCALER_DIV32_gc);//fire it up, prescale by 32.
    #else //It's a type b timer - we have already errored out if that wasn't defined
      _timer->CCMP = TIME_TRACKING_TIMER_PERIOD;
      // Enable timer interrupt, but clear the rest of register
      _timer->INTCTRL = TCB_CAPT_bm;
      // Clear timer mode (since it will have been set as PWM by init())
      _timer->CTRLB=0;
      // CLK_PER/1 is 0b00,. CLK_PER/2 is 0b01, so bitwise OR of valid divider with enable works
      _timer->CTRLA = TIME_TRACKING_TIMER_DIVIDER|TCB_ENABLE_bm;  // Keep this last before enabling interrupts to ensure tracking as accurate as possible
    #endif
  #endif
}

void set_millis(__attribute__((unused))uint32_t newmillis)
{
  #if defined(MILLIS_USE_TIMERNONE)
    badCall("set_millis() is only valid with millis timekeeping enabled.");
  #else
    #if defined(MILLIS_USE_TIMERRTC)
      // timer_overflow_count = newmillis >> 16;
      // millis = 61/64(timer_overflow_count << 16 + RTC.CNT)
      uint8_t oldSREG=SREG; // save SREG
      cli();                // interrupts off
      uint16_t temp=(newmillis % 61) << 6;
      newmillis=(newmillis / 61) << 6;
      temp=temp / 61;
      newmillis += temp;
      timer_overflow_count = newmillis >> 16;
      while(RTC.STATUS&RTC_CNTBUSY_bm); //wait if RTC busy
      RTC.CNT = newmillis & 0xFFFF;
      SREG = oldSREG; // reemable oimterripts if we killed them,
    #else
      /* farting around with micros via overflow count was ugly and buggy.
       * may implement again, better, in the future - but millis and micros
       * will get out of sync when you use set_millis
       * I think the way to do it is to make this implementation (but not big one)
       * inline, so if newmillis is constant, we can calculate the (compile-time known)
       * number of overflows using all the floating point math we want, and otherwise,
       * document that it will zero out micros.*/
      timer_millis = newmillis;
    #endif
  #endif
}


void init() {
  // Initializes hardware: First we configure the main clock, then fire up the other peripherals
  init_clock();
  init_ADC0();

  init_timers();

  #ifndef MILLIS_USE_TIMERNONE
    init_millis();
  #endif
  /*************************** ENABLE GLOBAL INTERRUPTS *************************/
  // Finally, after everything is initialized, we go ahead and enable interrupts.
  sei();
}

  /******************************** CLOCK STUFF *********************************/
#if defined(CLOCK_TUNE_INTERNAL)
  void tune_internal(void);
#endif

void __attribute__((weak)) init_clock() {
  #ifndef CLOCK_SOURCE
    #error "CLOCK_SOURCE not defined. CLOCK_SOURCE must be either 0 (internal) or 2 (external clock)"
  #endif
  #if (CLOCK_SOURCE==0)
    #if (defined(CLOCK_TUNE_INTERNAL))
      tune_internal(); // Will be inlined as only called once. Just too long and ugly to put two implementations in middle of this.
    #else
      #if (F_CPU == 20000000)
        /* No division on clock */
        _PROTECTED_WRITE(CLKCTRL_MCLKCTRLB, 0x00);
      #elif (F_CPU == 16000000)
        /* No division on clock */
        _PROTECTED_WRITE(CLKCTRL_MCLKCTRLB, 0x00);
      #elif (F_CPU == 10000000) //20MHz prescaled by 2
        /* Clock DIV2 */
        _PROTECTED_WRITE(CLKCTRL_MCLKCTRLB, (CLKCTRL_PEN_bm | CLKCTRL_PDIV_2X_gc));
      #elif (F_CPU == 8000000) //16MHz prescaled by 2
        /* Clock DIV2 */
        _PROTECTED_WRITE(CLKCTRL_MCLKCTRLB, (CLKCTRL_PEN_bm | CLKCTRL_PDIV_2X_gc));
      #elif (F_CPU == 5000000) //20MHz prescaled by 4
        /* Clock DIV4 */
        _PROTECTED_WRITE(CLKCTRL_MCLKCTRLB, (CLKCTRL_PEN_bm | CLKCTRL_PDIV_4X_gc));
      #elif (F_CPU == 4000000) //16MHz prescaled by 4
        /* Clock DIV4 */
        _PROTECTED_WRITE(CLKCTRL_MCLKCTRLB, (CLKCTRL_PEN_bm | CLKCTRL_PDIV_4X_gc));
      #elif (F_CPU == 1000000) //16MHz prescaled by 16
        /* Clock DIV16 */
        _PROTECTED_WRITE(CLKCTRL_MCLKCTRLB, (CLKCTRL_PEN_bm | CLKCTRL_PDIV_16X_gc));
      #else
        #ifndef F_CPU
          #error "F_CPU not defined"
        #else
          #error "F_CPU defined as an unsupported value for untuned internal oscillator"
        #endif
      #endif
    #endif
  #elif (CLOCK_SOURCE==2)
    _PROTECTED_WRITE(CLKCTRL_MCLKCTRLA, CLKCTRL_CLKSEL_EXTCLK_gc);
    // while (CLKCTRL.MCLKSTATUS & CLKCTRL_SOSC_bm);  //This either works, or hangs the chip - EXTS is pretty much useless here.
    // w/out CFD, easier to determine what happened if we don't just hang here.
    uint8_t count=10;
    while (CLKCTRL.MCLKSTATUS & CLKCTRL_SOSC_bm && count--);
    if (CLKCTRL.MCLKSTATUS & CLKCTRL_EXTS_bm) {
      _PROTECTED_WRITE(CLKCTRL_MCLKCTRLB, 0x00);
    }
  #else
    #error "CLOCK_SOURCE is defined, but it isn't 0 (internal) or 2 (external clock), and those are the only clock sources supported by this part."
  #endif
}




#if defined(CLOCK_TUNE_INTERNAL)
  #include "tune_guesses.h"
  void tune_internal() {
    uint8_t osccfg=FUSE.OSCCFG-1; /****** THIS IS A MAGIC NAME - DO NOT CHANGE IT ******/
    if (TUNED_CALIBRATION_OFFSET == -1) {
      GPIOR0 |= 0x80;
      GPIOR0 |= 0x40;
      return; //we can't do that speed at all with this part and oscillator setting! Hopefully users notice their sketch is running
      // way too slow, and will read the docs which contain further instructions for diagnosis of these sort of problems.
    } else {
      uint8_t tunedval=_SFR_MEM8(osccfg ? (0x1306 +  CLOCK_TUNE_START + TUNED_CALIBRATION_OFFSET) : (0x1300 +  CLOCK_TUNE_START + TUNED_CALIBRATION_OFFSET));
      if (tunedval == 0xFF) {
        GPIOR0 |= 0x40;
        int temp = GUESSCAL;
        if (temp > MAX_TUNING) {             // uhoh, if we apply the default guess, we'd be setting it higher than it's maximum value!
          if (MAX_TUNING - temp > 5) return; // How far away are we? If it's more than 5, give up - better to be obviously broken than non-obviously broken
          tunedval = MAX_TUNING;
        } else if (temp < 0) {               // uhoh, if we apply the default guess, we'd be setting it to a negative value (which would wrap around, resulting in the value being too high..
          if (temp < -5) return;             // How far away are we? If it's more than 5, give up - better to be obviously broken than non-obviously broken
          tunedval = 0;
        } else {
          tunedval = temp;
        }
      } else if (tunedval & 0x80) {
        GPIOR0 |= 0x80;
        return; // this chip was tuned and it's oscillator found to be unable to reach target and/or the chip ceased to be opprate before reaching that speed
        // such that either the tuning sketch crashed or the incredilbly crude sanity check turned up an error.
      }
      _PROTECTED_WRITE(CLKCTRL_OSC20MCALIBA,tunedval);
      _NOP();
    }
    _PROTECTED_WRITE(CLKCTRL_MCLKCTRLB, TUNE_PRESCALE);
  }
#endif


/********************************* ADC ****************************************/
void __attribute__((weak)) init_ADC0() {
  #if MEGATINYCORE_SERIES < 2
  /* ADC clock 1 MHz to 1.25 MHz at frequencies supported by megaTinyCore
   * Unlike the classic AVRs, which demand 50~200 kHz, for these, the datasheet
   * spec's 50 kHz to 1.5 MHz. Slower clocks provide better response to high
   * impedance signals, since the sample and hold circuit will be connected
   * to the pin for longer However, we can use the SAMPLEN register to
   * compensate for this!
   * We target a sampling time of 12us, which is a little shorter than the
   * classic AVRs, but the sampling cap is is 5pf instead of 14
   * At clock speeds of 12, 24, and 25 MHz when we are forced to use an divider
   * that leaves us with a markedly slower ADC clock (~750 kHz), we instead use
   * a value of 7, giving us 8 ADC clocks or... around 12us. .
   * As of 2.3.0, this setting is exposed by analogReadDuration()
   * Note that on 0/1-series, the prescale settings are placed powers-of-two
   * apart. On the 2-series and Dx-series, they are MUCH closer together.
   **************************************************************************/
    // 25 MHz / 32 = 780 kHz, 30 MHz / 32 = 937 kHz,  32 MHz / 32 =  1 MHz.
    #if   F_CPU   > 24000000    // 24 MHz / 16 = 1.5 MHz,  25 MHz / 32 =  780 kHz
      ADC0.CTRLC  = ADC_PRESC_DIV32_gc | ADC_REFSEL_VDDREF_gc | ADC_SAMPCAP_bm;
    #elif F_CPU  >= 12000000    // 16 MHz / 16 = 1.0 MHz,  20 MHz / 16 = 1.25 MHz
      ADC0.CTRLC  = ADC_PRESC_DIV16_gc | ADC_REFSEL_VDDREF_gc | ADC_SAMPCAP_bm;
    #elif F_CPU  >=  6000000    //  8 MHz /  8 = 1.0 MHz,  10 MHz /  8 = 1.25 MHz
      ADC0.CTRLC  =  ADC_PRESC_DIV8_gc | ADC_REFSEL_VDDREF_gc | ADC_SAMPCAP_bm;
    #elif F_CPU  >=  3000000    //  4 MHz /  4 = 1.0 MHz,   5 MHz /  4 = 1.25 MHz
      ADC0.CTRLC  =  ADC_PRESC_DIV4_gc | ADC_REFSEL_VDDREF_gc | ADC_SAMPCAP_bm;
    #else                       //  1 MHz /  2 = 500 kHz - the lowest setting
      ADC0.CTRLC  =  ADC_PRESC_DIV2_gc | ADC_REFSEL_VDDREF_gc | ADC_SAMPCAP_bm;
    #endif
    #if   (F_CPU == 6000000 || F_CPU == 12000000 || F_CPU == 24000000 || F_CPU ==25000000)
      ADC0.SAMPCTRL = (7); // 9 ADC clocks, 12 us
    #elif (F_CPU == 5000000 || F_CPU == 10000000 || F_CPU == 20000000 )
      ADC0.SAMPCTRL = 13;   // 15 ADC clock,s 12 us
    #else
      ADC0.SAMPCTRL = (10); // 12 ADC clocks, 12 us
    #endif
    ADC0.CTRLD    = ADC_INITDLY_DLY16_gc;
    ADC0.CTRLA    = ADC_ENABLE_bm;
  #else
    /* On the 2-series maximum with internal reference is 3 MHz, so we will
     * target highest speed that doesn't exceed that and 16 ADC clocks sample
     * duration. */
    #if F_CPU     > 32000000            // 36 MHz /14 = 2.57 MHz
      ADC0.CTRLB  = ADC_PRESC_DIV10_gc; // 33 MHz /14 = 2.35 MHz
    #elif F_CPU  >= 30000000            // 32 MHz /12 = 2.67 MHz
      ADC0.CTRLB  = ADC_PRESC_DIV12_gc; // 30 MHz /12 = 2.50 MHz
    #elif F_CPU  >= 24000000            // 25 MHz /10 = 2.50 MHz
      ADC0.CTRLB  = ADC_PRESC_DIV10_gc; // 24 MHz /10 = 2.40 MHz
    #elif F_CPU  >= 20000000
      ADC0.CTRLB  = ADC_PRESC_DIV8_gc;  // 20 MHz / 8 = 2.50 MHz
    #elif F_CPU  >= 16000000
      ADC0.CTRLB  = ADC_PRESC_DIV6_gc;  // 16 MHz / 6 = 2.67 MHz
    #elif F_CPU  >= 12000000
      ADC0.CTRLB  = ADC_PRESC_DIV4_gc;  // 12 MHz / 4 = 3.00 MHz
    #elif F_CPU  >=  6000000            // 10 MHz / 4 = 2.50 MHz
      ADC0.CTRLB  = ADC_PRESC_DIV4_gc;  //  8 MHz / 4 = 2.00 MHz
    #else                               //  5 MHz / 2 = 2.50 MHz
      ADC0.CTRLB  = ADC_PRESC_DIV2_gc;  //  4 MHz / 2 = 2.00 MHz
    #endif                              //  1 MHz / 2 =  500 kHz
    ADC0.CTRLE = 15; //15.5 without PGA, 16 with PGA, corresponding to 7.75 or 8 us.
    ADC0.CTRLA = ADC_ENABLE_bm | ADC_LOWLAT_bm;
    /* Default low latency mode on
     * Users can turn it off if they care about power consumption while ADC is on
     * and chip is awake, since these parts don't have the perverse ADC-left-on
     * behavior of classic AVRs. */
    ADC0.CTRLC = TIMEBASE_1US; //defined in Arduino.h.
    ADC0.PGACTRL = ADC_PGABIASSEL_3_4X_gc | ADC_ADCPGASAMPDUR_15CLK_gc;
    /* Note that we don't *enable* it automatically in init().
     * 3/4th bias is good up to 4 MHz CLK_ADC, 15 ADC Clocks to sample the PGA
     * up to 5 MHz, so within the regime of speeds that have to be compatible
     * with internal references, we are in the clear there. */
  #endif
}


#ifdef ADC1
  __attribute__((weak)) void init_ADC1() {
  #if   F_CPU > 24000000    // 24 MHz / 16 = 1.5 MHz,  25 MHz / 32 =  780 kHz
    ADC0.CTRLC  = ADC_PRESC_DIV32_gc | ADC_REFSEL_VDDREF_gc | ADC_SAMPCAP_bm;
  #elif F_CPU >= 12000000 // 16 MHz / 16 = 1 MHz,  20 MHz / 16 = 1.25 MHz
  ADC1.CTRLC = ADC_PRESC_DIV16_gc | ADC_REFSEL_VDDREF_gc | ADC_SAMPCAP_bm;;
  #elif F_CPU >= 6000000 // 8 MHz / 8 = 1 MHz, 10 MHz / 64 = 1.25 MHz
  ADC1.CTRLC = ADC_PRESC_DIV8_gc | ADC_REFSEL_VDDREF_gc | ADC_SAMPCAP_bm;;
  #elif F_CPU >= 3000000 // 4 MHz / 32 = 1 MHz, 5 MHz / 32 = 1.25 MHz
  ADC1.CTRLC = ADC_PRESC_DIV4_gc | ADC_REFSEL_VDDREF_gc | ADC_SAMPCAP_bm;;
  #else  // 1 MHz / 2 = 500 kHz - the lowest setting
  ADC1.CTRLC = ADC_PRESC_DIV2_gc | ADC_REFSEL_VDDREF_gc | ADC_SAMPCAP_bm;;
  #endif
  ADC1.SAMPCTRL = 14; //16 ADC clock sampling time - should be about the same amount of *time* as originally?
  ADC1.CTRLD = ADC_INITDLY_DLY16_gc;
  /* Enable ADC */
  ADC1.CTRLA |= ADC_ENABLE_bm;
}
#endif

void __attribute__((weak)) init_timers()  {

  init_TCA0();
  #if (defined(TCD0) && defined(USE_TIMERD0_PWM) && !defined(MILLIS_USE_TIMERD0))
    init_TCD0();
  #endif

}


#if (defined(TCD0) && defined(USE_TIMERD0_PWM) && !defined(MILLIS_USE_TIMERD0))
void __attribute__((weak)) init_TCD0() {
  TCD0.CMPBCLR  = 509; //510 counts, starts at 0, not 1!
  TCD0.CMPACLR  = 509;
  TCD0.CTRLC    = 0x80; //WOD outputs PWM B, WOC outputs PWM A
  TCD0.CTRLB    = TCD_WGMODE_ONERAMP_gc; //One Slope
  TCD0.CTRLA    = TIMERD0_PRESCALER; //OSC20M prescaled by 32, gives ~1.2 khz PWM at 20MHz.
}
#endif

void __attribute__((weak)) init_TCA0() {
    /*  TYPE A TIMER   */

  /* PORTMUX setting for TCA - don't need to set because using default */
  //PORTMUX.CTRLA = PORTMUX_TCA00_DEFAULT_gc;

  /* Enable Split Mode */
  TCA0.SPLIT.CTRLD = TCA_SPLIT_SPLITM_bm;

  //Only 1 WGM so no need to specifically set up.

  /* Period setting, 8-bit register in SPLIT mode */
  TCA0.SPLIT.LPER    = PWM_TIMER_PERIOD;
  TCA0.SPLIT.HPER    = PWM_TIMER_PERIOD;

  /* Default duty 0%, will re-assign in analogWrite() */
  // 2/1/2021: Why the heck are we bothering to set these AT ALL?! The duty cycles for *non-active* type A timer channels? Which are already initialized to 0 automatically?
  /*
    TCA0.SPLIT.LCMP0 = 0;
    TCA0.SPLIT.HCMP0 = 0;
    TCA0.SPLIT.LCMP1 = 0;
    TCA0.SPLIT.HCMP1 = 0;
    TCA0.SPLIT.LCMP2 = 0;
    TCA0.SPLIT.HCMP2 = 0;
  */

  /* Use prescale appropriate for system clock speed */

  #if (F_CPU > 25000000) //   use 256 divider when clocked over 25 MHz
    TCA0.SPLIT.CTRLA   = (TCA_SPLIT_CLKSEL_DIV256_gc) | (TCA_SPLIT_ENABLE_bm);
  #elif (F_CPU > 5000000) //  use 64 divider for everything in the middle
    TCA0.SPLIT.CTRLA   =  (TCA_SPLIT_CLKSEL_DIV64_gc) | (TCA_SPLIT_ENABLE_bm);
  #elif (F_CPU > 1000000) // and use 16...
    TCA0.SPLIT.CTRLA   =  (TCA_SPLIT_CLKSEL_DIV16_gc) | (TCA_SPLIT_ENABLE_bm);
  #else                   // or even 8 otherwise for really slow system clocks.
    TCA0.SPLIT.CTRLA   =   (TCA_SPLIT_CLKSEL_DIV8_gc) | (TCA_SPLIT_ENABLE_bm);
  #endif
  #ifdef __AVR_ATtinyxy2__
    PORTMUX.CTRLC = 1; //move WO0 output to PA7 so PA3 can be used with WO3
  #endif
}
