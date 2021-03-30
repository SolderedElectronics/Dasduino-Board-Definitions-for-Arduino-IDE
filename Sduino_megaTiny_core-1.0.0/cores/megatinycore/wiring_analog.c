/*
  wiring_analog.c - analog input and output
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

  Modified 28 September 2010 by Mark Sproul
*/

#include "wiring_private.h"
#include "pins_arduino.h"
#include "Arduino.h"

void analogReference(uint8_t mode) {
  switch (mode) {
      #if defined(EXTERNAL)
    case EXTERNAL:
      #elif defined(EXTERNAL_EXPERIMENTAL)
    case EXTERNAL_EXPERIMENTAL:
      #endif
    case VDD:
      ADC0.CTRLC = (ADC0.CTRLC & ~(ADC_REFSEL_gm)) | mode | ADC_SAMPCAP_bm; //per datasheet, recommended SAMPCAP=1 at ref > 1v - we don't *KNOW* the external reference will be >1v, but it's probably more likely...
      // VREF.CTRLA does not need to be reconfigured, as the voltage references only supply their specified voltage when requested to do so by the ADC.
      break;
    case INTERNAL0V55:
      VREF.CTRLA =  VREF.CTRLA & ~(VREF_ADC0REFSEL_gm); //These bits are all 0 for 0.55v reference, so no need to do the mode << VREF_ADC0REFSEL_gp here;
      ADC0.CTRLC = (ADC0.CTRLC & ~(ADC_REFSEL_gm | ADC_SAMPCAP_bm)) | INTERNAL; //per datasheet, recommended SAMPCAP=0 at ref < 1v
      break;
    case INTERNAL1V1:
    case INTERNAL2V5:
    case INTERNAL4V34:
    case INTERNAL1V5:
      VREF.CTRLA = (VREF.CTRLA & ~(VREF_ADC0REFSEL_gm)) | (mode << VREF_ADC0REFSEL_gp);
      ADC0.CTRLC = (ADC0.CTRLC & ~(ADC_REFSEL_gm)) | INTERNAL | ADC_SAMPCAP_bm; //per datasheet, recommended SAMPCAP=1 at ref > 1v
      break;
    default:
      ADC0.CTRLC = (ADC0.CTRLC & ~(ADC_REFSEL_gm)) | VDD | ADC_SAMPCAP_bm; //per datasheet, recommended SAMPCAP=1 at ref > 1v - we don't *KNOW* the external reference will be >1v, but it's probably more likely...
  }
}
#ifdef DAC0
void DACReference(uint8_t mode) {
  if (mode < 5) {
    VREF.CTRLA = mode | (VREF.CTRLA & ~7);
  }
}
#endif
int analogRead(uint8_t pin) {
  #ifdef ADC_DAC0
  if (pin != ADC_DAC0 && pin != ADC_INTREF && pin != ADC_TEMPERATURE)
  #else
  if (pin != ADC_INTREF && pin != ADC_TEMPERATURE)
  #endif
  {
    pin = digitalPinToAnalogInput(pin);
    if (pin == NOT_A_PIN) {
      return -1;
    }
  }


  #if defined(ADC0)
  /* Reference should be already set up */
  /* Select channel */
  ADC0.MUXPOS = (pin << ADC_MUXPOS_gp);

  /* Start conversion */
  ADC0.COMMAND = ADC_STCONV_bm;

  /* Wait for result ready */
  while (!(ADC0.INTFLAGS & ADC_RESRDY_bm));

  /* Combine two bytes */
  return ADC0.RES;

  #else  /* No ADC, return 0 */
  return 0;
  #endif

}

//analogReadResolution() has two legal values you can pass it, 8 or 10 on these parts. According to the datasheet, you can clock the ADC faster if you set it to 8.
//like the pinswap functions, if the user passes bogus values, we set it to the default and return false.

bool analogReadResolution(uint8_t res) {
  if (res == 8) {
    ADC0.CTRLA |= ADC_RESSEL_bm;
    return true;
  }
  //if argument wasn't 8, we'll be putting it to default value either way
  ADC0.CTRLA &= ~ADC_RESSEL_bm;
  return (res == 10); //but only return true if the value passed was the valid option, 10.
}


// Right now, PWM output only works on the pins with
// hardware support.  These are defined in the appropriate
// pins_*.c file.  For the rest of the pins, we default
// to digital output.
void analogWrite(uint8_t pin, int val) {
  uint8_t bit_pos  = digitalPinToBitPosition(pin);
  if (bit_pos == NOT_A_PIN) {
    return;
  }
  // We need to make sure the PWM output is enabled for those pins
  // that support it.  Also, make sure the pin is in output mode
  // for consistently with Wiring, which doesn't require a pinMode
  // call for the analog output pins.
  pinMode(pin, OUTPUT);

  /* Get timer */
  uint8_t digital_pin_timer =  digitalPinToTimer(pin);
  uint8_t *timer_cmp_out;
  /* Find out Port and Pin to correctly handle port mux, and timer. */
  switch (digital_pin_timer) { //use only low nybble which defines which timer it is

    case TIMERA0:
      if (val <= 0) { /* if zero or negative drive digital low */
        digitalWrite(pin, LOW);
      } else if (val >= 255) { /* if max or greater drive digital high */
        digitalWrite(pin, HIGH);
      } else {
        /* Calculate correct compare buffer register */
        #ifdef __AVR_ATtinyxy2__
        if (bit_pos == 7) {
          bit_pos = 0;  //on the xy2, WO0 is on PA7
        }
        #endif
        if (bit_pos > 2) {
          bit_pos -= 3;
          timer_cmp_out = ((uint8_t *)(&TCA0.SPLIT.HCMP0)) + (bit_pos << 1);
          (*timer_cmp_out) = (val);
          TCA0.SPLIT.CTRLB |= (1 << (TCA_SPLIT_HCMP0EN_bp + bit_pos));
        } else {
          timer_cmp_out = ((uint8_t *)(&TCA0.SPLIT.LCMP0)) + (bit_pos << 1);
          (*timer_cmp_out) = (val);
          TCA0.SPLIT.CTRLB |= (1 << (TCA_SPLIT_LCMP0EN_bp + bit_pos));
        }
      }
      break;
      // End of TCA case

  #if defined(DAC0)
    case DACOUT:
    {
      DAC0.DATA = val;
      DAC0.CTRLA = 0x41; //OUTEN=1, ENABLE=1
      break;
    }
  #endif
  #if (defined(TCD0) && defined(USE_TIMERD0_PWM))
    case TIMERD0:
      {
      #ifndef NO_GLITCH_TIMERD0
        if (val < 1) { /* if zero or negative drive digital low */
          digitalWrite(pin, LOW);
        } else if (val > 254) { /* if max or greater drive digital high */
          digitalWrite(pin, HIGH);
        } else {
      #endif
        // Calculation of values to write to CMPxSET
        // val is 1~254, so 255-val is 1~254. so we double it and subtract 1, now we have odd number between 1 and 507, corresponding to an even number of counts.
        // Timer counts to 509, 510 counts including 0... so this gives us our promised 1/255~254/255 duty cycles, not 2/511th~509/511th...
        // Simple stuff, how do people get this wrong? Sheesh...

        // Now, if NO_GLITCH_TIMERD0 is defined, val can be anything...
        #if defined(NO_GLITCH_TIMERD0)
          uint8_t set_inven=0;
          if (val < 1) {
            val=0; //this will "just work", we'll set it to the maximum, it will never match, and will stay LOW
          } else if (val > 254) {
            val=0; //here we *also* set it to 0 so it would stay LOW
            set_inven=1; //but we invert the pin output with INVEN!
          }
        #endif

        uint8_t oldSREG=SREG;
        cli(); //interrupts off... wouldn't due to have this mess interrupted and messed with...
        while (!(TCD0.STATUS & (TCD_ENRDY_bm | TCD_CMDRDY_bm ))); //if previous sync/enable in progress, wait for it to finish.
        // with interrupts off since an interrupt could trip these...
        //set new values
        if (bit_pos) {  //PIN_PC1
          TCD0.CMPBSET = ((255 - val) << 1)-1;
        } else {        //PIN_PC0
          TCD0.CMPASET = ((255 - val) << 1)-1;
        }

        if (!(TCD0.FAULTCTRL & (1 << (6 + bit_pos)))) {
          //if it's not active, we need to activate it...
          //if not active, we need to activate it, which produces a glitch in the PWM
          uint8_t TCD0_prescaler=TCD0.CTRLA&(~TCD_ENABLE_bm);
          TCD0.CTRLA = TCD0_prescaler; //stop the timer
          while (!(TCD0.STATUS & TCD_ENRDY_bm)); // wait until it's actually stopped
          _PROTECTED_WRITE(TCD0.FAULTCTRL, TCD0.FAULTCTRL | (1 << (6 + bit_pos)));
          TCD0.CTRLA = (TCD0_prescaler | TCD_ENABLE_bm); //re-enable it
        } else {
          if (bit_pos) {  //PIN_PC1
            TCD0.CMPBSET = ((255 - val) << 1)-1;
          } else {        //PIN_PC0
            TCD0.CMPASET = ((255 - val) << 1)-1;
          }
          TCD0.CTRLE = 0x02; //Synchronize
        }

        #if defined(NO_GLITCH_TIMERD0)
          // We only support control of the TCD0 PWM functionality on PIN_PC0 and PIN_PC1 (on 20 and 24 pin parts )
          // so if we're here, we're acting on either PC0 or PC1.
          if (set_inven==0){
            // we are not setting invert to make the pin HIGH when not set; either was 0 (just set CMPxSET > CMPBCLR)
            // or somewhere in between.
            if (bit_pos==0){
              PORTC.PIN0CTRL&=~(PORT_INVEN_bm);
            } else {
              PORTC.PIN1CTRL&=~(PORT_INVEN_bm);
            }
          } else {
            // we *are* turning off PWM while forcing pin high - analogwrite(pin,255) was called on TCD0 PWM pin...
            if (bit_pos==0){
              PORTC.PIN0CTRL|=PORT_INVEN_bm;
            } else {
              PORTC.PIN1CTRL|=PORT_INVEN_bm;
            }
          }
        #endif
        SREG=oldSREG;
      }
      break;
    #endif
    //end of TCD0 code

    /* If non timer pin, or unknown timer definition.  */
    /* do a digital write  */
    case NOT_ON_TIMER:
    default:
      if (val < 128) {
        digitalWrite(pin, LOW);
      } else {
        digitalWrite(pin, HIGH);
      }
      break;
  } //end of switch/case
} // end of analogWrite
