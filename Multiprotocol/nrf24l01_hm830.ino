/*
 This project is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 Deviation is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
 You should have received a copy of the GNU General Public License along with Deviation.  If not, see <http://www.gnu.org/licenses/>.
 */

/* This protocol is for the HM Hobby HM830 RC Paper Airplane 
   Protocol spec: 
	Channel data:
		AA BB CC DD EE FF GG
		AA : Throttle Min=0x00 max =0x64
		BB : 
			bit 0,1,2: Left/Right magnitude, bit 5 Polarity (set = right)
			bit 6: Accelerate
			bit 7: Right button (also the ABC Button)
		CC : bit 0 seems to be impacted by the Right button
		DD
		EE
		FF : Trim (bit 0-5: Magnitude, bit 6 polarity (set = right)
		GG : Checksum (CRC8 on bytes AA-FF), init = 0xa5, poly = 0x01
*/

#ifdef HM830_NRF24L01_INO

#include "iface_nrf24l01.h"

enum {
    HM830_BIND1A = 0,
    HM830_BIND2A,
    HM830_BIND3A,
    HM830_BIND4A,
    HM830_BIND5A,
    HM830_BIND6A,
    HM830_BIND7A,
    HM830_DATA1,
    HM830_DATA2,
    HM830_DATA3,
    HM830_DATA4,
    HM830_DATA5,
    HM830_DATA6,
    HM830_DATA7,
    HM830_BIND1B = 0x80,
    HM830_BIND2B,
    HM830_BIND3B,
    HM830_BIND4B,
    HM830_BIND5B,
    HM830_BIND6B,
    HM830_BIND7B,
};

static const uint8_t init_vals[][2] = {
    {NRF24L01_17_FIFO_STATUS, 0x00},
    {NRF24L01_16_RX_PW_P5,    0x07},
    {NRF24L01_15_RX_PW_P4,    0x07},
    {NRF24L01_14_RX_PW_P3,    0x07},
    {NRF24L01_13_RX_PW_P2,    0x07},
    {NRF24L01_12_RX_PW_P1,    0x07},
    {NRF24L01_11_RX_PW_P0,    0x07},
    {NRF24L01_0F_RX_ADDR_P5,  0xC6},
    {NRF24L01_0E_RX_ADDR_P4,  0xC5},
    {NRF24L01_0D_RX_ADDR_P3,  0xC4},
    {NRF24L01_0C_RX_ADDR_P2,  0xC3},
    {NRF24L01_09_CD,          0x00},
    {NRF24L01_08_OBSERVE_TX,  0x00},
    {NRF24L01_07_STATUS,      0x07},
//    {NRF24L01_06_RF_SETUP,    0x07},
    {NRF24L01_05_RF_CH,       0x18},
    {NRF24L01_04_SETUP_RETR,  0x3F},
    {NRF24L01_03_SETUP_AW,    0x03},
    {NRF24L01_02_EN_RXADDR,   0x3F},
    {NRF24L01_01_EN_AA,       0x3F},
    {NRF24L01_00_CONFIG,      0x0E},
};

static uint8_t count;
static const uint8_t rf_ch[]     = {0x08, 0x35, 0x12, 0x3f, 0x1c, 0x49, 0x26};
static const uint8_t bind_addr[] = {0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xc2};

static uint8_t crc8(uint32_t result, uint8_t *data, int len) {
	int polynomial = 0x01;
	for(int i = 0; i < len; i++) {
		result = result ^ data[i];
		for(int j = 0; j < 8; j++) {
			if(result & 0x80) { result = (result << 1) ^ polynomial; }
			else { result = result << 1; }
		}
	}
	return result & 0xff;
}

static void HM830_init() {
	NRF24L01_Initialize();
	for (uint32_t i = 0; i < sizeof(init_vals) / sizeof(init_vals[0]); i++) { NRF24L01_WriteReg(init_vals[i][0], init_vals[i][1]); }

	NRF24L01_SetTxRxMode(TX_EN);
	NRF24L01_SetBitrate(0);
	NRF24L01_WriteRegisterMulti(NRF24L01_0A_RX_ADDR_P0, bind_addr,   5);
	NRF24L01_WriteRegisterMulti(NRF24L01_0B_RX_ADDR_P1, bind_addr+1, 5);
	NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR,    bind_addr,   5);
	NRF24L01_Activate(0x73);  //Enable FEATURE
	NRF24L01_WriteReg(NRF24L01_1D_FEATURE, 0x07);
	NRF24L01_WriteReg(NRF24L01_1C_DYNPD,   0x3F);
	//NRF24L01_ReadReg(NRF24L01_07_STATUS) ==> 0x07

	// Check for Beken BK2421/BK2423 chip
	// It is done by using Beken specific activate code, 0x53 and checking that status register changed appropriately
	// There is no harm to run it on nRF24L01 because following closing activate command changes state back even if it does something on nRF24L01
	// For detailed description of what's happening here see : http://www.inhaos.com/uploadfile/otherpic/AN0008-BK2423%20Communication%20In%20250Kbps%20Air%20Rate.pdf
	NRF24L01_Activate(0x53); // magic for BK2421 bank switch
//	printf("=>H377 : Trying to switch banks\n");
	if (NRF24L01_ReadReg(NRF24L01_07_STATUS) & 0x80) {
//		printf("=>H377 : BK2421 detected\n");
		long nul = 0;
		// Beken registers don't have such nice names, so we just mention them by their numbers
		// It's all magic, eavesdropped from real transfer and not even from the data sheet - it has slightly different values
		NRF24L01_WriteRegisterMulti(0x00, (uint8_t *) "\x40\x4B\x01\xE2", 4);
		NRF24L01_WriteRegisterMulti(0x01, (uint8_t *) "\xC0\x4B\x00\x00", 4);
		NRF24L01_WriteRegisterMulti(0x02, (uint8_t *) "\xD0\xFC\x8C\x02", 4);
		NRF24L01_WriteRegisterMulti(0x03, (uint8_t *) "\xF9\x00\x39\x21", 4);
		NRF24L01_WriteRegisterMulti(0x04, (uint8_t *) "\xC1\x96\x9A\x1B", 4);
		NRF24L01_WriteRegisterMulti(0x05, (uint8_t *) "\x24\x06\x7F\xA6", 4);
		NRF24L01_WriteRegisterMulti(0x06, (uint8_t *) &nul, 4);
		NRF24L01_WriteRegisterMulti(0x07, (uint8_t *) &nul, 4);
		NRF24L01_WriteRegisterMulti(0x08, (uint8_t *) &nul, 4);
		NRF24L01_WriteRegisterMulti(0x09, (uint8_t *) &nul, 4);
		NRF24L01_WriteRegisterMulti(0x0A, (uint8_t *) &nul, 4);
		NRF24L01_WriteRegisterMulti(0x0B, (uint8_t *) &nul, 4);
		NRF24L01_WriteRegisterMulti(0x0C, (uint8_t *) "\x00\x12\x73\x00", 4);
		NRF24L01_WriteRegisterMulti(0x0D, (uint8_t *) "\x46\xB4\x80\x00", 4);
		//NRF24L01_WriteRegisterMulti(0x0E, (uint8_t *) "\x41\x10\x04\x82\x20\x08\x08\xF2\x7D\xEF\xFF", 11);
		NRF24L01_WriteRegisterMulti(0x04, (uint8_t *) "\xC7\x96\x9A\x1B", 4);
		NRF24L01_WriteRegisterMulti(0x04, (uint8_t *) "\xC1\x96\x9A\x1B", 4);
	} else { }	//	printf("=>H377 : nRF24L01 detected\n");
	//NRF24L01_ReadReg(NRF24L01_07_STATUS) ==> 0x07
	NRF24L01_Activate(0x53); // switch bank back

	NRF24L01_FlushTx();
	//NRF24L01_ReadReg(NRF24L01_07_STATUS) ==> 0x0e
	NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x0e);
	//NRF24L01_ReadReg(NRF24L01_00_CONFIG); ==> 0x0e
	NRF24L01_WriteReg(NRF24L01_00_CONFIG, 0x0e);
	NRF24L01_ReadReg(NRF24L01_01_EN_AA);      // No Auto Acknoledgement
}

static void build_bind_packet() {
	for(int i = 0; i < 6; i++) { packet[i] = rx_tx_addr[i]; }
	packet[6] = crc8(0xa5, packet, 6);
}

static void build_data_packet() {
	uint8_t ail_sign = 0, trim_sign = 0;

	throttle = (uint32_t)Servo_data[2] * 50 / PPM_MAX + 50;
	if (throttle < 0) { throttle = 0; }

	aileron = (uint32_t)Servo_data[0] * 8 / PPM_MAX;
	if (aileron < 0) {	aileron = -aileron;	ail_sign = 1; }
	if (aileron > 7) { aileron = 7; }

	uint8_t turbo = (uint32_t)Servo_data[1] > 0 ? 1 : 0;

	uint8_t trim = ((uint32_t)Servo_data[3] * 0x1f / PPM_MAX);
	if (trim < 0) {		trim = -trim;	trim_sign = 1; }
	if (trim > 0x1f) { trim = 0x1f; }

	uint8_t rbutton = (uint32_t)Channels[4] > 0 ? 1 : 0;
	packet[0] = throttle;
	packet[1] = aileron;
	if (ail_sign) {	packet[1] |= 0x20; }
	if (turbo) { 	packet[1] |= 0x40; }
	if (rbutton) {	packet[1] |= 0x80; }
	packet[5] = trim;
	if (trim_sign) {	packet[5] |= 0x20;}
	packet[6] = crc8(0xa5, packet, 6);
}

static void send_packet_hm830() {
	NRF24L01_ReadReg(NRF24L01_17_FIFO_STATUS);
	NRF24L01_WritePayload(packet, 7);
}

static uint16_t handle_binding() {
	uint8_t status = NRF24L01_ReadReg(NRF24L01_07_STATUS);
	if (status & 0x20) {
		//Binding  complete
		phase = HM830_DATA1 + ((phase&0x7F)-HM830_BIND1A);
		count = 0;
		NRF24L01_WriteRegisterMulti(NRF24L01_0A_RX_ADDR_P0, rx_tx_addr,   5);
		NRF24L01_WriteRegisterMulti(NRF24L01_0B_RX_ADDR_P1, rx_tx_addr+1, 5);
		NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR,    rx_tx_addr,   5);
		NRF24L01_FlushTx();
		build_data_packet();
		uint8_t rb = NRF24L01_ReadReg(NRF24L01_07_STATUS); //==> 0x0E
		NRF24L01_WriteReg(NRF24L01_07_STATUS, rb);
		rb = NRF24L01_ReadReg(NRF24L01_00_CONFIG);    //==> 0x0E
		NRF24L01_WriteReg(NRF24L01_00_CONFIG, rb);
		send_packet_hm830();
		return 14000;
	}
	switch (phase) {
		case HM830_BIND1A:
			//Look for a Rx that is already bound
			NRF24L01_SetPower();
			NRF24L01_WriteRegisterMulti(NRF24L01_0A_RX_ADDR_P0, rx_tx_addr,   5);
			NRF24L01_WriteRegisterMulti(NRF24L01_0B_RX_ADDR_P1, rx_tx_addr+1, 5);
			NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR,    rx_tx_addr,   5);
			NRF24L01_WriteReg(NRF24L01_05_RF_CH, rf_ch[0]);
			build_bind_packet();
			break;
		case HM830_BIND1B:
			//Look for a Rx that is not yet bound
			NRF24L01_WriteRegisterMulti(NRF24L01_0A_RX_ADDR_P0, bind_addr,   5);
			NRF24L01_WriteRegisterMulti(NRF24L01_0B_RX_ADDR_P1, bind_addr+1, 5);
			NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR,    bind_addr,   5);
			NRF24L01_WriteReg(NRF24L01_05_RF_CH, rf_ch[0]);
			break;
		case HM830_BIND2A:
		case HM830_BIND3A:
		case HM830_BIND4A:
		case HM830_BIND5A:
		case HM830_BIND6A:
		case HM830_BIND7A:
		case HM830_BIND2B:
		case HM830_BIND3B:
		case HM830_BIND4B:
		case HM830_BIND5B:
		case HM830_BIND6B:
		case HM830_BIND7B:
			NRF24L01_WriteReg(NRF24L01_05_RF_CH, rf_ch[(phase&0x7F)-HM830_BIND1A]);
			break;
	}
	NRF24L01_FlushTx();
	uint8_t rb = NRF24L01_ReadReg(NRF24L01_07_STATUS); //==> 0x0E
	NRF24L01_WriteReg(NRF24L01_07_STATUS, rb);
	rb = NRF24L01_ReadReg(NRF24L01_00_CONFIG);    //==> 0x0E
	NRF24L01_WriteReg(NRF24L01_00_CONFIG, rb);
	send_packet_hm830();
	phase++;
	if (phase == HM830_BIND7B+1) { 		phase = HM830_BIND1A; }
	else if (phase == HM830_BIND7A+1) {	phase = HM830_BIND1B; }
	return 20000;
}

static uint16_t handle_data() {
	uint8_t status = NRF24L01_ReadReg(NRF24L01_07_STATUS);
	if (count <= 0 || !(status & 0x20)) {
		if(count < 0 || ! (status & 0x20)) {
			count = 0;
			//We didn't get a response on this channel, try the next one
			phase++;
			if (phase-HM830_DATA1 > 6) { phase = HM830_DATA1; }

			NRF24L01_WriteReg(NRF24L01_05_RF_CH, rf_ch[0]);
			NRF24L01_FlushTx();
			build_data_packet();
			uint8_t rb = NRF24L01_ReadReg(NRF24L01_07_STATUS); //==> 0x0E
			NRF24L01_WriteReg(NRF24L01_07_STATUS, rb);
			rb = NRF24L01_ReadReg(NRF24L01_00_CONFIG);    //==> 0x0E
			NRF24L01_WriteReg(NRF24L01_00_CONFIG, rb);
			send_packet_hm830();
			return 14000;
		}
	}
	build_data_packet();
	count++;
	if(count == 98) {
		count = -1;
		NRF24L01_SetPower();
	}
	uint8_t rb = NRF24L01_ReadReg(NRF24L01_07_STATUS); //==> 0x0E
	NRF24L01_WriteReg(NRF24L01_07_STATUS, rb);
	rb = NRF24L01_ReadReg(NRF24L01_00_CONFIG);    //==> 0x0E
	NRF24L01_WriteReg(NRF24L01_00_CONFIG, rb);
	send_packet_hm830();
	return 20000;
}



static uint32_t HM830_callback() {
    if ((phase & 0x7F) < HM830_DATA1) { return handle_binding(); }
    else { return handle_data(); }
}


static uint32_t HM830_setup(){
    count = 0;
	//	initialize_tx_id

	rx_tx_addr[4] = 0xee;
	rx_tx_addr[5] = 0xc2;
    HM830_init();
    phase = HM830_BIND1A;
	
	return 500;

//    CLOCK_StartTimer(50000, HM830_callback);
}

/*
const void *HM830_Cmds(enum ProtoCmds cmd)
{
    switch(cmd) {
        case PROTOCMD_INIT:  initialize(); return 0;
        case PROTOCMD_DEINIT:
        case PROTOCMD_RESET:
            CLOCK_StopTimer();
            return (void *)(NRF24L01_Reset() ? 1L : -1L);
        case PROTOCMD_CHECK_AUTOBIND: return (void *)1L; // Always Autobind
        case PROTOCMD_BIND:  initialize(); return 0;
        case PROTOCMD_NUMCHAN: return (void *) 5L; // T, A, E, R, G
        case PROTOCMD_DEFAULT_NUMCHAN: return (void *)5L;
        // TODO: return id correctly
        case PROTOCMD_CURRENT_ID: return Model.fixed_id ? (void *)((unsigned long)Model.fixed_id) : 0;
        case PROTOCMD_TELEMETRYSTATE: return (void *)(long)PROTO_TELEM_UNSUPPORTED;
        default: break;
    }
    return 0;
}
*/
#endif //PROTO_HAS_NRF24L01
