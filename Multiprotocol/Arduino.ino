/*
 This project is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 Multiprotocol is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with Multiprotocol.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef STM32_BOARD
/************************************/
/************************************/
/**  Arduino replacement routines  **/
/************************************/
// replacement map()
int16_t map( int16_t x, int16_t in_min, int16_t in_max, int16_t out_min, int16_t out_max)
{
//  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
	long y ;
	x -= in_min ;
	y = out_max - out_min ;
	y *= x ;
	x = y / (in_max - in_min) ;
	return x  + out_min ;
}

// replacement millis() and micros()
// These work polled, no interrupts
// micros() MUST be called at least once every 32 milliseconds
uint16_t MillisPrecount ;
uint16_t lastTimerValue ;
uint32_t TotalMicros ;
uint32_t TotalMillis ;
uint8_t Correction ;

uint32_t micros()
{
   uint16_t elapsed ;
   uint8_t millisToAdd ;
   uint8_t oldSREG = SREG ;
   cli() ;
   uint16_t time = TCNT1 ;   // Read timer 1
   SREG = oldSREG ;

   elapsed = time - lastTimerValue ;
   elapsed += Correction ;
   Correction = elapsed & 0x01 ;
   elapsed >>= 1 ;
   
   uint32_t ltime = TotalMicros ;
   ltime += elapsed ;
   cli() ;
   TotalMicros = ltime ;   // Done this way for RPM to work correctly
   lastTimerValue = time ;
   SREG = oldSREG ;   // Still valid from above
   
   elapsed += MillisPrecount;
   millisToAdd = 0 ;
   
   if ( elapsed  > 15999 )
   {
      millisToAdd = 16 ;
      elapsed -= 16000 ;
   }
   if ( elapsed  > 7999 )
   {
      millisToAdd += 8 ;
      elapsed -= 8000 ;
   }
   if ( elapsed  > 3999 )
   {
      millisToAdd += 4 ;      
      elapsed -= 4000 ;
   }
   if ( elapsed  > 1999 )
   {
      millisToAdd += 2 ;
      elapsed -= 2000 ;
   }
   if ( elapsed  > 999 )
   {
      millisToAdd += 1 ;
      elapsed -= 1000 ;
   }
   TotalMillis += millisToAdd ;
   MillisPrecount = elapsed ;
   return TotalMicros ;
}

uint32_t millis()
{
   micros() ;
   return TotalMillis ;
}

void delayMilliseconds(unsigned long ms)
{
   uint16_t start = (uint16_t)micros();
   uint16_t lms = ms ;

   while (lms > 0) {
      if (((uint16_t)micros() - start) >= 1000) {
         lms--;
         start += 1000;
      }
   }
}

/* Important notes:
	- Max value is 16000µs
	- delay is not accurate due to interrupts happening */
void delayMicroseconds(unsigned int us)
{
   if (--us == 0)
      return;
   us <<= 2;	// * 4
   us -= 2;		// - 2
#ifdef ORANGE_TX
	 __asm__ __volatile__ (
      "1: sbiw %0,1" "\n\t" // 2 cycles
			"nop \n"
			"nop \n"
			"nop \n"
			"nop \n"
      "brne 1b" : "=w" (us) : "0" (us) // 2 cycles
   );
#else   
	 __asm__ __volatile__ (
      "1: sbiw %0,1" "\n\t" // 2 cycles
      "brne 1b" : "=w" (us) : "0" (us) // 2 cycles
   );
#endif
}

#ifndef ORANGE_TX
	void init()
	{
	   // this needs to be called before setup() or some functions won't work there
	   sei();
	}
#endif //ORANGE_TX

#endif //STM32_BOARD