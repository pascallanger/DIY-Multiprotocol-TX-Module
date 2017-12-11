/*
  main.cpp - Main loop for Arduino sketches
  Copyright (c) 2005-2013 Arduino Team.  All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

//#include <Arduino.h> Platform.h includes Arduino.h
#include <Platform.h> /* to make sure that Arduino.h is included, as well as pins_arduino.h and other things */


#ifdef EIND
// if I have an EIND register, I want it pre-loaded with the correct value
// for info on THIS thing, see http://gcc.gnu.org/onlinedocs/gcc/AVR-Options.html
// in essence, 'init3' section functions run just before 'main()'

// must prototype it to get all of the attributes
static void __attribute__((section(".init3"),naked,used,no_instrument_function)) init3_set_eind (void);

void init3_set_eind (void)
{
  __asm volatile ("ldi r24,pm_hh8(__trampolines_start)\n\t"
                  "out %i0,r24" :: "n" (&EIND) : "r24","memory");
}
#endif // EIND

//Declared weak in Arduino.h to allow user redefinitions.
int atexit(void (*func)()) { return 0; }

// Weak empty variant initialization function.
// May be redefined by variant files.
void initVariant() __attribute__((weak));
void initVariant() { }

int main(void)
{

	init();

	initVariant();

// TEMPORARY - moved [so I can debug it]
//#if defined(USBCON)
//  USBDevice.attach();
//#endif
	
	setup();

	for (;;) {
		loop();
		if (serialEventRun) serialEventRun();
	}
        
	return 0;
}

