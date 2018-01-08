/*
 Protocol by Dennis Cabell, 2017
 KE8FZX
  
 To use this software, you must adhere to the license terms described below, and assume all responsibility for the use
 of the software.  The user is responsible for all consequences or damage that may result from using this software.
 The user is responsible for ensuring that the hardware used to run this software complies with local regulations and that 
 any radio signal generated or received from use of this software is legal for that user to generate.  The author(s) of this software 
 assume no liability whatsoever.  The author(s) of this software is not responsible for legal or civil consequences of 
 using this software, including, but not limited to, any damages cause by lost control of a vehicle using this software.  
 If this software is copied or modified, this disclaimer must accompany all copies.
 
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
 
// The Receiver for this protocol is available at: https://github.com/soligen2010/RC_RX_CABELL_V3_FHSS

#if defined(CABELL_NRF24L01_INO)

#include "iface_nrf24l01.h"

#define CABELL_BIND_COUNT		2000		// At least 2000 so that if TX toggles the serial bind flag then bind mode is never exited
#define CABELL_PACKET_PERIOD	3000		// Do not set too low or else next packet may not be finished transmitting before the channel is changed next time around

#define CABELL_NUM_CHANNELS		16			// The maximum number of RC channels that can be sent in one packet
#define CABELL_MIN_CHANNELS		4			// The minimum number of channels that must be included in a packet, the number of channels cannot be reduced any further than this
#define CABELL_PAYLOAD_BYTES	24			// 12 bits per value * 16 channels

#define CABELL_RADIO_CHANNELS			9	// This is 1/5 of the total number of radio channels used for FHSS
#define CABELL_RADIO_MIN_CHANNEL_NUM	3	// Channel 0 is right on the boarder of allowed frequency range, so move up to avoid bleeding over
#define CABELL_TELEMETRY_PACKET_LENGTH	4

#define CABELL_BIND_RADIO_ADDR	0xA4B7C123F7LL

#define CABELL_OPTION_MASK_CHANNEL_REDUCTION		0x0F
#define CABELL_OPTION_MASK_RECIEVER_OUTPUT_MODE		0x30
#define CABELL_OPTION_SHIFT_RECIEVER_OUTPUT_MODE	4
#define CABELL_OPTION_MASK_MAX_POWER_OVERRIDE		0x40

typedef struct
{
   enum RxMode_t : uint8_t
   {   // Note bit 8 is used to indicate if the packet is the first of 2 on the channel.  Mask out this bit before using the enum
         normal                 = 0,
         bind                   = 1,
         setFailSafe            = 2,
         normalWithTelemetry    = 3,
         telemetryResponse      = 4,
         unBind                 = 127
   } RxMode;
   uint8_t reserved = 0;
   uint8_t option;
                          /*   mask 0x0F    : Channel reduction.  The number of channels to not send (subtracted from the 16 max channels) at least 4 are always sent
                           *   mask 0x30>>4 : Receiver output mode
                           *                  0 (00) = Single PPM on individual pins for each channel 
                           *                  1 (01) = SUM PPM on channel 1 pin
                           *                  2 (10) = Future use.  Reserved for SBUS output
                           *                  3 (11) = Unused
                           *   mask 0x40>>6   Contains max power override flag for Multi-protocol TX module. Also sent to RX
                           *   mask 0x80>>7   Unused 
                           */  
   uint8_t modelNum;
   uint8_t checkSum_LSB; 
   uint8_t checkSum_MSB; 
   uint8_t payloadValue [CABELL_PAYLOAD_BYTES] = {0}; //12 bits per channel value, unsigned
} CABELL_RxTxPacket_t;   

//-----------------------------------------------------------------------------------------
static uint8_t __attribute__((unused)) CABELL_getNextChannel (uint8_t seqArray[], uint8_t seqArraySize, uint8_t prevChannel)
{
	/* Possible channels are in 5 bands, each band comprised of seqArraySize channels
	* seqArray contains seqArraySize elements in the relative order in which we should progress through the band 
	* 
	* Each time the channel is changes, bands change in a way so that the next channel will be in a
	* different non-adjacent band. Both the band changes and the index in seqArray is incremented.
	*/
	prevChannel -= CABELL_RADIO_MIN_CHANNEL_NUM;				// Subtract CABELL_RADIO_MIN_CHANNEL_NUM because it was added to the return value
	if(prevChannel>(seqArraySize * 5))
		prevChannel=seqArraySize * 5;							// Constrain the values just in case something bogus was sent in.

	uint8_t currBand = prevChannel / seqArraySize;             
	uint8_t nextBand = (currBand + 3) % 5;

	uint8_t prevChannalSeqArrayValue = prevChannel % seqArraySize;
	uint8_t prevChannalSeqArrayPosition = 0;
	for (int x = 0; x < seqArraySize; x++)
	{	// Find the position of the previous channel in the array
		if (seqArray[x] == prevChannalSeqArrayValue)
			prevChannalSeqArrayPosition = x;
	}
	uint8_t nextChannalSeqArrayPosition = prevChannalSeqArrayPosition + 1;
	if (nextChannalSeqArrayPosition >= seqArraySize)
		nextChannalSeqArrayPosition = 0;

	return (seqArraySize * nextBand) + seqArray[nextChannalSeqArrayPosition] + CABELL_RADIO_MIN_CHANNEL_NUM;	// Add CABELL_RADIO_MIN_CHANNEL_NUM so we dont use channel 0 as it may bleed below 2.400 GHz
}

//-----------------------------------------------------------------------------------------
#if defined CABELL_HUB_TELEMETRY 
static void __attribute__((unused)) CABELL_get_telemetry()
{
	// calculate TX rssi based on past 250 expected telemetry packets.  Cannot use full second count because telemetry_counter is not large enough
	state++;
	if (state > 250)
	{
		TX_RSSI = telemetry_counter;
		telemetry_counter = 0;
		state = 0;
		telemetry_lost=0;
	}

	// Process incoming telemetry packet of it was received
	if (NRF24L01_ReadReg(NRF24L01_07_STATUS) & _BV(NRF24L01_07_RX_DR)) 
	{ // data received from model
		NRF24L01_ReadPayload(packet, CABELL_TELEMETRY_PACKET_LENGTH);
		if ((packet[0] & 0x7F) == CABELL_RxTxPacket_t::telemetryResponse)	// ignore high order bit in compare because it toggles with each packet
		{
			RX_RSSI = packet[1];	// Packet rate 0 to 255 where 255 is 100% packet rate
			v_lipo1 = packet[2];	// Directly from analog input of receiver, but reduced to 8-bit depth (0 to 255).  Scaling depends on the input to the analog pin of the receiver.
			v_lipo2 = packet[3];	// Directly from analog input of receiver, but reduced to 8-bit depth (0 to 255).  Scaling depends on the input to the analog pin of the receiver.
			telemetry_counter++;      
			if(telemetry_lost==0)
				telemetry_link=1;
		}
	}
	else
	{
		// If no telemetry packet was received then delay by the typical telemetry packet processing time
		// This is done to try to keep the sendPacket process timing more consistent. Since the SPI payload read takes some time
		delayMicroseconds(50);
	}
	NRF24L01_SetTxRxMode(TX_EN);  
	NRF24L01_FlushRx(); 
}
#endif

//-----------------------------------------------------------------------------------------
static void __attribute__((unused)) CABELL_send_packet(uint8_t bindMode)
{  
	#if defined CABELL_HUB_TELEMETRY  
		if (!bindMode && (sub_protocol == CABELL_V3_TELEMETRY))		// check for incoming packet and switch radio back to TX mode if we were listening for telemetry      
			CABELL_get_telemetry();
	#endif

	CABELL_RxTxPacket_t TxPacket;

	uint8_t channelReduction = constrain((option & CABELL_OPTION_MASK_CHANNEL_REDUCTION),0,CABELL_NUM_CHANNELS-CABELL_MIN_CHANNELS);	// Max 12 - cannot reduce below 4 channels
	if (bindMode)
		channelReduction = 0;	// Send full packet to bind as higher channels will contain bind info

	uint8_t packetSize = sizeof(TxPacket) - ((((channelReduction - (channelReduction%2))/ 2)) * 3);		// reduce 3 bytes per 2 channels, but not last channel if it is odd
	uint8_t maxPayloadValueIndex = sizeof(TxPacket.payloadValue) - (sizeof(TxPacket) - packetSize);

	if ((sub_protocol == CABELL_UNBIND) && !bindMode)
	{
		TxPacket.RxMode = CABELL_RxTxPacket_t::unBind;
		TxPacket.option = option;
	}
	else
	{
		if (sub_protocol == CABELL_SET_FAIL_SAFE && !bindMode)
			TxPacket.RxMode = CABELL_RxTxPacket_t::setFailSafe;
		else
		{
			if (bindMode)
				TxPacket.RxMode = CABELL_RxTxPacket_t::bind;        
			else
			{
				switch (sub_protocol)
				{
					case CABELL_V3_TELEMETRY:
						TxPacket.RxMode = CABELL_RxTxPacket_t::normalWithTelemetry;
						break;
					default:
						TxPacket.RxMode = CABELL_RxTxPacket_t::normal;  
						break;
				}      
			}
		}
		TxPacket.option = (bindMode) ? (option & (~CABELL_OPTION_MASK_CHANNEL_REDUCTION)) : option;		//remove channel reduction if in bind mode
	}
	TxPacket.reserved = 0;
	TxPacket.modelNum = RX_num;
	uint16_t checkSum = TxPacket.modelNum + TxPacket.option + TxPacket.RxMode  + TxPacket.reserved;		// Start Calculate checksum

	int adjusted_x;
	int payloadIndex = 0;
	uint16_t holdValue;

	for (int x = 0;(x < CABELL_NUM_CHANNELS - channelReduction); x++)
	{
		switch (x)
		{
			case 0	: adjusted_x = ELEVATOR;	break;
			case 1	: adjusted_x = AILERON;		break;
			case 2	: adjusted_x = RUDDER;		break;
			case 3	: adjusted_x = THROTTLE;	break;
			default	: adjusted_x = x;			break;
		}
		holdValue = convert_channel_16b_limit(adjusted_x,1000,2000);				// valid channel values are 1000 to 2000
		if (bindMode)
		{
			switch (adjusted_x)
			{
				case THROTTLE	: holdValue = 1000;					break;      // always set throttle to off when binding for safety
				//tx address sent for bind
				case 11			: holdValue = 1000 + rx_tx_addr[0];	break;
				case 12			: holdValue = 1000 + rx_tx_addr[1];	break;
				case 13			: holdValue = 1000 + rx_tx_addr[2];	break;
				case 14			: holdValue = 1000 + rx_tx_addr[3];	break;
				case 15			: holdValue = 1000 + rx_tx_addr[4];	break;
			}
		}

		// use 12 bits per value
		if (x % 2)
		{ //output channel number is ODD
			holdValue = holdValue<<4;
			payloadIndex--;     
		}
		else
			holdValue &= 0x0FFF;
		TxPacket.payloadValue[payloadIndex] |=  (uint8_t)(holdValue & 0x00FF);
		payloadIndex++;
		TxPacket.payloadValue[payloadIndex] |=  (uint8_t)((holdValue>>8) & 0x00FF);   
		payloadIndex++;
	}

	for(int x = 0; x < maxPayloadValueIndex ; x++)
		checkSum += TxPacket.payloadValue[x];  // Finish Calculate checksum 

	TxPacket.checkSum_MSB = checkSum >> 8;
	TxPacket.checkSum_LSB = checkSum & 0x00FF;

	// Set channel for next transmission
	rf_ch_num = CABELL_getNextChannel (hopping_frequency,CABELL_RADIO_CHANNELS, rf_ch_num);
	NRF24L01_WriteReg(NRF24L01_05_RF_CH,rf_ch_num); 

	//NRF24L01_FlushTx();   //just in case things got hung up
	//NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);

	uint8_t* p = reinterpret_cast<uint8_t*>(&TxPacket.RxMode);
	*p &= 0x7F;                  // Make sure 8th bit is clear
	*p |= (packet_count++)<<7;   // This causes the 8th bit of the first byte to toggle with each xmit so consecutive payloads are not identical.
	// This is a work around for a reported bug in clone NRF24L01 chips that mis-took this case for a re-transmit of the same packet.

	CABELL_SetPower();
	NRF24L01_WritePayload((uint8_t*)&TxPacket, packetSize);

	#if defined CABELL_HUB_TELEMETRY 
		if (!bindMode && (sub_protocol == CABELL_V3_TELEMETRY))
		{ // switch radio to rx as soon as packet is sent  
			// calculate transmit time based on packet size and data rate of 1MB per sec
			// This is done because polling the status register during xmit caused issues.
			// bits = packst_size * 8  +  73 bits overhead
			// at 250 Kbs per sec, one bit is 4 uS
			// then add 140 uS which is 130 uS to begin the xmit and 10 uS fudge factor
			delayMicroseconds(((((unsigned long)packetSize * 8ul)  +  73ul) * 4ul) + 140ul)   ;
			packet_period = CABELL_PACKET_PERIOD + (constrain(((int16_t)(CABELL_NUM_CHANNELS - channelReduction) - (int16_t)6 ),(int16_t)0 ,(int16_t)10 ) * (int16_t)100);  // increase packet period by 100 us for each channel over 6
			NRF24L01_WriteReg(NRF24L01_00_CONFIG, 0x0F);  // RX mode with 16 bit CRC
		}
		else 
	#endif
			packet_period = CABELL_PACKET_PERIOD;   // Standard packet period when not in telemetry mode.
}

//-----------------------------------------------------------------------------------------
static void __attribute__((unused)) CABELL_getChannelSequence (uint8_t outArray[], uint8_t numChannels, uint64_t permutation)
{
	/* This procedure initializes an array with the sequence progression of channels.
	* This is not the actual channels itself, but the sequence base to be used within bands of 
	* channels.
	* 
	* There are numChannels! permutations for arranging the channels
	* one of these permutations will be calculated based on the permutation input
	* permutation should be between 1 and numChannels! but the routine will constrain it
	* if these bounds are exceeded.  Typically the radio's unique TX ID should be used.
	* 
	* The maximum numChannels is 20.  Anything larger than this will cause the uint64_t
	* variables to overflow, yielding unknown results (possibly infinite loop?).  Therefor
	* this routine constrains the value.
	*/  
	uint8_t i;   //iterator counts numChannels
	uint64_t indexOfNextSequenceValue;
	uint64_t numChannelsFactorial=1;
	uint8_t  sequenceValue;

	numChannels = constrain(numChannels,1,20);

	for (i = 1; i <= numChannels;i++)
	{
		numChannelsFactorial *= i;      //  Calculate n!
		outArray[i-1] = i-1;            //  Initialize array with the sequence
	}

	permutation = (permutation % numChannelsFactorial) + 1;    // permutation must be between 1 and n! or this algorithm will infinite loop

	//Rearrange the array elements based on the permutation selected
	for (i=0, permutation--; i<numChannels; i++ )
	{
		numChannelsFactorial /= ((uint64_t)numChannels)-i;
		indexOfNextSequenceValue = i+(permutation/numChannelsFactorial);
		permutation %= numChannelsFactorial;

		//Copy the value in the selected array position
		sequenceValue = outArray[indexOfNextSequenceValue];

		//Shift the unused elements in the array to make room to move in the one just selected
		for( ; indexOfNextSequenceValue > i; indexOfNextSequenceValue--)
			outArray[indexOfNextSequenceValue] = outArray[indexOfNextSequenceValue-1];

		// Copy the selected value into it's new array slot
		outArray[i] = sequenceValue;
	}
}

//-----------------------------------------------------------------------------------------
static void __attribute__((unused)) CABELL_setAddress()
{
	uint64_t CABELL_addr;

	//  Serial.print("NORM ID: ");Serial.print((uint32_t)(CABELL_normal_addr>>32)); Serial.print("    ");Serial.println((uint32_t)((CABELL_normal_addr<<32)>>32));

	if (IS_BIND_DONE)
	{
		CABELL_addr = (((uint64_t)rx_tx_addr[0]) << 32) + 
			(((uint64_t)rx_tx_addr[1]) << 24) + 
			(((uint64_t)rx_tx_addr[2]) << 16) + 
			(((uint64_t)rx_tx_addr[3]) << 8) + 
			(((uint64_t)rx_tx_addr[4]));					// Address to use after binding
	}
	else
		CABELL_addr = CABELL_BIND_RADIO_ADDR;				// Static addr for binding

	CABELL_getChannelSequence(hopping_frequency,CABELL_RADIO_CHANNELS,CABELL_addr);		// Get the sequence for hopping through channels
	rf_ch_num = CABELL_RADIO_MIN_CHANNEL_NUM;				// Initialize the channel sequence

	packet_count=0;  

	uint64_t CABELL_Telemetry_addr = ~CABELL_addr;			// Invert bits for reading so that telemetry packets have a different address.

	NRF24L01_WriteRegisterMulti(NRF24L01_0A_RX_ADDR_P0, reinterpret_cast<uint8_t*>(&CABELL_Telemetry_addr), 5);
	NRF24L01_WriteRegisterMulti(NRF24L01_0B_RX_ADDR_P1, reinterpret_cast<uint8_t*>(&CABELL_Telemetry_addr), 5);
	NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR,    reinterpret_cast<uint8_t*>(&CABELL_addr), 5);
}

//-----------------------------------------------------------------------------------------
static void __attribute__((unused)) CABELL_init()
{
	NRF24L01_Initialize();
	CABELL_SetPower();
	NRF24L01_SetBitrate(NRF24L01_BR_250K);				// slower data rate gives better range/reliability
	NRF24L01_WriteReg(NRF24L01_01_EN_AA, 0x00);			// No Auto Acknowledgment on all data pipes  
	NRF24L01_SetTxRxMode(TX_EN);						//Power up and 16 bit CRC

	CABELL_setAddress();

	NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);
	NRF24L01_FlushTx();
	NRF24L01_FlushRx();
	NRF24L01_WriteReg(NRF24L01_02_EN_RXADDR, 0x01);
	NRF24L01_WriteReg(NRF24L01_11_RX_PW_P0, 0x20);		// 32 byte packet length
	NRF24L01_WriteReg(NRF24L01_12_RX_PW_P1, 0x20);		// 32 byte packet length
	NRF24L01_WriteReg(NRF24L01_03_SETUP_AW, 0x03);
	NRF24L01_WriteReg(NRF24L01_04_SETUP_RETR, 0x5F);	// no retransmits
	NRF24L01_Activate(0x73);							// Activate feature register
	NRF24L01_WriteReg(NRF24L01_1C_DYNPD, 0x3F);			// Enable dynamic payload length on all pipes
	NRF24L01_WriteReg(NRF24L01_1D_FEATURE, 0x04);		// Enable dynamic Payload Length
	NRF24L01_Activate(0x73);
	prev_power = NRF_POWER_0;
}

//-----------------------------------------------------------------------------------------
static void CABELL_SetPower()    // This over-ride the standard Set Power to allow an flag in option to indicate max power setting
                          // Note that on many modules max power may actually be worse than the normal high power setting
                          // test and only use max if it helps the range
{
	if(IS_BIND_DONE && !IS_RANGE_FLAG_on && ((option & CABELL_OPTION_MASK_MAX_POWER_OVERRIDE) != 0))
	{   // If we are not in range or bind mode and power setting override is in effect, then set max power, else standard power logic
		if(prev_power != NRF_POWER_3)   // prev_power is global variable for NRF24L01; NRF_POWER_3 is max power
		{
			uint8_t rf_setup = NRF24L01_ReadReg(NRF24L01_06_RF_SETUP);
			rf_setup = (rf_setup & 0xF9) | (NRF_POWER_3 << 1);
			NRF24L01_WriteReg(NRF24L01_06_RF_SETUP, rf_setup);
			prev_power=NRF_POWER_3;
		}    
	}
	else
		NRF24L01_SetPower();
}

//-----------------------------------------------------------------------------------------
uint16_t CABELL_callback()
{
	if (IS_BIND_DONE)
	{
		CABELL_send_packet(0);  // packet_period is set/adjusted in CABELL_send_packet
		return packet_period;
	}
	if (bind_counter == 0)
	{
		BIND_DONE;
		CABELL_init();   // non-bind address 
	}
	else
	{
		CABELL_send_packet(1);
		bind_counter--;
	}
	return CABELL_PACKET_PERIOD;
}

//-----------------------------------------------------------------------------------------
uint16_t initCABELL(void)
{
	if (IS_BIND_DONE)
		bind_counter = 0;
	else  
		bind_counter = CABELL_BIND_COUNT;
	CABELL_init();
	#if defined CABELL_HUB_TELEMETRY 
		init_frskyd_link_telemetry();
		telemetry_lost=1;				// do not send telemetry to TX right away until we have a TX_RSSI value to prevent warning message...
	#endif

	packet_period = CABELL_PACKET_PERIOD;

	return packet_period;
}

#endif