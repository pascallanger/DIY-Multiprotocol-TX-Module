/*
 This project is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 Deviation is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
 You should have received a copy of the GNU General Public License along with Deviation.  If not, see <http://www.gnu.org/licenses/>.
 */
#if defined(WK2x01_CYRF6936_INO)
#include "iface_cyrf6936.h"

#define PKTS_PER_CHANNEL 4

//Fewer bind packets in the emulator so we can get right to the important bits
#define WK_BIND_COUNT 2980

#define NUM_WAIT_LOOPS (100 / 5) //each loop is ~5us.  Do not wait more than 100us


#define WK_BIND 0
#define WK_BOUND_1 1
#define WK_BOUND_2 2
#define WK_BOUND_3 3
#define WK_BOUND_4 4
#define WK_BOUND_5 5
#define WK_BOUND_6 6
#define WK_BOUND_7 7
#define WK_BOUND_8 8

static const uint8_t sopcode[8] = {
    /* Note these are in order transmitted (LSB 1st) */
    0xDF,0xB1,0xC0,0x49,0x62,0xDF,0xC1,0x49 //0x49C1DF6249C0B1DF
};
static const uint8_t fail_map[8] = {2, 1, 0, 3, 4, 5, 6, 7};

static uint8_t wk_pkt_num;
static u8 *radio_ch_ptr;
static uint16_t WK_BIND_COUNTer;
static uint8_t last_beacon;
/*
static const char * const wk2601_opts[] = {
  _tr_noop("Chan mode"), _tr_noop("5+1"), _tr_noop("Heli"), _tr_noop("6+1"), NULL,
  _tr_noop("COL Inv"), _tr_noop("Normal"), _tr_noop("Inverted"), NULL,
  _tr_noop("COL Limit"), "-100", "100", NULL,
  NULL
};
*/
#define WK2601_OPT_CHANMODE 0
#define WK2601_OPT_PIT_INV 1
#define WK2601_OPT_PIT_LIMIT 2
#define LAST_PROTO_OPT 3

static void add_pkt_crc(uint8_t init) {
	uint8_t add = init;
	uint8_t xou = init;
	int i;
	for (i = 0; i < 14; i++) {	add += packet[i];	xou ^= packet[i];	}
	packet[14] = xou;
	packet[15] = add & 0xff;
}
static const char init_2801[] = {0xc5, 0x34, 0x60, 0x00, 0x25};
static const char init_2601[] = {0xb9, 0x45, 0xb0, 0xf1, 0x3a};
static const char init_2401[] = {0xa5, 0x23, 0xd0, 0xf0, 0x00};
static void build_bind_pkt(const char *init) {
	packet[0] = init[0];
	packet[1] = init[1];
	packet[2] = rx_tx_addr[0];
	packet[3] = rx_tx_addr[1];
	packet[4] = init[2];
	packet[5] = rx_tx_addr[2];
	packet[6] = 0xff;
	packet[7] = 0x00;
	packet[8] = 0x00;
	packet[9] = 0x32;
	if (sub_protocol == WK2401) { packet[10]  = 0x10 | ((fixed_id >> 0)  & 0x0e); }
	else { packet[10]  = (fixed_id >> 0) & 0xff; }
	packet[11] = (fixed_id >> 8)  & 0xff;
	packet[12] = ((fixed_id >> 12) & 0xf0) | wk_pkt_num;
	packet[13] = init[3];
	add_pkt_crc(init[4]);
}

static uint16_t get_channel(uint8_t ch, uint32_t scale, uint32_t center, uint32_t range) {
	uint32_t value = (uint32_t)Servo_data[ch] * scale / PPM_MAX + center;
	if (value < center - range) { value = center - range; }
	if (value > center + range) { value = center + range; }
	return value;
}

static void build_data_pkt_2401() {
	uint8_t i;
	uint16_t msb = 0;
	uint8_t offset = 0;
	for (i = 0; i < 4; i++) {
		if (i == 2) { offset = 1; }
		uint16_t value = get_channel(i, 0x800, 0, 0xA00); //12 bits, allow value to go to 125%
		uint16_t base = abs(value) >> 2;  //10 bits is the base value
		uint16_t trim = abs(value) & 0x03; //lowest 2 bits represent trim
		if (base >= 0x200) {  //if value is > 100%, remainder goes to trim
			trim = 4 *(base - 0x200);
			base = 0x1ff;
		}
		base = (value >= 0) ? 0x200 + base : 0x200 - base;
		trim = (value >= 0) ? 0x200 + trim : 0x200 - trim;

		packet[2*i+offset]   = base & 0xff;
		packet[2*i+offset+1] = trim & 0xff;
		msb = (msb << 4) | ((base >> 6) & 0x0c) | ((trim >> 8) & 0x03);
	}
	packet[4] = msb >> 8; //Ele/Ail MSB
	packet[9] = msb & 0xff; //Thr/Rud MSB
	packet[10]  = 0xe0 | ((fixed_id >> 0)  & 0x0e);
	packet[11] = (fixed_id >> 8)  & 0xff;
	packet[12] = ((fixed_id >> 12) & 0xf0) | wk_pkt_num;
	packet[13] = 0xf0; //FIXME - What is this?
	add_pkt_crc(0x00);
}

#define PCT(pct, max) (((max) * (pct) + 1L) / 1000)
#define MAXTHR 426 //Measured to provide equal value at +/-0
static void channels_6plus1_2601(int frame, int *_v1, int *_v2) {
	uint16_t thr = get_channel(2, 1000, 0, 1000);
	int v1;
	int thr_rev = 0, pitch_rev = 0;
	if(thr > 0) {
		if(thr >= 780) { //78%
			v1 = 0; //thr = 60% * (x - 78%) / 22% + 40%
			thr = PCT(1000-MAXTHR,512) * (thr-PCT(780,1000)) / PCT(220,1000) + PCT(MAXTHR,512);
		} else {
			v1 = 1023 - 1023 * thr / 780;
			thr = PCT(MAXTHR, 512); //40%
		}
	}
	else {
		thr = -thr;
		thr_rev = 1;
		if(thr >= 780) { //78%
			v1 = 1023; //thr = 60% * (x - 78%) / 22% + 40%
			thr = PCT(1000-MAXTHR,512) * (thr-PCT(780,1000)) / PCT(220,1000) + PCT(MAXTHR,512);
			if (thr >= 512) { thr = 511; }
		}
		else {
			v1 = 1023 * thr / 780;
			thr = PCT(MAXTHR, 512); //40%
		}
	}
	if (thr >= 512) { thr = 511; }
	packet[2] = thr & 0xff;
	packet[4] = (packet[4] & 0xF3) | ((thr >> 6) & 0x04);

	uint16_t pitch= get_channel(5, 0x400, 0, 0x400);
	if (pitch < 0) {
		pitch_rev = 1;
		pitch = -pitch;
	}
	if (frame == 1) {
		//Pitch curve and range
		if (thr > PCT(MAXTHR, 512)) { *_v2 = pitch - pitch * 16 * (thr - PCT(MAXTHR, 512)) / PCT(1000 - MAXTHR, 512) / 100; }
		else { *_v2 = pitch; }
		*_v1 = 0;
	}
	else if (frame == 2) {
		//Throttle curve & Expo
		*_v1 = v1;
		*_v2 = 512;
	}
	packet[7] = (thr_rev << 5) | (pitch_rev << 2); //reverse bits
	packet[8] = 0;
}

static void channels_5plus1_2601(int frame, int *v1, int *v2) {
	(void)v1;
	//Zero out pitch, provide ail, ele, thr, rud, gyr + gear
	if (frame == 1) { *v2 = 0; } //Pitch curve and range
	packet[7] = 0;
	packet[8] = 0;
}
static void channels_heli_2601(int frame, int *v1, int *v2) {
	(void)frame;
	//pitch is controlled by rx
	//we can only control fmode, pit-reverse and pit/thr rate
	int pit_rev = 0;
	if (Model.proto_opts[WK2601_OPT_PIT_INV]) { pit_rev = 1; }
	uint16_t pit_rate = get_channel(5, 0x400, 0, 0x400);
	int fmode = 1;
	if (pit_rate < 0) {		pit_rate = -pit_rate;	fmode = 0;	}
	if (frame == 1) {
		//Pitch curve and range
		*v1 = pit_rate;
		*v2 = Model.proto_opts[WK2601_OPT_PIT_LIMIT] * 0x400 / 100 + 0x400;
	}
	packet[7] = (pit_rev << 2); //reverse bits
	packet[8] = fmode ? 0x02 : 0x00;
}

static void build_data_pkt_2601() {
	uint8_t i;
	uint8_t msb = 0;
	uint8_t frame = (wk_pkt_num % 3);
	for (i = 0; i < 4; i++) {
		uint16_t value = get_channel(i, 0x190, 0, 0x1FF);
		uint16_t mag = value < 0 ? -value : value;
		packet[i] = mag & 0xff;
		msb = (msb << 2) | ((mag >> 8) & 0x01) | (value < 0 ? 0x02 : 0x00);
	}
	packet[4] = msb;
	int v1 = 0x200, v2 = 0x200;
	if (frame == 0) {
		//Gyro & Rudder mix
		v1 = get_channel(6, 0x200, 0x200, 0x200);
		v2 = 0;
	}
	if (Model.proto_opts[WK2601_OPT_CHANMODE] == 1) { channels_heli_2601(frame, &v1, &v2); }
	else if (Model.proto_opts[WK2601_OPT_CHANMODE] == 2) { channels_6plus1_2601(frame, &v1, &v2); }
	else { channels_5plus1_2601(frame, &v1, &v2); }
	if (v1 > 1023) { v1 = 1023; }
	if (v2 > 1023) { v2 = 1023; }
	packet[5] = v2 & 0xff;
	packet[6] = v1 & 0xff;
	//packet[7] handled by channel code
	packet[8] |= (get_channel(4, 0x190, 0, 0x1FF) > 0 ? 1 : 0);
	packet[9] =  ((v1 >> 4) & 0x30) | ((v2 >> 2) & 0xc0) | 0x04 | frame;
	packet[10]  = (fixed_id >> 0)  & 0xff;
	packet[11] = (fixed_id >> 8)  & 0xff;
	packet[12] = ((fixed_id >> 12) & 0xf0) | wk_pkt_num;
	packet[13] = 0xff;

	add_pkt_crc(0x3A);
}

static void build_data_pkt_2801() {
	uint8_t i;
	uint16_t msb = 0;
	uint8_t offset = 0;
	uint8_t sign = 0;
	for (i = 0; i < 8; i++) {
		if (i == 4) { offset = 1; }
		uint16_t value = get_channel(i, 0x190, 0, 0x3FF);
		uint16_t mag = value < 0 ? -value : value;
		packet[i+offset] = mag & 0xff;
		msb = (msb << 2) | ((mag >> 8) & 0x03);
		if (value < 0) { sign |= 1 << i; }
	}
	packet[4] = msb >> 8;
	packet[9] = msb  & 0xff;
	packet[10]  = (fixed_id >> 0)  & 0xff;
	packet[11] = (fixed_id >> 8)  & 0xff;
	packet[12] = ((fixed_id >> 12) & 0xf0) | wk_pkt_num;
	packet[13] = sign;
	add_pkt_crc(0x25);
}

static void build_beacon_pkt_2801() {
	last_beacon ^= 1;
	uint8_t i;
	uint8_t en = 0;
	uint8_t bind_state;
	if (WK_BIND_COUNTer) {	bind_state = 0xe4; }
	else {	bind_state = 0x1b; }
	for (i = 0; i < 4; i++) {
/*		if (Model.limits[fail_map[i + last_beacon * 4]].flags & CH_FAILSAFE_EN) {
			uint32_t value = Model.limits[fail_map[i + last_beacon * 4]].failsafe + 128;
			if (value > 255) { value = 255; }
			if (value < 0) { value = 0; }
			packet[i+1] = value;
			en |= 1 << i;
		} else 
*/		{ packet[i+1] = 0; }
	}
	packet[0] = en;
	packet[5] = packet[4];
	packet[4] = last_beacon << 6;
	packet[6] = rx_tx_addr[0];
	packet[7] = rx_tx_addr[1];
	packet[8] = rx_tx_addr[2];
	packet[9] = bind_state;
	packet[10]  = (fixed_id >> 0)  & 0xff;
	packet[11] = (fixed_id >> 8)  & 0xff;
	packet[12] = ((fixed_id >> 12) & 0xf0) | wk_pkt_num;
	packet[13] = 0x00; //Does this matter?  in the docs it is the same as the data packet
	add_pkt_crc(0x1C);
}

static void wk2x01_cyrf_init() {
	/* Initialise CYRF chip */
	CYRF_WriteRegister(CYRF_03_TX_CFG, 0x28 | CYRF_HIGH_POWER);
	CYRF_WriteRegister(CYRF_06_RX_CFG, 0x4A);
	CYRF_WriteRegister(CYRF_0B_PWR_CTRL, 0x00);
	CYRF_WriteRegister(CYRF_0C_XTAL_CTRL, 0xC0);
	CYRF_WriteRegister(CYRF_0D_IO_CFG, 0x04);
	CYRF_WriteRegister(CYRF_0F_XACT_CFG, 0x2C);
	CYRF_WriteRegister(CYRF_10_FRAMING_CFG, 0xEE);
	CYRF_WriteRegister(CYRF_1B_TX_OFFSET_LSB, 0x55);
	CYRF_WriteRegister(CYRF_1C_TX_OFFSET_MSB, 0x05);
	CYRF_WriteRegister(CYRF_1D_MODE_OVERRIDE, 0x18);
	CYRF_WriteRegister(CYRF_32_AUTO_CAL_TIME, 0x3C);
	CYRF_WriteRegister(CYRF_35_AUTOCAL_OFFSET, 0x14);
	CYRF_WriteRegister(CYRF_1E_RX_OVERRIDE, 0x90);
	CYRF_WriteRegister(CYRF_1F_TX_OVERRIDE, 0x00);
	CYRF_WriteRegister(CYRF_01_TX_LENGTH, 0x10);
	CYRF_WriteRegister(CYRF_0F_XACT_CFG, 0x2C);
	CYRF_WriteRegister(CYRF_28_CLK_EN, 0x02);
	CYRF_WriteRegister(CYRF_27_CLK_OVERRIDE, 0x02);
	CYRF_ConfigSOPCode(sopcode);
	CYRF_WriteRegister(CYRF_0F_XACT_CFG, 0x28);
	CYRF_WriteRegister(CYRF_1E_RX_OVERRIDE, 0x10);
	CYRF_WriteRegister(CYRF_0E_GPIO_CTRL, 0x20);
	CYRF_WriteRegister(CYRF_0F_XACT_CFG, 0x2C);
}

void WK_BuildPacket_2801() {
	switch(phase) {
		case WK_BIND:
			build_bind_pkt(init_2801);
//			if ((--WK_BIND_COUNTer == 0) || PROTOCOL_SticksMoved(0)) {
			if ((--WK_BIND_COUNTer == 0)) {
				WK_BIND_COUNTer = 0;
				BIND_DONE;
				phase = WK_BOUND_1;
			}
			break;
		case WK_BOUND_1:
		case WK_BOUND_2:
		case WK_BOUND_3:
		case WK_BOUND_4:
		case WK_BOUND_5:
		case WK_BOUND_6:
		case WK_BOUND_7:
			build_data_pkt_2801();
			phase++;
			break;
		case WK_BOUND_8:
			build_beacon_pkt_2801();
			phase = WK_BOUND_1;
			if (WK_BIND_COUNTer) {
				WK_BIND_COUNTer--;
				if (WK_BIND_COUNTer == 0) { BIND_DONE; }
			}
			break;
	}
	wk_pkt_num = (wk_pkt_num + 1) % 12;
}

void WK_BuildPacket_2601() {
	if (WK_BIND_COUNTer) {
		WK_BIND_COUNTer--;
		build_bind_pkt(init_2601);
		if ((WK_BIND_COUNTer == 0)) {
			WK_BIND_COUNTer = 0;
			BIND_DONE;
		}
	}
	else { build_data_pkt_2601(); }
	wk_pkt_num = (wk_pkt_num + 1) % 12;
}

void WK_BuildPacket_2401() {
	if (WK_BIND_COUNTer) {
		WK_BIND_COUNTer--;
		build_bind_pkt(init_2401);
		if ((WK_BIND_COUNTer == 0)) {
			WK_BIND_COUNTer = 0;
			BIND_DONE;
		}
	}
	else { build_data_pkt_2401(); }
	wk_pkt_num = (wk_pkt_num + 1) % 12;
}

static uint16_t wk_cb() {
	if (packet_sent == 0) {
		packet_sent = 1;
		if(sub_protocol == WK2801) {			WK_BuildPacket_2801(); }
		else if(sub_protocol == WK2601) {	WK_BuildPacket_2601(); }
		else if(sub_protocol == WK2401) {	WK_BuildPacket_2401(); }
		CYRF_WriteDataPacket(packet);
		return 1600;
	}
	packet_sent = 0;
	int i = 0;
	while (! (CYRF_ReadRegister(0x04) & 0x02)) { if(++i > NUM_WAIT_LOOPS) { break; } }
	if((wk_pkt_num & 0x03) == 0) {
		radio_ch_ptr = radio_ch_ptr == &rx_tx_addr[2] ? rx_tx_addr : radio_ch_ptr + 1;
		CYRF_ConfigRFChannel(*radio_ch_ptr);
		//Keep transmit power updated
		CYRF_WriteRegister(CYRF_03_TX_CFG, 0x28 | CYRF_HIGH_POWER);
	}
	return 1200;
}

static void wk_bind() {
	if((sub_protocol != WK2801)) { return; }
	fixed_id = ((MProtocol_id_master << 2)  & 0x0ffc00) | ((MProtocol_id_master >> 10) & 0x000300) | ((MProtocol_id_master)       & 0x0000ff);
	WK_BIND_COUNTer = WK_BIND_COUNT / 8 + 1;
	BIND_IN_PROGRESS;
}

static uint16_t wk_setup() {
	CYRF_Reset();
	wk2x01_cyrf_init();
	CYRF_SetTxRxMode(TX_EN);
	CYRF_FindBestChannels(rx_tx_addr, 3, 4, 4, 80);
	
	radio_ch_ptr = rx_tx_addr;
	CYRF_ConfigRFChannel(*radio_ch_ptr);

	wk_pkt_num = 0;
	packet_sent = 0;
	last_beacon = 0;
	fixed_id = ((MProtocol_id_master << 2)  & 0x0ffc00) | ((MProtocol_id_master >> 10) & 0x000300) | ((MProtocol_id_master)       & 0x0000ff);
	if (sub_protocol == WK2401) { fixed_id |= 0x01; }  //Fixed ID must be odd for 2401
	if(sub_protocol != WK2801) {
		WK_BIND_COUNTer = WK_BIND_COUNT;
		phase = WK_BIND;
		BIND_IN_PROGRESS;
	}
	else {
		phase = WK_BOUND_1;
		WK_BIND_COUNTer = 0;
	}
	CYRF_ConfigRFChannel(*radio_ch_ptr);
	return 2800;
}
/*
const void *WK2x01_Cmds(enum ProtoCmds cmd) {
    switch(cmd) {
        case PROTOCMD_INIT:  initialize(); return 0;
        case PROTOCMD_DEINIT: return 0;
        case PROTOCMD_CHECK_AUTOBIND:
            return (Model.protocol == WK2801 && Model.fixed_id) ? 0 : (void *)1L;
        case PROTOCMD_BIND:  wk_bind(); return 0;
        case PROTOCMD_DEFAULT_NUMCHAN: return (Model.protocol == WK2801)
              ? (void *)8L
              : (Model.protocol == WK2601)
                ? (void *)6L
                : (void *)4L;
        case PROTOCMD_NUMCHAN: return (Model.protocol == WK2801)
              ? (void *)8L
              : (Model.protocol == WK2601)
                ? (void *)7L
                : (void *)4L;
        case PROTOCMD_GETOPTIONS:
            if(Model.protocol == WK2601)
                return wk2601_opts;
            break;
        case PROTOCMD_TELEMETRYSTATE: return (void *)(long)PROTO_TELEM_UNSUPPORTED;
        default: break;
    }
    return 0;
}
*/
#endif
