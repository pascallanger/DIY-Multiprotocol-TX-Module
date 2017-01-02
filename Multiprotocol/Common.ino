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
// Channel value is converted to 8bit values full scale
uint8_t convert_channel_8b(uint8_t num)
{
	return (uint8_t) (map(limit_channel_100(num),servo_min_100,servo_max_100,0,255));	
}

// Channel value is converted to 8bit values to provided values scale
uint8_t convert_channel_8b_scale(uint8_t num,uint8_t min,uint8_t max)
{
	return (uint8_t) (map(limit_channel_100(num),servo_min_100,servo_max_100,min,max));	
}

// Channel value is converted sign + magnitude 8bit values
uint8_t convert_channel_s8b(uint8_t num)
{
	uint8_t ch;
	ch = convert_channel_8b(num);
	return (ch < 128 ? 127-ch : ch);	
}

// Channel value is converted to 10bit values
uint16_t convert_channel_10b(uint8_t num)
{
	return (uint16_t) (map(limit_channel_100(num),servo_min_100,servo_max_100,1,1023));
}

// Channel value is multiplied by 1.5
uint16_t convert_channel_frsky(uint8_t num)
{
	return Servo_data[num] + Servo_data[num]/2;
}

// Channel value is converted for HK310
void convert_channel_HK310(uint8_t num, uint8_t *low, uint8_t *high)
{
	uint16_t temp=0xFFFF-(4*Servo_data[num])/3;
	*low=(uint8_t)(temp&0xFF);
	*high=(uint8_t)(temp>>8);
}

// Channel value is limited to PPM_100
uint16_t limit_channel_100(uint8_t ch)
{
	if(Servo_data[ch]>servo_max_100)
		return servo_max_100;
	else
		if (Servo_data[ch]<servo_min_100)
			return servo_min_100;
	return Servo_data[ch];
}

/******************************/
/**  FrSky D and X routines  **/
/******************************/
void Frsky_init_hop(void)
{
	uint8_t val;
	uint8_t channel = rx_tx_addr[0]&0x07;
	uint8_t channel_spacing = rx_tx_addr[1];
	//Filter bad tables
	if(channel_spacing<0x02) channel_spacing+=0x02;
	if(channel_spacing>0xE9) channel_spacing-=0xE7;
	if(channel_spacing%0x2F==0) channel_spacing++;
		
	hopping_frequency[0]=channel;
	for(uint8_t i=1;i<50;i++)
	{
		channel=(channel+channel_spacing) % 0xEB;
		val=channel;
		if((val==0x00) || (val==0x5A) || (val==0xDC))
			val++;
		hopping_frequency[i]=i>47?0:val;
	}
}
