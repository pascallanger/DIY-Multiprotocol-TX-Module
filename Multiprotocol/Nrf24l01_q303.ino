#ifdef Q303_NRF24L01_INO

#include "iface_nrf24l01.h"

#define Q303_BIND_COUNT 2335

#define Q303_PACKET_SIZE  10
#define Q303_PACKET_PERIOD   1500  // Timeout for callback in uSec
#define CX35_PACKET_PERIOD   3000
#define Q303_INITIAL_WAIT     500
#define Q303_RF_BIND_CHANNEL 0x02
#define Q303_NUM_RF_CHANNELS    4



static uint8_t tx_addr[5];
static uint8_t current_chan;

enum {
    Q303_BIND,
    Q303_DATA
};


// flags going to packet[8] (Q303)
#define Q303_FLAG_HIGHRATE 0x03
#define Q303_FLAG_AHOLD    0x40
#define Q303_FLAG_RTH      0x80

// flags going to packet[9] (Q303)
#define Q303_FLAG_GIMBAL_DN 0x04
#define Q303_FLAG_GIMBAL_UP 0x20
#define Q303_FLAG_HEADLESS  0x08
#define Q303_FLAG_FLIP      0x80
#define Q303_FLAG_SNAPSHOT  0x10
#define Q303_FLAG_VIDEO     0x01

static u8 cx35_lastButton()
{
	#define CX35_BTN_TAKEOFF  1
	#define CX35_BTN_DESCEND  2
	#define CX35_BTN_SNAPSHOT 4
	#define CX35_BTN_VIDEO    8
	#define CX35_BTN_RTH      16
	
    #define CX35_CMD_RATE     0x09
	#define CX35_CMD_TAKEOFF  0x0e
	#define CX35_CMD_DESCEND  0x0f
	#define CX35_CMD_SNAPSHOT 0x0b
	#define CX35_CMD_VIDEO    0x0c
	#define CX35_CMD_RTH      0x11
	
    static uint8_t cx35_btn_state;
    static uint8_t command;
    // simulate 2 keypress on rate button just after bind
    if(packet_counter < 50) {
        cx35_btn_state = 0;
        packet_counter++;
        command = 0x00; // startup
    }
    else if(packet_counter < 150) {
        packet_counter++;
        command = CX35_CMD_RATE; // 1st keypress
    }
    else if(packet_counter < 250) {
        packet_counter++;
        command |= 0x20; // 2nd keypress
    }
    
    // descend
    else if(!(GET_FLAG(CHANNEL_ARM, 1)) && !(cx35_btn_state & CX35_BTN_DESCEND)) {
        cx35_btn_state |= CX35_BTN_DESCEND;
        cx35_btn_state &= ~CX35_BTN_TAKEOFF;
        command = CX35_CMD_DESCEND;
    }
        
    // take off
    else if(GET_FLAG(CHANNEL_ARM,1) && !(cx35_btn_state & CX35_BTN_TAKEOFF)) {
        cx35_btn_state |= CX35_BTN_TAKEOFF;
        cx35_btn_state &= ~CX35_BTN_DESCEND;
        command = CX35_CMD_TAKEOFF;
    }
    
    // RTH
    else if(GET_FLAG(CHANNEL_RTH,1) && !(cx35_btn_state & CX35_BTN_RTH)) {
        cx35_btn_state |= CX35_BTN_RTH;
        if(command == CX35_CMD_RTH)
            command |= 0x20;
        else
            command = CX35_CMD_RTH;
    }
    else if(!(GET_FLAG(CHANNEL_RTH,1)) && (cx35_btn_state & CX35_BTN_RTH)) {
        cx35_btn_state &= ~CX35_BTN_RTH;
        if(command == CX35_CMD_RTH)
            command |= 0x20;
        else
            command = CX35_CMD_RTH;
    }
    
    // video
    else if(GET_FLAG(CHANNEL_VIDEO,1) && !(cx35_btn_state & CX35_BTN_VIDEO)) {
        cx35_btn_state |= CX35_BTN_VIDEO;
        if(command == CX35_CMD_VIDEO)
            command |= 0x20;
        else
            command = CX35_CMD_VIDEO;
    }
    else if(!(GET_FLAG(CHANNEL_VIDEO,1)) && (cx35_btn_state & CX35_BTN_VIDEO)) {
        cx35_btn_state &= ~CX35_BTN_VIDEO;
        if(command == CX35_CMD_VIDEO)
            command |= 0x20;
        else
            command = CX35_CMD_VIDEO;
    }
    
    // snapshot
    else if(GET_FLAG(CHANNEL_SNAPSHOT,1) && !(cx35_btn_state & CX35_BTN_SNAPSHOT)) {
        cx35_btn_state |= CX35_BTN_SNAPSHOT;
        if(command == CX35_CMD_SNAPSHOT)
            command |= 0x20;
        else
            command = CX35_CMD_SNAPSHOT;
    }
    
    if(!(GET_FLAG(CHANNEL_SNAPSHOT,1)))
        cx35_btn_state &= ~CX35_BTN_SNAPSHOT;
    
    return command;
}

static void send_packet(uint8_t bind)
{
	if(bind) {
		packet[0] = 0xaa;
		memcpy(&packet[1], txid, 4);
		memset(&packet[5], 0, 5);
	}
	else {
		aileron  = 1000 -	map(Servo_data[AILERON],	1000, 2000, 1000, 0);
		elevator = 1000 - 	map(Servo_data[ELEVATOR],	1000, 2000, 1000, 0);
		throttle = 			map(Servo_data[THROTTLE],	1000, 2000, 0, 1000);
		rudder   = 		 	map(Servo_data[RUDDER],		1000, 2000, 0, 1000);

		packet[0] = 0x55;
		packet[1] = aileron >> 2     ;     // 8 bits
		packet[2] = (aileron & 0x03) << 6  // 2 bits
				| (elevator >> 4);       // 6 bits
		packet[3] = (elevator & 0x0f) << 4 // 4 bits
				| (throttle >> 6);       // 4 bits
		packet[4] = (throttle & 0x3f) << 2 // 6 bits 
				| (rudder >> 8);         // 2 bits
		packet[5] = rudder & 0xff;         // 8 bits
        
        switch(sub_protocol) {
            case FORMAT_Q303:
				packet[6] = 0x10; // trim(s) ?
				packet[7] = 0x10; // trim(s) ?
				packet[8] = 0x03  // high rate (0-3)
						| GET_FLAG(Servo_AUX1, Q303_FLAG_AHOLD)
						| GET_FLAG(Servo_AUX6, Q303_FLAG_RTH);
				packet[9] = 0x40 // always set
						| GET_FLAG(Servo_AUX5, Q303_FLAG_HEADLESS)
						| GET_FLAG(Servo_AUX2, Q303_FLAG_FLIP)
						| GET_FLAG(Servo_AUX3, Q303_FLAG_SNAPSHOT)
						| GET_FLAG(Servo_AUX4, Q303_FLAG_VIDEO);
				if(Servo_data[AUX7] < (servo_max_100-PPM_SWITCH))
					packet[9] |= Q303_FLAG_GIMBAL_DN;
				else if(Servo_data[AUX7] > PPM_SWITCH)
					packet[9] |= Q303_FLAG_GIMBAL_UP;
				break;
            case FORMAT_CX35:
                slider = map(Servo_data[AUX7], 1000, 2000, 731, 342);	// GIMBAL
                packet[6] = slider >> 2;
                packet[7] = ((slider & 3) << 6)
                          | 0x3e; // ?? 6 bit left (always 111110 ?)
                
                packet[8] = 0x80; // always set
                packet[9] = cx35_lastButton();
                break;
		}
	}

	// Power on, TX mode, CRC enabled
    XN297_Configure(BV(NRF24L01_00_EN_CRC) | BV(NRF24L01_00_CRCO) | BV(NRF24L01_00_PWR_UP));

    NRF24L01_WriteReg(NRF24L01_05_RF_CH, bind ? Q303_RF_BIND_CHANNEL : hopping_frequency[current_chan++]);
    current_chan %= Q303_NUM_RF_CHANNELS;

    NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);
    NRF24L01_FlushTx();

    XN297_WritePayload(packet, Q303_PACKET_SIZE);

    // Check and adjust transmission power. We do this after
    // transmission to not bother with timeout after power
    // settings change -  we have plenty of time until next
    // packet.
    if (tx_power != Model.tx_power) {
        //Keep transmit power updated
        tx_power = Model.tx_power;
        NRF24L01_SetPower(tx_power);
    }
}

static uint16_t q303_callback()
{
	switch (phase) {
		case Q303_BIND:
			if (bind_counter == 0) {
				tx_addr[0] = 0x55;
				memcpy(&tx_addr[1], txid, 4);
				XN297_SetTXAddr(tx_addr, 5);
				phase = Q303_DATA;
				packet_counter = 0;
				BIND_DONE;
			} else {
				send_packet(1);
				bind_counter--;
			}
			break;
		case Q303_DATA:
			send_packet(0);
			break;
	}
	return packet_period;
}

static uint16_t q303_init()
{
	offset = txid[0] & 3;
	if(sub_protocol == FORMAT_CX35) {        
		for(i=0; i<Q303_NUM_RF_CHANNELS; i++)
			hopping_frequency[i] = 0x14 + i*3 + offset;
	}
	else{ // Q303
		for(i=0; i<Q303_NUM_RF_CHANNELS; i++)
			hopping_frequency[i] = 0x46 + i*2 + offset;
	}

	NRF24L01_Initialize();
	NRF24L01_SetTxRxMode(TX_EN);
    if(sub_protocol == FORMAT_CX35) {
        XN297_SetScrambledMode(XN297_SCRAMBLED);
        NRF24L01_SetBitrate(NRF24L01_BR_1M);
    }
    else { // Q303
        XN297_SetScrambledMode(XN297_UNSCRAMBLED);
        NRF24L01_SetBitrate(NRF24L01_BR_250K);
    }
	XN297_SetTXAddr((uint8_t *) "\xCC\xCC\xCC\xCC\xCC", 5);
	NRF24L01_FlushTx();
    NRF24L01_FlushRx();
	NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);     // Clear data ready, data sent, and retransmit
	NRF24L01_WriteReg(NRF24L01_01_EN_AA, 0x00);      // No Auto Acknowldgement on all data pipes
	NRF24L01_WriteReg(NRF24L01_02_EN_RXADDR, 0x01);
	NRF24L01_WriteReg(NRF24L01_03_SETUP_AW, 0x03);
	NRF24L01_WriteReg(NRF24L01_04_SETUP_RETR, 0x00); // no retransmits	NRF24L01_SetBitrate(NRF24L01_BR_250K);
	NRF24L01_SetPower();

	NRF24L01_Activate(0x73);                          // Activate feature register
	NRF24L01_WriteReg(NRF24L01_1C_DYNPD, 0x00);       // Disable dynamic payload length on all pipes
	NRF24L01_WriteReg(NRF24L01_1D_FEATURE, 0x01);     // Set feature bits on
	NRF24L01_Activate(0x73);
	
	BIND_IN_PROGRESS;
	bind_counter = Q303_BIND_COUNT;
    if(Model.proto_opts[PROTOOPTS_FORMAT] == FORMAT_Q303)
        packet_period = Q303_PACKET_PERIOD;
    else if(Model.proto_opts[PROTOOPTS_FORMAT] == FORMAT_CX35)
        packet_period = CX35_PACKET_PERIOD;
	current_chan = 0;
	phase = Q303_BIND;
	return Q303_INITIAL_WAIT;
}

#endif