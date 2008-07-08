// ### BOILERPLATE ###
// 64-bit Counter - Firmware 
// Copyright (C) 2008 Peter Todd <pete@petertodd.org>
// 
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
// ### BOILERPLATE ###

#include <common.h>
#include <signal.h>
#include <counter.h>

code char at __CONFIG1H config1h = 0xFF & _OSC_EXT_Port_on_RA6_1H & _FCMEN_ON_1H; 
code char at __CONFIG2L config2l = 0xFF & _PUT_ON_2L & _BODEN_ON_2L & _BODENV_2_0V_2L;
code char at __CONFIG2H config2h = 0xFF & _WDT_DISABLED_CONTROLLED_2H;
code char at __CONFIG3H config3h = 0xFF & _MCLRE_MCLR_enabled_RA5_input_dis_3H;
code char at __CONFIG4L config4l = 0xFF & _LVP_OFF_4L & _BACKBUG_OFF_4L & _STVR_OFF_4L;
code char at __CONFIG5L config5l = 0xFF & _CP_0_OFF_5L & _CP_1_OFF_5L;
code char at __CONFIG5H config5h = 0xFF & _CPD_OFF_5H & _CPB_OFF_5H;
code char at __CONFIG6L config6l = 0xFF & _WRT_0_OFF_6L & _WRT_1_OFF_6L;
code char at __CONFIG6H config6h = 0xFF & _WRTD_OFF_6H & _WRTB_OFF_6H & _WRTC_OFF_6H;
code char at __CONFIG7L config7l = 0xFF & _EBTR_0_OFF_7L & _EBTR_1_OFF_7L;
code char at __CONFIG7H config7h = 0xFF & _EBTRB_OFF_7H;

// If stack isn't set, delay() calls do really weird things.
//
// Base set at 0x80 as aparently there is a "access bank" from 0x00 to 0x80
//
// Unsure as to what the meaning of all that is exactly...
#pragma stack 0x80 0x40

// Interupts, AKA signals.

#define SIG_OSCFIF SIG_PIR(2),7 

// Returns a random number after permutating the LFSR seed.
uint8_t rand_seed = 1;
uint8_t rand(){
  __asm
    BANKSEL _rand_seed
    BCF     STATUS,0
    RRCF    _rand_seed,W
    BTFSC   STATUS,0
    XORLW   0xB4
    MOVWF   _rand_seed
  _endasm;

  return  rand_seed;
}

DEF_INTHIGH(high_int)
	
DEF_HANDLER(SIG_TMR0,_tmr0_handler)
DEF_HANDLER(SIG_OSCFIF,_oscfif_handler)

END_DEF

SIGHANDLER(_tmr0_handler)

{
  inc_counter();

  // Check the brightness knob and update the pwm
  ADCON0bits.GO = 1;
  while (ADCON0bits.GO);
  CCPR1L = ADRESH;

  // Add to the random "pool"
  rand_seed ^= ADRESH;

  // Re-enable ourselves.
  INTCONbits.T0IF = 0;
}

SIGHANDLER(_oscfif_handler)

{
  // Save the lower three bytes of the counter to the eeprom, in reverse order
  // from most user visible to least.
  save_counter(2,3);
  save_counter(1,2);
  save_counter(0,1);

  // Wait, this should ensure that the power has failed fully, or has come
  // back, without excessive writes to the eeprom.
  delay10ktcy(1);

  // Reset, nothing we can do.
  Reset();
}

void main(){
  // Initialization

  // Variables

  // Ports
  TRISA = 0x00;
  TRISB = 0x00;

  // Modules
  init_counter();
  //init_eeprom();
  //init_time();
  //init_user();

  // Enable the clock oscillator fail interrupt
  PIE2bits.OSCFIE = 1;
  INTCONbits.PEIE = 1;

  // Setup pwm and adc for the led brightness control
  TRISAbits.TRISA0 = 1;
  ADCON0 = b(00000001);
  ADCON1 = b(01111110);
  ADCON2 = b(00000110);
  
  CCP1CON = b(00001100);
  PR2 = 0xFF;
  T2CON = b(00000100);
  CCPR1L = 0x10;

  // Last digit in the hardware counter:
  // 64mhz / 2^16 = 976hz
  //
  // Our oscillator is 16mhz, therefor:
  //
  // 16mhz/4/256/8/4 = 488hz
  //
  // Mind... Shouldn't the last divisor be 2, not 4?
  INTCONbits.T0IF = 0;
  INTCONbits.T0IE = 1;
  INTCONbits.GIE = 1;
  T0CON = b(11000010); 

  while (1){
    // Dither the PWM timebase to avoid beat frequencies.
    TMR2 -= rand() & b(00000001);
  }
}

// Local Variables:
// mode: C
// fill-column: 76
// c-file-style: "gnu"
// indent-tabs-mode: nil
// End:
// vim: et:sw=2:sts=2:ts=2:cino=>2s,{s,\:s,+s,t0,g0,^-2,e-2,n-2,p2s,(0,=s:
