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
#include <counter.h>

uint8_t count[6];

void init_counter(){
  // Load from eeprom
  for (EEADR = 0; EEADR < sizeof(count); EEADR++){
    EECON1bits.EEPGD = 0;
    EECON1bits.CFGS = 0;
    EECON1bits.RD = 1;
    count[EEADR] = EEDATA;
  }
}

void save_counter(uint8_t start,uint8_t end){
  uint8_t i;
  for (i = start; i < end; i++){
    EEADR = i; 

    EEDATA = count[EEADR];

    EECON1bits.EEPGD = 0;
    EECON1bits.CFGS = 0;
    EECON1bits.WREN = 1;

    // critical bit here
    _asm 
      movlw 0x55
      movwf _EECON2
      movlw 0xAA
      movwf _EECON2
      bsf _EECON1,1 
    _endasm;

    while (EECON1bits.WR); // wait for write to finish
  }
}

#define select_byte(i) LATA = ((LATA & b(11110001)) | (i<<1))

// Clear latches by selecting a non-existant byte, see schematic.
#define clear_latches() select_byte(6)

void display_counter(){
  uint8_t i;

  for (i = 0; i < sizeof(count); i++){
    // Clear all all byte selectors, disabling all driver latches.
    clear_latches();
    delay10tcy(1);

    // Load can proceed without causing glitches.
    LATB = count[i];
    if (count[i] & b(00001000)){
      LATAbits.LATA6 = 1;
    } else {
      LATAbits.LATA6 = 0;
    }

    // Latch the byte
    delay10tcy(1);
    select_byte(i);
    delay10tcy(1);
  }
}

void inc_counter(){
  uint8_t i;

  for (i = 0; i < sizeof(count); i++){
    count[i]++;

    if (count[i]) break;
  }

  // Save the whole eeprom every 4.764 hours.  
  if (i > 2){
    save_counter(0,sizeof(count));
  }

  display_counter();
}

// Local Variables:
// mode: C
// fill-column: 76
// c-file-style: "gnu"
// indent-tabs-mode: nil
// End:
// vim: et:sw=2:sts=2:ts=2:cino=>2s,{s,\:s,+s,t0,g0,^-2,e-2,n-2,p2s,(0,=s:
