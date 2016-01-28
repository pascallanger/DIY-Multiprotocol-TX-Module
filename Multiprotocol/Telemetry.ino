//*************************************
// FrSky Telemetry serial code        *
// By Midelic  on RCG                 *
//*************************************

#if defined TELEMETRY

#define USER_MAX_BYTES 6
#define MAX_PKTX 10
uint8_t frame[18];
uint8_t pass = 0;
uint8_t index;
uint8_t prev_index;
uint8_t pktx[MAX_PKTX];

void frskySendStuffed()
{
	Serial_write(0x7E);
	for (uint8_t i = 0; i < 9; i++)
	{

		if ((frame[i] == 0x7e) || (frame[i] == 0x7d))
		{
			Serial_write(0x7D);	    	  
			frame[i] ^= 0x20;	
		}
		Serial_write(frame[i]);
	}
	Serial_write(0x7E);
}

void compute_RSSIdbm(){ 
	RSSI_dBm = (((uint16_t)(pktt[len-2])*18)>>5);
	if(pktt[len-2] >=128)
		RSSI_dBm -= 82;
	else
		RSSI_dBm += 65;
}

void frsky_link_frame()
{
	frame[0] = 0xFE;
	if ((cur_protocol[0]&0x1F)==MODE_FRSKY)
	{		
		compute_RSSIdbm();				
		frame[1] = pktt[3];
		frame[2] = pktt[4];
		frame[3] = (uint8_t)RSSI_dBm; 
		frame[4] = pktt[5]*2;
		frame[5] = frame[6] = frame[7] = frame[8] = 0;			
	}
	else
		if ((cur_protocol[0]&0x1F)==MODE_HUBSAN)
		{	
			frame[1] = v_lipo*2; //v_lipo; common 0x2A=42/10=4.2V
			frame[2] = frame[1];			
			frame[3] =0X6e;
			frame[4] =2*0x6e;				
			frame[5] = frame[6] = frame[7] = frame[8] = 0;
		}
	frskySendStuffed();
}

#if defined HUB_TELEMETRY
void frsky_user_frame()
{
	uint8_t indexx = 0, c=0, j=8, n=0, i;

	if(pktt[6]>0 && pktt[6]<=MAX_PKTX)
	{//only valid hub frames	  
	  	frame[0] = 0xFD;
		frame[1] = 0;
		frame[2] = pktt[7];
		
        switch(pass)
		{
			case 0:
				indexx=pktt[6];
				for(i=0;i<indexx;i++)
				{
					if(pktt[j]==0x5E)
					{
						if(c++)
						{
							c=0;
							n++;
							j++;
						}
					}
					pktx[i]=pktt[j++];
				}	
				indexx = indexx-n;
				pass=1;
		
			case 1:
				index=indexx;
				prev_index = indexx; 
				if(index<USER_MAX_BYTES)
				{   			
					for(i=0;i<index;i++)
						frame[i+3]=pktx[i];
					pktt[6]=0;
					pass=0;
				}
				else
				{
					index = USER_MAX_BYTES;
					for(i=0;i<index;i++)
						frame[i+3]=pktx[i];
					pass=2;
				}			
				break;
			case 2:		
				index = prev_index - index;
				prev_index=0;
				if(index<MAX_PKTX-USER_MAX_BYTES)	//10-6=4
					for(i=0;i<index;i++)
						frame[i+3]=pktx[USER_MAX_BYTES+i];
				pass=0;
				pktt[6]=0; 
				break;
			default:
				break;
		}
		if(!index)
			return;
		frame[1] = index;
		frskySendStuffed();
	}
	else
		pass=0;
}	   
#endif

void frskyUpdate()
{
	if(telemetry_link)
	{	
		frsky_link_frame();
		telemetry_link=0;
		return;
	}
	#if defined HUB_TELEMETRY
	if(!telemetry_link)
		frsky_user_frame();
	#endif
}

#endif