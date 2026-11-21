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
/*
 FrSky ACCESS 2.4 GHz, from the ISRM-N 2.1.9 and RX6R 2.1.0 firmwares and
 captures of an X9 Lite ISRM with an RX6R 2.1.8.

 CC2500: 125 kbps 2-FSK, 69.8 kHz deviation, sync D391 sent twice (TX), the
 receiver's replies on 89CB, variable length, CRC. Channel n at 2403.97 MHz
 + n x 300 kHz (CHANNR, 299.93 kHz steps as Mike's UNI ACCESS). Every 7 ms
 a 33-byte frame on the next hop, the receiver's 18-byte reply 3.97 ms
 after its sync word on the same channel.

 Frame bytes (after the length byte, packet[n+1]):
   0..2   TX id (id low, id high, third byte)
   3..4   hop state: index | skip << 6 (index of the channel it is sent on)
   5      RX number, bits 6..7: 0x40 16 channels, 0x00 8 channels
   6      0x20 | slot: a failsafe value in that slot (channel n in frame n
          of every 1024); bits 6..7 region, 1 FCC, 0 EU (ISRM 0x239; the
          channels, hops and timing are the same in both, the RX6R ignores it)
   7      race mode switch (0)
   8..19  8 channel slots, 12 bits each, D16 packing; slot k = ch k or k+8
   20..30 S.Port downlink: seq/ack, count | receiver slot << 4, 0xFE, 8 bytes
   31..32 CRC (D16) of bytes 2..30, big endian
 Bytes 3 .. 3+n-1 XOR key = (id * 0xD7) ^ 0x83, n = key & 7 (3 at least).
 Reply: id (3 bytes), RSSI | 0x80 or RX voltage XOR (id * 0xD7) ^ 0x74,
 seq/ack, count, 0xFE, up to 8 bytes of the S.Port stream (as D16) or
 zeros, CRC over bytes 2..15 with byte 3 before its XOR.

 Bind (one bind does both steps):
 - register on channel 0: 03 01 01 00 <sub>, sub 0 until a receiver in
   register mode answers with its name, then subs 1..5 with its name, our
   registration ID (the module ID in hex) and UID 0
 - bind on channel 239: 03 01 02 00 00 + registration ID + UID (0..7 in
   turn) until a registered receiver answers with its name, then sub 1 with
   the name, slot 0, TX id and RX number, subs 2..5 to confirm
 Without an answer to the register frames for FRSKYA_REG_TRY frames, the
 module tries bind for as long (the receiver may be registered already).
 RX6R: register mode = bind button held at power-up until the LEDs
 alternate; it listens for bind after every plain power-up.
*/

#if defined(FRSKYA_CC2500_INO)

#include "iface_cc2500.h"

#define FRSKYA_PERIOD		7000
#define FRSKYA_TX_US		3900	// TX start to RX on: the frame is 2.9 ms on air, the reply starts ~4.2 ms
#define FRSKYA_RX_US		2800	// RX on to reading the reply (it ends at ~6.1 ms)
#define FRSKYA_LEN			33
#define FRSKYA_REPLY_LEN	18
#define FRSKYA_REG_CH		47		// hopping_frequency[47] = 0
#define FRSKYA_BIND_CH		48		// hopping_frequency[48] = 239
#define FRSKYA_REG_TRY		300		// frames (2.1 s)

enum {
	FRSKYA_REGISTER = 0,
	FRSKYA_BIND,
	FRSKYA_DATA,
};

static uint8_t FrSkyA_mode, FrSkyA_sub, FrSkyA_tries, FrSkyA_uid, FrSkyA_phase;
static uint16_t FrSkyA_count;
static uint8_t FrSkyA_name[8], FrSkyA_regid[8];
static uint8_t FrSkyA_chan_start;
// Subtypes: 16ch, 8ch (FCC), EU16ch, EU 8ch (EU): bit 0 = 8 channels
#define FRSKYA_8CH		(sub_protocol & 1)
#define FRSKYA_REGION	(sub_protocol >= A_EU16 ? 0x00 : 0x40)	// byte 6 bits 6..7
#ifdef FAILSAFE_ENABLE
	static bool FrSkyA_fs_valid;
	static uint16_t FrSkyA_fs_count;
#endif

// Lua pipe (EdgeTX multiBuffer "Pipe", MULTI telemetry type 18), messages as
// EdgeTX's PXX2 ones: type 5 = receiver settings (flag0: slot, 0x40 write;
// flag1; one channel byte per output), type 6 = hardware info (request: the
// receiver index; answer: index, model ID, hw and sw versions, variant,
// capabilities). In: type, offset << 4 | length, up to 5 bytes per MULTI
// frame (8 bytes are S.Port). Out: type, length, data.
// On air (ISRM 0x11af0 / 0xe23c / 0x115d8): downlink stream 5 in place of
// the S.Port data, byte 21 slot << 4 | count, 22 type, 23 offset << 4 |
// length, up to 7 bytes; the receiver answers the same way, up to 8 bytes.
#define FRSKYA_MSG_MAX 15
static uint8_t FrSkyA_req[FRSKYA_MSG_MAX];
static uint8_t FrSkyA_cfg_buf[FRSKYA_MSG_MAX], FrSkyA_cfg_len, FrSkyA_cfg_pos, FrSkyA_cfg_type;
static uint8_t FrSkyA_rep[FRSKYA_MSG_MAX];
uint8_t FrSkyA_lua_out[2 + FRSKYA_MSG_MAX], FrSkyA_lua_out_len;

void FrSkyA_lua_in(const uint8_t *d, uint8_t len)
{
	if(len < 2)
		return;
	uint8_t pos = d[1] >> 4, total = d[1] & 0x0F, n = len - 2;
	if(total == 0 || pos >= total)
		return;
	if(pos + n > total)
		n = total - pos;
	memcpy(&FrSkyA_req[pos], &d[2], n);
	if(pos + n < total)
		return;							// more to come
	if(d[0] == 6)
	{// Hardware info: the receiver index (ISRM 0x11660)
		FrSkyA_cfg_buf[0] = FrSkyA_req[0];
		FrSkyA_cfg_len = 1;
	}
	else if(d[0] != 5)
		return;
	else if(FrSkyA_req[0] & 0x40)
	{// Write: flag0, flag1, outputs
		memcpy(FrSkyA_cfg_buf, FrSkyA_req, total);
		FrSkyA_cfg_len = total;
	}
	else
	{// Read: flag0 and a flag1 byte, as the ISRM sends it
		FrSkyA_cfg_buf[0] = FrSkyA_req[0] & 0x0F;
		FrSkyA_cfg_buf[1] = 0;
		FrSkyA_cfg_len = 2;
	}
	FrSkyA_cfg_type = d[0];
	FrSkyA_cfg_pos = 0;
}

// The receiver's answer (reply frame bytes 5..15 at d): settings carry
// offset << 4 | length, hardware info only an offset (RX6R 0x11eec, one
// frame: index, model, hw and sw versions, variant, capabilities)
static void __attribute__((unused)) FrSkyA_stream_reply(const uint8_t *d)
{
	uint8_t n = (d[0] & 0x0F);
	if(n == 0)
		return;
	n--;								// the count includes the header
	if(n > 8) n = 8;
	uint8_t pos, total;
	if(d[1] == 6)
	{
		pos = d[2];
		total = pos + n;
		if(pos || total > FRSKYA_MSG_MAX)
			return;
	}
	else
	{
		pos = d[2] >> 4;
		total = d[2] & 0x0F;
		if(total == 0 || pos >= total)
			return;
	}
	if(pos + n > total)
		n = total - pos;
	memcpy(&FrSkyA_rep[pos], &d[3], n);
	if(pos + n < total)
		return;
	FrSkyA_lua_out[0] = d[1];
	FrSkyA_lua_out[1] = total;
	memcpy(&FrSkyA_lua_out[2], FrSkyA_rep, total);
	FrSkyA_lua_out_len = total + 2;
}

const PROGMEM uint8_t FRSKYA_cc2500_conf[]= {
	/*02_IOCFG0*/  	 0x06 ,
	/*00_IOCFG2*/  	 0x06 ,
	/*17_MCSM1*/   	 0x0C ,
	/*18_MCSM0*/   	 0x18 ,
	/*06_PKTLEN*/  	 0x26 ,
	/*07_PKTCTRL1*/	 0x04 ,	// status appended, no address check
	/*08_PKTCTRL0*/	 0x05 ,	// variable length, CRC
	/*3E_PATABLE*/ 	 0xff ,
	/*0B_FSCTRL1*/ 	 0x08 ,
	/*0C_FSCTRL0*/ 	 0x00 ,
	/*0D_FREQ2*/   	 0x5C ,	// 2403.970 MHz
	/*0E_FREQ1*/   	 0x75 ,
	/*0F_FREQ0*/   	 0xDC ,
	/*10_MDMCFG4*/ 	 0x6C ,	// 271 kHz, 125 kbps
	/*11_MDMCFG3*/ 	 0x3B ,
	/*12_MDMCFG2*/ 	 0x03 ,	// 2-FSK, 30/32 sync
	/*13_MDMCFG1*/ 	 0x23 ,	// 4 preamble bytes
	/*14_MDMCFG0*/ 	 0x7A ,	// 299.93 kHz
	/*15_DEVIATN*/ 	 0x53  };	// 69.8 kHz

static uint16_t __attribute__((unused)) FrSkyA_id()
{
	return rx_tx_addr[3] | (rx_tx_addr[2] << 8);
}

static uint8_t __attribute__((unused)) FrSkyA_key(uint8_t x)
{
	return (uint8_t)(rx_tx_addr[3] * 0xD7) ^ x;
}

static void __attribute__((unused)) FrSkyA_crc()
{// Frame bytes 2..30
	uint16_t lcrc = FrSkyX_crc(&packet[3], FRSKYA_LEN - 4);
	packet[FRSKYA_LEN - 1] = lcrc >> 8;
	packet[FRSKYA_LEN]     = lcrc;
}

static void __attribute__((unused)) FrSkyA_setup_frame(uint8_t type)
{
	memset(packet, 0, FRSKYA_LEN + 1);
	packet[0] = FRSKYA_LEN;
	packet[1] = 0x03;
	packet[2] = 0x01;
	packet[3] = type;					// 1 register, 2 bind
	packet[5] = FrSkyA_sub;
	if(type == 1)
	{
		if(FrSkyA_sub)
		{
			memcpy(&packet[6], FrSkyA_name, 8);
			memcpy(&packet[14], FrSkyA_regid, 8);
			packet[22] = 0;					// UID
		}
	}
	else
	{
		if(FrSkyA_sub == 0)
		{
			memcpy(&packet[6], FrSkyA_regid, 8);
			packet[14] = FrSkyA_uid;
			FrSkyA_uid = (FrSkyA_uid + 1) & 7;
		}
		else
		{
			memcpy(&packet[6], FrSkyA_name, 8);
			// packet[14..15]: receiver slot 0
			packet[16] = rx_tx_addr[3];
			packet[17] = rx_tx_addr[2];
			packet[18] = rx_tx_addr[1];
			packet[19] = RX_num & 0x3F;
			packet[20] = 0x01;
			if(FrSkyA_sub >= 2)
				packet[6] = 0xFF;
		}
	}
	FrSkyA_crc();
}

// Slot chan & 7: channel chan, or the failsafe value of fs_chan
static uint16_t __attribute__((unused)) FrSkyA_slot_value(uint8_t chan, uint8_t fs_slot, uint8_t fs_chan)
{
	#ifdef FAILSAFE_ENABLE
		if((chan & 0x07) == fs_slot)
			return FrSkyX_scaleForPXX_FS(fs_chan);
	#else
		(void)fs_slot; (void)fs_chan;
	#endif
	return FrSkyX_scaleForPXX(chan);
}

static void __attribute__((unused)) FrSkyA_data_frame()
{
	packet[0] = FRSKYA_LEN;
	packet[1] = rx_tx_addr[3];
	packet[2] = rx_tx_addr[2];
	packet[3] = rx_tx_addr[1];
	packet[4] = (FrSkyX_chanskip << 6) | hopping_frequency_no;
	packet[5] = FrSkyX_chanskip >> 2;
	packet[6] = (RX_num & 0x3F) | (FRSKYA_8CH ? 0x00 : 0x40);
	packet[7] = FRSKYA_REGION;			// no failsafe value in this frame
	packet[8] = 0;						// race mode off
	//Failsafe (ISRM 0x9854): frame n of every 1024 carries channel n's value in
	//slot n & 7 (PXX scale, +2048 for ch 9-16, 0 = no pulses, 2047 = hold, as
	//D16), byte 6 = 0x20 | slot. Once the radio has sent values (failsafe mode
	//"receiver": none, the receiver keeps its own)
	uint8_t fs_slot = 0xFF, fs_chan = 0;
	#ifdef FAILSAFE_ENABLE
		if(IS_FAILSAFE_VALUES_on)
		{
			FrSkyA_fs_valid = true;
			FrSkyA_fs_count = 0x3FF;	// new values: from ch 1 in the next frames
			FAILSAFE_VALUES_off;
		}
		FrSkyA_fs_count = (FrSkyA_fs_count + 1) & 0x3FF;
		if(FrSkyA_fs_valid && FrSkyA_fs_count < (FRSKYA_8CH ? 8 : 16))
		{
			fs_chan = FrSkyA_fs_count;
			fs_slot = fs_chan & 0x07;
			packet[7] = FRSKYA_REGION | 0x20 | fs_slot;
		}
	#endif
	//Channels, 8 slots: ch k or k+8 in turn (16 channels), ch k (8)
	uint8_t chan = FrSkyA_chan_start;
	for(uint8_t i = 9; i < 9 + 12; i += 3)
	{
		uint16_t ch1 = FrSkyA_slot_value(chan++, fs_slot, fs_chan);
		uint16_t ch2 = FrSkyA_slot_value(chan++, fs_slot, fs_chan);
		packet[i]   = ch1;
		packet[i+1] = ((ch1 >> 8) & 0x0F) | (ch2 << 4);
		packet[i+2] = ch2 >> 4;
	}
	if(FRSKYA_8CH)
		FrSkyA_chan_start = 0;
	else
		FrSkyA_chan_start ^= 0x08;
	//S.Port downlink: frame bytes 20 (seq), 21 (count, slot 0), 22 (type), 23..30
	FrSkyX_seq_sport(21, 30);			// data at packet[23..30]
	memmove(&packet[24], &packet[23], 8);
	packet[23] = 0xFE;					// sequenced S.Port
	if(FrSkyA_cfg_len && packet[22] == 0)
	{// Receiver settings in place of the S.Port data (none in this frame)
		uint8_t n = FrSkyA_cfg_len - FrSkyA_cfg_pos;
		if(n > 7) n = 7;
		packet[22] = ((FrSkyA_cfg_buf[0] & 0x03) << 4) | (n + 1);
		packet[23] = FrSkyA_cfg_type;
		packet[24] = FrSkyA_cfg_type == 6 ? FrSkyA_cfg_pos : (FrSkyA_cfg_pos << 4) | FrSkyA_cfg_len;	// ISRM 0x10630: offset only
		memset(&packet[25], 0, 7);
		memcpy(&packet[25], &FrSkyA_cfg_buf[FrSkyA_cfg_pos], n);
		FrSkyA_cfg_pos += n;
		if(FrSkyA_cfg_pos >= FrSkyA_cfg_len)
			FrSkyA_cfg_len = 0;
	}
	//XOR from frame byte 3
	uint8_t key = FrSkyA_key(0x83);
	uint8_t n = key & 7;
	if(n < 3) n = 3;
	for(uint8_t i = 0; i < n; i++)
		packet[4 + i] ^= key;
	FrSkyA_crc();
}

static void __attribute__((unused)) FrSkyA_set_sync(uint16_t sync)
{
	CC2500_WriteReg(CC2500_04_SYNC1, sync >> 8);
	CC2500_WriteReg(CC2500_05_SYNC0, sync);
}

// A reply in packet_in: 1 length byte, 18 bytes, 2 status bytes
static bool __attribute__((unused)) FrSkyA_read_reply()
{
	uint8_t len = CC2500_ReadReg(CC2500_3B_RXBYTES | CC2500_READ_BURST);
	if((len & 0x80) || (len & 0x7F) < FRSKYA_REPLY_LEN + 3)
		return false;						// overflow or nothing complete
	CC2500_ReadData(packet_in, FRSKYA_REPLY_LEN + 3);
	return packet_in[0] == FRSKYA_REPLY_LEN && (packet_in[FRSKYA_REPLY_LEN + 2] & 0x80);	// CC2500 CRC
}

static bool __attribute__((unused)) FrSkyA_setup_reply_ok(uint8_t type)
{// Same CRC as the TX frames (bytes 2..15)
	if(packet_in[1] != 0x03 || packet_in[2] != 0x01 || packet_in[3] != type || packet_in[5] != FrSkyA_sub)
		return false;
	uint16_t lcrc = FrSkyX_crc(&packet_in[3], FRSKYA_REPLY_LEN - 4);
	return packet_in[17] == (lcrc >> 8) && packet_in[18] == (lcrc & 0xFF);
}

static void __attribute__((unused)) FrSkyA_register_reply()
{
	if(!FrSkyA_setup_reply_ok(1))
		return;
	if(FrSkyA_sub == 0)
		memcpy(FrSkyA_name, &packet_in[6], 8);
	debugln("A reg %d", FrSkyA_sub);
	FrSkyA_sub++;
	FrSkyA_tries = 0;
	if(FrSkyA_sub > 5)
	{// Registered: on to bind
		FrSkyA_mode = FRSKYA_BIND;
		FrSkyA_sub = 0;
		FrSkyA_count = 0;
	}
}

static void __attribute__((unused)) FrSkyA_bind_reply()
{
	if(!FrSkyA_setup_reply_ok(2))
		return;
	if(FrSkyA_sub == 0)
		memcpy(FrSkyA_name, &packet_in[6], 8);
	debugln("A bind %d", FrSkyA_sub);
	FrSkyA_sub++;
	FrSkyA_tries = 0;
	if(FrSkyA_sub > 5)
	{
		BIND_DONE;
		FrSkyA_mode = FRSKYA_DATA;
		FrSkyX_telem_init();
	}
}

#if defined TELEMETRY
static void __attribute__((unused)) FrSkyA_telemetry()
{
	if(packet_in[1] != rx_tx_addr[3] || packet_in[2] != rx_tx_addr[2] || packet_in[3] != rx_tx_addr[1])
		return;
	uint8_t b3 = packet_in[4] ^ FrSkyA_key(0x74);
	// CRC over frame bytes 2..15, byte 3 before its XOR
	uint8_t tmp[FRSKYA_REPLY_LEN - 4];
	memcpy(tmp, &packet_in[3], FRSKYA_REPLY_LEN - 4);
	tmp[1] = b3;
	uint16_t lcrc = FrSkyX_crc(tmp, FRSKYA_REPLY_LEN - 4);
	if(packet_in[17] != (lcrc >> 8) || packet_in[18] != (lcrc & 0xFF))
		return;
	if(packet_in[7] == 5 || packet_in[7] == 6)
		FrSkyA_stream_reply(&packet_in[6]);
	// As a D16 telemetry frame for frsky_process_telemetry(): length, id,
	// id, byte, RSSI | 0x80 or voltage, seq, count, data, CRC (D16, of
	// bytes 3..15), RSSI, LQI. The ACCESS reply carries the same sequenced
	// S.Port byte stream as D16 (7E, stuffed), up to 8 bytes after the
	// stream type; bytes past the count are left over from older replies
	uint8_t d[20];
	uint8_t count = packet_in[7] == 0xFE ? packet_in[6] & 0x0F : 0;
	if(count > 8)
		count = 8;
	d[0] = 17;
	d[1] = packet_in[1];
	d[2] = packet_in[2];
	d[3] = packet_in[3];
	d[4] = b3;
	d[5] = packet_in[5];
	d[6] = count;
	memset(&d[7], 0, 9);
	memcpy(&d[7], &packet_in[8], count);
	lcrc = FrSkyX_crc(&d[3], 13);
	d[16] = lcrc >> 8;
	d[17] = lcrc;
	d[18] = packet_in[19];
	d[19] = packet_in[20];
	if(frsky_process_telemetry(d, 20))
	{
		pps_counter++;
		if(TX_LQI == 0)
			TX_LQI++;
	}
}
#endif

uint16_t FRSKYA_callback()
{
	#ifdef MULTI_EU
		if(sub_protocol < A_EU16)
			return FRSKYA_PERIOD;		// FCC subtypes: nothing sent in the EU builds
	#endif
	switch(FrSkyA_phase)
	{
		case 0:
		{// TX
			#ifdef MULTI_SYNC
				telemetry_set_input_sync(FRSKYA_PERIOD);
			#endif
			CC2500_SetFreqOffset();
			CC2500_SetPower();
			uint8_t ch;
			if(FrSkyA_mode != FRSKYA_DATA && IS_BIND_DONE)
			{// Bind ended by the radio
				FrSkyA_mode = FRSKYA_DATA;
				FrSkyX_telem_init();
			}
			if(FrSkyA_mode == FRSKYA_DATA)
			{
				ch = hopping_frequency_no;
				FrSkyA_data_frame();
			}
			else
			{
				if(++FrSkyA_count >= FRSKYA_REG_TRY && FrSkyA_sub == 0)
				{// Nobody answers: try the other step
					FrSkyA_count = 0;
					FrSkyA_mode = FrSkyA_mode == FRSKYA_REGISTER ? FRSKYA_BIND : FRSKYA_REGISTER;
				}
				ch = FrSkyA_mode == FRSKYA_REGISTER ? FRSKYA_REG_CH : FRSKYA_BIND_CH;
				FrSkyA_setup_frame(FrSkyA_mode == FRSKYA_REGISTER ? 1 : 2);
			}
			FrSkyX_set_start(ch);
			FrSkyA_set_sync(0xD391);
			CC2500_SetTxRxMode(TX_EN);
			CC2500_Strobe(CC2500_SFTX);
			CC2500_WriteData(packet, FRSKYA_LEN + 1);
			FrSkyA_phase = 1;
			return FRSKYA_TX_US;
		}
		case 1:
			// RX for the reply, same channel
			CC2500_Strobe(CC2500_SIDLE);
			FrSkyA_set_sync(0x89CB);
			CC2500_SetTxRxMode(RX_EN);
			CC2500_Strobe(CC2500_SFRX);
			CC2500_Strobe(CC2500_SRX);
			FrSkyA_phase = 2;
			return FRSKYA_RX_US;
		default:
		{// The reply, then the next hop
			bool got = FrSkyA_read_reply();
			CC2500_Strobe(CC2500_SIDLE);
			if(FrSkyA_mode == FRSKYA_REGISTER)
			{
				if(got) FrSkyA_register_reply();
			}
			else if(FrSkyA_mode == FRSKYA_BIND)
			{
				if(got) FrSkyA_bind_reply();
			}
			else
			{
				#if defined TELEMETRY
					if(got)
						FrSkyA_telemetry();
					if(millis() - pps_timer >= 700)
					{// 100 frames
						pps_timer = millis();
						TX_LQI = pps_counter;
						pps_counter = 0;
					}
					if(TX_LQI == 0)
						FrSkyX_telem_init();
					else
						telemetry_link = 1;
				#endif
				hopping_frequency_no = (hopping_frequency_no + FrSkyX_chanskip) % 47;
			}
			if(FrSkyA_mode != FRSKYA_DATA && FrSkyA_sub && ++FrSkyA_tries > 10)
			{// Lost the receiver in the middle of a step: start over
				FrSkyA_sub = 0;
				FrSkyA_tries = 0;
			}
			FrSkyA_phase = 0;
			return FRSKYA_PERIOD - FRSKYA_TX_US - FRSKYA_RX_US;
		}
	}
}

void FRSKYA_init()
{
	set_rx_tx_addr(MProtocol_id_master);
	FrSkyFormat = 0;					// FCC hop table
	FrSkyX2_init_hop();					// the ACCESS table, [47] = 0
	hopping_frequency[FRSKYA_BIND_CH] = 239;
	// Registration ID: the module ID in hex
	for(uint8_t i = 0; i < 8; i++)
	{
		uint8_t v = (MProtocol_id_master >> (28 - 4 * i)) & 0x0F;
		FrSkyA_regid[i] = v < 10 ? '0' + v : 'A' + v - 10;
	}
	FrSkyX_chanskip = 0;
	while(!FrSkyX_chanskip)
		FrSkyX_chanskip = random(0xfefefefe) % 47;
	hopping_frequency_no = 0;
	FrSkyA_chan_start = 0;

	FRSKY_init_cc2500(FRSKYA_cc2500_conf);
	// Frequency offset compensation only once a carrier is there (10 dB over
	// the noise): left running on noise before the reply, it drifts and 15 %
	// of the replies were missed. Without it, 15 % fail the CRC (the RX6R is
	// on 300 kHz steps, the CC2500 on 299.93 kHz).
	CC2500_WriteReg(CC2500_19_FOCCFG, 0x36);
	CC2500_WriteReg(CC2500_1C_AGCCTRL1, 0x68);
	for(uint8_t c = 0; c <= FRSKYA_BIND_CH; c++)
	{// Calibrate the hops, register and bind channels
		CC2500_Strobe(CC2500_SIDLE);
		CC2500_WriteReg(CC2500_0A_CHANNR, hopping_frequency[c]);
		CC2500_Strobe(CC2500_SCAL);
		delayMicroseconds(900);
		calData[c] = CC2500_ReadReg(CC2500_25_FSCAL1);
	}
	CC2500_WriteReg(CC2500_18_MCSM0, 0x08);	// no calibration: calData

	FrSkyA_mode = IS_BIND_IN_PROGRESS ? FRSKYA_REGISTER : FRSKYA_DATA;
	FrSkyA_sub = FrSkyA_tries = FrSkyA_uid = 0;
	FrSkyA_count = 0;
	FrSkyA_phase = 0;
	memcpy(FrSkyA_name, "RX      ", 8);
	FrSkyX_telem_init();
	FrSkyA_cfg_len = FrSkyA_lua_out_len = 0;
	#ifdef FAILSAFE_ENABLE
		FrSkyA_fs_valid = false;
		FrSkyA_fs_count = 0;
	#endif
}
#endif
