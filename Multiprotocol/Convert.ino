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

/************************/
/**  Convert routines  **/
/************************/
// Reverse a channel and store it
void reverse_channel(uint8_t num)
{
	uint16_t val=2048-Channel_data[num];
	if(val>=2048) val=2047;
	Channel_data[num]=val;
}

// Channel value is converted to ppm 860<->2140 -125%<->+125% and 988<->2012 -100%<->+100%
uint16_t convert_channel_ppm(uint8_t num)
{
	uint16_t val=Channel_data[num];
	return (((val<<2)+val)>>3)+860;				//value range 860<->2140 -125%<->+125%
}

// Channel value 100% is converted to 10bit values 0<->1023
uint16_t convert_channel_10b(uint8_t num)
{
	uint16_t val=Channel_data[num];
	val=((val<<2)+val)>>3;
	if(val<=128) return 0;
	if(val>=1152) return 1023;
	return val-128;
}

// Channel value 100% is converted to 8bit values 0<->255
uint8_t convert_channel_8b(uint8_t num)
{
	uint16_t val=Channel_data[num];
	val=((val<<2)+val)>>5;
	if(val<=32) return 0;
	if(val>=288) return 255;
	return val-32;
}

// Channel value 100% is converted to 8b with deadband
uint8_t convert_channel_8b_limit_deadband(uint8_t num,uint8_t min,uint8_t mid, uint8_t max, uint8_t deadband)
{
	uint16_t val=limit_channel_100(num);		// 204<->1844
	uint16_t db_low=CHANNEL_MID-deadband, db_high=CHANNEL_MID+deadband; // 1024+-deadband
	int32_t calc;
	uint8_t out;
	if(val>=db_low && val<=db_high)
		return mid;
	else if(val<db_low)
	{
		val-=CHANNEL_MIN_100;
		calc=mid-min;
		calc*=val;
		calc/=(db_low-CHANNEL_MIN_100);
		out=calc;
		out+=min;
	}
	else
	{
		val-=db_high;
		calc=max-mid;
		calc*=val;
		calc/=(CHANNEL_MAX_100-db_high+1);
		out=calc;
		out+=mid;
		if(max>min) out++; else out--;
	}
	return out;
}

// Channel value 100% is converted to value scaled
int16_t convert_channel_16b_limit(uint8_t num,int16_t min,int16_t max)
{
	int32_t val=limit_channel_100(num);			// 204<->1844
	val=(val-CHANNEL_MIN_100)*(max-min)/(CHANNEL_MAX_100-CHANNEL_MIN_100)+min;
	return (uint16_t)val;
}

// Channel value -125%<->125% is scaled to 16bit value with no limit
int16_t convert_channel_16b_nolimit(uint8_t num, int16_t min, int16_t max)
{
	int32_t val=Channel_data[num];				// 0<->2047
	val=(val-CHANNEL_MIN_100)*(max-min)/(CHANNEL_MAX_100-CHANNEL_MIN_100)+min;
	return (uint16_t)val;
}

// Channel value is converted sign + magnitude 8bit values
uint8_t convert_channel_s8b(uint8_t num)
{
	uint8_t ch;
	ch = convert_channel_8b(num);
	return (ch < 128 ? 127-ch : ch);	
}

// Channel value is limited to 100%
uint16_t limit_channel_100(uint8_t num)
{
	if(Channel_data[num]>=CHANNEL_MAX_100)
		return CHANNEL_MAX_100;
	if (Channel_data[num]<=CHANNEL_MIN_100)
		return CHANNEL_MIN_100;
	return Channel_data[num];
}

// Channel value is converted for HK310
void convert_channel_HK310(uint8_t num, uint8_t *low, uint8_t *high)
{
	uint16_t temp=0xFFFF-(3440+((Channel_data[num]*5)>>1))/3;
	*low=(uint8_t)(temp&0xFF);
	*high=(uint8_t)(temp>>8);
}

#ifdef FAILSAFE_ENABLE
// Failsafe value is converted for HK310
void convert_failsafe_HK310(uint8_t num, uint8_t *low, uint8_t *high)
{
	uint16_t temp=0xFFFF-(3440+((Failsafe_data[num]*5)>>1))/3;
	*low=(uint8_t)(temp&0xFF);
	*high=(uint8_t)(temp>>8);
}
#endif

// Channel value for FrSky (PPM is multiplied by 1.5)
uint16_t convert_channel_frsky(uint8_t num)
{
	uint16_t val=Channel_data[num];
	return ((val*15)>>4)+1290;
}

// 0-2047, 0 = 817, 1024 = 1500, 2047 = 2182
//64=860,1024=1500,1984=2140//Taranis 125%
static uint16_t  __attribute__((unused)) FrSkyX_scaleForPXX( uint8_t i )
{	//mapped 860,2140(125%) range to 64,1984(PXX values);
	uint16_t chan_val=convert_channel_frsky(i)-1226;
	if(i>7) chan_val|=2048;   // upper channels offset
	return chan_val;
}

#ifdef FAILSAFE_ENABLE
static uint16_t  __attribute__((unused)) FrSkyX_scaleForPXX_FS( uint8_t i )
{	//mapped 1,2046(125%) range to 64,1984(PXX values);
	uint16_t chan_val=((Failsafe_data[i]*15)>>4)+64;
	if(Failsafe_data[i]==FAILSAFE_CHANNEL_NOPULSES)
		chan_val=FAILSAFE_CHANNEL_NOPULSES;
	else if(Failsafe_data[i]==FAILSAFE_CHANNEL_HOLD)
		chan_val=FAILSAFE_CHANNEL_HOLD;
	if(i>7) chan_val|=2048;   // upper channels offset
	return chan_val;
}
#endif
