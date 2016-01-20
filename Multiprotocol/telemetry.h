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


static void frskySendStuffed(uint8_t frame[])
{
	Serial_write(0x7E);
	for (uint8_t i = 0; i < 9; i++) {
		if ((frame[i] == 0x7e) || (frame[i] == 0x7d)) {
			Serial_write(0x7D);	    	  
			frame[i] ^= 0x20;			
		}
		Serial_write(frame[i]);
	}
	Serial_write(0x7E);
}

static void frskySendFrame()
{
	uint8_t frame[9];

		frame[0] = 0xfe;
		if ((cur_protocol[0]&0x1F)==MODE_FRSKY)
		{		 
			compute_RSSIdbm();			 					 		  
			frame[1] = pktt[3];
			frame[2] = pktt[4];
			frame[3] = (uint8_t)RSSI_dBm; 
			frame[4] = pktt[5]*2;//txrssi  
			frame[5] = frame[6] = frame[7] = frame[8] = 0;
		}
		else
			if ((cur_protocol[0]&0x1F)==MODE_HUBSAN)
			{	
				frame[1] = v_lipo*2; 
				frame[2] = 0;
				frame[3] = 0x5A;//dummy value
				frame[4] = 2 * 0x5A;//dummy value
				frame[5] = frame[6] = frame[7] = frame[8] = 0;
			}	
		frskySendStuffed(frame);
}

void frskyUpdate()
{
	if(telemetry_link)
	{	
		frskySendFrame();
		telemetry_link=0;
	}
}
