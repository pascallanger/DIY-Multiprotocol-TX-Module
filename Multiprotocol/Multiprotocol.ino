/*********************************************************
		Multiprotocol Tx code
		by Midelic and Pascal Langer(hpnuts)
		fork by Tipouic
  http://www.rcgroups.com/forums/showthread.php?t=2165676
    https://github.com/pascallanger/DIY-Multiprotocol-TX-Module/edit/master/README.md

<<<<<<< HEAD
	Thanks to PhracturedBlue, Hexfet, Goebish and all protocol developers
        Ported  from deviation firmware 
=======
	Thanks to PhracturedBlue, Hexfet, Goebish, Victzh and all protocol developers
				Ported  from deviation firmware 
>>>>>>> refs/remotes/pascallanger/master

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
#include <avr/eeprom.h>
#include <avr/pgmspace.h>
<<<<<<< HEAD
#include <util/delay.h>
=======
//#define DEBUG_TX
#include "Pins.h"
>>>>>>> refs/remotes/pascallanger/master
#include "Multiprotocol.h"

//Multiprotocol module configuration file
#include "_Config.h"
<<<<<<< HEAD

=======
#include "TX_Def.h"

#ifdef XMEGA
	#undef ENABLE_PPM			// Disable PPM for OrangeTX module
	#undef A7105_INSTALLED		// Disable A7105 for OrangeTX module
	#undef CC2500_INSTALLED		// Disable CC2500 for OrangeTX module
	#undef NRF24L01_INSTALLED	// Disable NRF for OrangeTX module
	#define TELEMETRY			// Enable telemetry
	#define INVERT_TELEMETRY	// Enable invert telemetry
	#define DSM_TELEMETRY		// Enable DSM telemetry
#endif

>>>>>>> refs/remotes/pascallanger/master
//Global constants/variables
uint32_t MProtocol_id;//tx id,
uint32_t MProtocol_id_master;
uint32_t blink=0;
//
uint16_t counter;
uint8_t  channel;
uint8_t  packet[40];

#define NUM_CHN 16
// Servo data
uint16_t Servo_data[NUM_CHN];
uint8_t  Servo_AUX;
<<<<<<< HEAD
const uint8_t ch[]={AILERON, ELEVATOR, THROTTLE, RUDDER, AUX1, AUX2, AUX3, AUX4};

// Protocol variables
uint8_t rx_tx_addr[5];
uint8_t phase;
=======
uint16_t servo_max_100,servo_min_100,servo_max_125,servo_min_125;

// Protocol variables
uint8_t  cyrfmfg_id[6];//for dsm2 and devo
uint8_t  rx_tx_addr[5];
uint8_t  phase;
>>>>>>> refs/remotes/pascallanger/master
uint16_t bind_counter;
uint8_t  bind_phase;
uint8_t  binding_idx;
uint16_t packet_period;
<<<<<<< HEAD
uint8_t packet_count;
uint8_t packet_sent;
uint8_t packet_length;
uint8_t hopping_frequency[23];
uint8_t *hopping_frequency_ptr;
uint8_t hopping_frequency_no=0;
uint8_t rf_ch_num;
uint8_t throttle, rudder, elevator, aileron;
uint8_t flags;
uint16_t crc;
//
uint32_t state;
uint8_t len;
uint8_t RX_num;
=======
uint8_t  packet_count;
uint8_t  packet_sent;
uint8_t  packet_length;
uint8_t  hopping_frequency[23];
uint8_t  *hopping_frequency_ptr;
uint8_t  hopping_frequency_no=0;
uint8_t  rf_ch_num;
uint8_t  throttle, rudder, elevator, aileron;
uint8_t  flags;
uint16_t crc;
uint8_t  crc8;
uint16_t seed;
//
uint16_t state;
uint8_t  len;
uint8_t  RX_num;

#if defined(FRSKYX_CC2500_INO) || defined(SFHSS_CC2500_INO)
	uint8_t calData[48];
#endif

//Channel mapping for protocols
const uint8_t CH_AETR[]={AILERON, ELEVATOR, THROTTLE, RUDDER, AUX1, AUX2, AUX3, AUX4, AUX5, AUX6, AUX7, AUX8};
const uint8_t CH_TAER[]={THROTTLE, AILERON, ELEVATOR, RUDDER, AUX1, AUX2, AUX3, AUX4, AUX5, AUX6, AUX7, AUX8};
const uint8_t CH_RETA[]={RUDDER, ELEVATOR, THROTTLE, AILERON, AUX1, AUX2, AUX3, AUX4, AUX5, AUX6, AUX7, AUX8};
const uint8_t CH_EATR[]={ELEVATOR, AILERON, THROTTLE, RUDDER, AUX1, AUX2, AUX3, AUX4, AUX5, AUX6, AUX7, AUX8};
>>>>>>> refs/remotes/pascallanger/master

// Mode_select variables
uint8_t mode_select;
uint8_t protocol_flags=0,protocol_flags2=0;

// PPM variable
volatile uint16_t PPM_data[NUM_CHN];

<<<<<<< HEAD
// Serial variables
#define RXBUFFER_SIZE 25
#define TXBUFFER_SIZE 12
volatile uint8_t rx_buff[RXBUFFER_SIZE];
volatile uint8_t rx_ok_buff[RXBUFFER_SIZE];
volatile uint8_t tx_buff[TXBUFFER_SIZE];
volatile uint8_t idx = 0;
=======
#ifndef XMEGA
//Random variable
volatile uint32_t gWDT_entropy=0;
#endif
>>>>>>> refs/remotes/pascallanger/master

//Serial protocol
uint8_t sub_protocol;
uint8_t protocol;
uint8_t option;
uint8_t cur_protocol[3];
uint8_t prev_option;
uint8_t prev_power=0xFD; // unused power value

//Serial RX variables
#define BAUD 100000
#define RXBUFFER_SIZE 26
volatile uint8_t rx_buff[RXBUFFER_SIZE];
volatile uint8_t rx_ok_buff[RXBUFFER_SIZE];
volatile uint8_t discard_frame = 0;

//Make sure telemetry is selected correctly
#ifndef TELEMETRY
	#undef INVERT_TELEMETRY
	#undef DSM_TELEMETRY	
	#undef SPORT_TELEMETRY	
	#undef HUB_TELEMETRY
#else
	#if not defined(CYRF6936_INSTALLED) || not defined(DSM_CYRF6936_INO)
		#undef DSM_TELEMETRY
	#endif
	#if (not defined(CC2500_INSTALLED) || not defined(FRSKYD_CC2500_INO)) && (not defined(A7105_INSTALLED) || not defined(HUBSAN_A7105_INO))
		#undef HUB_TELEMETRY
	#endif
	#if not defined(CC2500_INSTALLED) || not defined(FRSKYX_CC2500_INO)
		#undef SPORT_TELEMETRY
	#endif
#endif
#if not defined(DSM_TELEMETRY) && not defined(HUB_TELEMETRY) && not defined(SPORT_TELEMETRY)
	#undef TELEMETRY
	#undef INVERT_TELEMETRY
#endif

// Telemetry
#define MAX_PKT 27
uint8_t pkt[MAX_PKT];//telemetry receiving packets
#if defined(TELEMETRY)
<<<<<<< HEAD
uint8_t pktt[MAX_PKT];//telemetry receiving packets
	volatile uint8_t tx_head;
	volatile uint8_t tx_tail;
=======
	#ifdef INVERT_TELEMETRY
	// enable bit bash for serial
		#ifndef XMEGA
			#define	BASH_SERIAL 1
		#endif
		#define	INVERT_SERIAL 1
	#endif
	uint8_t pass = 0;
	uint8_t pktt[MAX_PKT];//telemetry receiving packets
	#ifndef BASH_SERIAL
		#define TXBUFFER_SIZE 32
		volatile uint8_t tx_buff[TXBUFFER_SIZE];
		volatile uint8_t tx_head=0;
		volatile uint8_t tx_tail=0;
	#endif // BASH_SERIAL
>>>>>>> refs/remotes/pascallanger/master
	uint8_t v_lipo;
	int16_t RSSI_dBm;
	//const uint8_t RSSI_offset=72;//69 71.72 values db
	uint8_t telemetry_link=0; 
	uint8_t telemetry_counter=0;
#endif 

// Callback
typedef uint16_t (*void_function_t) (void);//pointer to a function with no parameters which return an uint16_t integer
void_function_t remote_callback = 0;

// Init
void setup()
{
	#ifdef XMEGA
		// General pinout
		PORTD.OUTSET = 0x17 ;
		PORTD.DIRSET = 0xB2 ;
		PORTD.DIRCLR = 0x4D ;
		PORTD.PIN0CTRL = 0x18 ;
		PORTD.PIN2CTRL = 0x18 ;
		PORTE.DIRSET = 0x01 ;
		PORTE.DIRCLR = 0x02 ;
		// Timer1 config
		// TCC1 16-bit timer, clocked at 0.5uS
		EVSYS.CH3MUX = 0x80 + 0x04 ;	// Prescaler of 16
		TCC1.CTRLB = 0; TCC1.CTRLC = 0; TCC1.CTRLD = 0; TCC1.CTRLE = 0;
		TCC1.INTCTRLA = 0; TIMSK1 = 0;
		TCC1.PER = 0xFFFF ;
		TCNT1 = 0 ;
		TCC1.CTRLA = 0x0B ;	// Event3 (prescale of 16)
	#else	
		// General pinout
		// all inputs
		DDRB=0x00;DDRC=0x00;DDRD=0x00;
		// outputs
		SDI_output;
		SCLK_output;
		#ifdef A7105_INSTALLED
			A7105_CSN_output;
		#endif
		#ifdef CC2500_INSTALLED
			CC25_CSN_output;
		#endif
		#ifdef CYRF6936_INSTALLED
			CYRF_RST_output;
			CYRF_CSN_output;
		#endif
		#ifdef NRF24L01_INSTALLED
			NRF_CSN_output;
		#endif
		PE1_output;
		PE2_output;
		SERIAL_TX_output;

		// pullups
		MODE_DIAL1_port |= _BV(MODE_DIAL1_pin);
		MODE_DIAL2_port |= _BV(MODE_DIAL2_pin);
		MODE_DIAL3_port |= _BV(MODE_DIAL3_pin);
		MODE_DIAL4_port |= _BV(MODE_DIAL4_pin);
		BIND_port |= _BV(BIND_pin);

		// Timer1 config
		TCCR1A = 0;
		TCCR1B = (1 << CS11);	//prescaler8, set timer1 to increment every 0.5us(16Mhz) and start timer
		
		// Random
		random_init();
	#endif

	// Set Chip selects
<<<<<<< HEAD
	CS_on;
	CC25_CSN_on;
	NRF_CSN_on;
	CYRF_CSN_on;
	//  Set SPI lines
	SDI_on;
	SCK_off;

	// Timer1 config
	TCCR1A = 0;
	TCCR1B = (1 << CS11); //prescaler8, set timer1 to increment every 0.5us(16Mhz) and start timer
=======
	#ifdef A7105_INSTALLED
		A7105_CSN_on;
	#endif
	#ifdef CC2500_INSTALLED
		CC25_CSN_on;
	#endif
	#ifdef CYRF6936_INSTALLED
		CYRF_CSN_on;
	#endif
	#ifdef NRF24L01_INSTALLED
		NRF_CSN_on;
	#endif
	//	Set SPI lines
	SDI_on;
	SCLK_off;
>>>>>>> refs/remotes/pascallanger/master

	// Set servos positions
	for(uint8_t i=0;i<NUM_CHN;i++)
		Servo_data[i]=1500;
<<<<<<< HEAD
	Servo_data[THROTTLE]=PPM_MIN_100;
	memcpy((void *)PPM_data,Servo_data, sizeof(Servo_data));

	//Wait for every component to start
	delay(100);
	
	// Read status of bind button
	if( (PINB & _BV(5)) == 0x00 )
		BIND_BUTTON_FLAG_on;  // If bind button pressed save the status for protocol id reset under hubsan

	// Read status of mode select binary switch
	// after this mode_select will be one of {0000, 0001, ..., 1111}
	mode_select=0x0F - ( ( (PINB>>2)&0x07 ) | ( (PINC<<3)&0x08) );//encoder dip switches 1,2,4,8=>B2,B3,B4,C0
	//**********************************
//mode_select=1;	// here to test PPM
	//**********************************

	// Update LED
	LED_OFF; 
	LED_SET_OUTPUT; 

	// Read or create protocol id
	MProtocol_id_master=random_id(10,false);

	//Init RF modules
	#ifdef	CC2500_INSTALLED
		CC2500_Reset();
	#endif

=======
	Servo_data[THROTTLE]=servo_min_100;
	#ifdef ENABLE_PPM
		memcpy((void *)PPM_data,Servo_data, sizeof(Servo_data));
	#endif
	
	//Wait for every component to start
	delayMilliseconds(100);
	
	// Read status of bind button
	if( IS_BIND_BUTTON_on )
		BIND_BUTTON_FLAG_on;	// If bind button pressed save the status for protocol id reset under hubsan

	// Read status of mode select binary switch
	// after this mode_select will be one of {0000, 0001, ..., 1111}
	#ifndef ENABLE_PPM
		mode_select = MODE_SERIAL ;	// force serial mode
	#else
		mode_select =
			((MODE_DIAL1_ipr & _BV(MODE_DIAL1_pin)) ? 0 : 1) + 
			((MODE_DIAL2_ipr & _BV(MODE_DIAL2_pin)) ? 0 : 2) +
			((MODE_DIAL3_ipr & _BV(MODE_DIAL3_pin)) ? 0 : 4) +
			((MODE_DIAL4_ipr & _BV(MODE_DIAL4_pin)) ? 0 : 8);
	#endif

	// Update LED
	LED_off;
	LED_output; 

	//Init RF modules
	modules_reset();

#ifndef XMEGA
	//Init the seed with a random value created from watchdog timer for all protocols requiring random values
	randomSeed(random_value());
#endif

	// Read or create protocol id
	MProtocol_id_master=random_id(10,false);
	
#ifdef ENABLE_PPM
>>>>>>> refs/remotes/pascallanger/master
	//Protocol and interrupts initialization
	if(mode_select != MODE_SERIAL)
	{ // PPM
		mode_select--;
<<<<<<< HEAD
		cur_protocol[0]	=	PPM_prot[mode_select].protocol;
=======
		protocol		=	PPM_prot[mode_select].protocol;
		cur_protocol[1] = protocol;
>>>>>>> refs/remotes/pascallanger/master
		sub_protocol   	=	PPM_prot[mode_select].sub_proto;
		RX_num			=	PPM_prot[mode_select].rx_num;
		MProtocol_id	=	RX_num + MProtocol_id_master;
		option			=	PPM_prot[mode_select].option;
		if(PPM_prot[mode_select].power)		POWER_FLAG_on;
		if(PPM_prot[mode_select].autobind)	AUTOBIND_FLAG_on;
		mode_select++;
<<<<<<< HEAD

		protocol_init();
		
		//Configure PPM interrupt
		EICRA |=(1<<ISC11);		// The rising edge of INT1 pin D3 generates an interrupt request
		EIMSK |= (1<<INT1);		// INT1 interrupt enable
		#if defined(TELEMETRY)
				PPM_Telemetry_serial_init();		// Configure serial for telemetry
=======
		servo_max_100=PPM_MAX_100; servo_min_100=PPM_MIN_100;
		servo_max_125=PPM_MAX_125; servo_min_125=PPM_MIN_125;

		protocol_init();

		//Configure PPM interrupt
		#if PPM_pin == 2
			EICRA |= _BV(ISC01);	// The rising edge of INT0 pin D2 generates an interrupt request
			EIMSK |= _BV(INT0);		// INT0 interrupt enable
		#elif PPM_pin == 3
			EICRA |= _BV(ISC11);	// The rising edge of INT1 pin D3 generates an interrupt request
			EIMSK |= _BV(INT1);		// INT1 interrupt enable
		#else
			#error PPM pin can only be 2 or 3
		#endif

		#if defined(TELEMETRY)
			PPM_Telemetry_serial_init();		// Configure serial for telemetry
>>>>>>> refs/remotes/pascallanger/master
		#endif
	}
	else
#endif //ENABLE_PPM
	{ // Serial
		#ifdef ENABLE_SERIAL
			for(uint8_t i=0;i<3;i++)
				cur_protocol[i]=0;
			protocol=0;
			servo_max_100=SERIAL_MAX_100; servo_min_100=SERIAL_MIN_100;
			servo_max_125=SERIAL_MAX_125; servo_min_125=SERIAL_MIN_125;
			Mprotocol_serial_init(); // Configure serial and enable RX interrupt
		#endif //ENABLE_SERIAL
	}
}

// Main
// Protocol scheduler
void loop()
<<<<<<< HEAD
{
		if(mode_select==MODE_SERIAL && IS_RX_FLAG_on)	// Serial mode and something has been received
		{
			update_serial_data(); // Update protocol and data
			update_aux_flags();
		if(IS_CHANGE_PROTOCOL_FLAG_on)
		{ // Protocol needs to be changed
		LED_OFF;                  //led off during protocol init
		module_reset();               //reset previous module
		protocol_init();							//init new protocol
		CHANGE_PROTOCOL_FLAG_off;         //done
	}
	}
	if(mode_select!=MODE_SERIAL && IS_PPM_FLAG_on)	// PPM mode and a full frame has been received
	{
		for(uint8_t i=0;i<NUM_CHN;i++)
		{ // update servo data without interrupts to prevent bad read in protocols
			cli();	// disable global int
			Servo_data[i]=PPM_data[i];
			sei();	// enable global int
		}
		update_aux_flags();
		PPM_FLAG_off;	// wait for next frame before update
	}
	update_led_status();
	#if defined(TELEMETRY)
	if( ((cur_protocol[0]&0x1F)==MODE_FRSKY) || ((cur_protocol[0]&0x1F)==MODE_HUBSAN) || ((cur_protocol[0]&0x1F)==MODE_FRSKYX) )
		frskyUpdate();
	#endif 
	if (remote_callback != 0)
		CheckTimer(remote_callback); 
=======
{ 
	uint16_t next_callback,diff=0xFFFF;

	while(1)
	{
		if(remote_callback==0 || diff>2*200)
		{
			do
			{
				Update_All();
			}
			while(remote_callback==0);
		}
		if( (TIFR1 & OCF1A_bm) != 0)
		{
			cli();					// Disable global int due to RW of 16 bits registers
			OCR1A=TCNT1;			// Callback should already have been called... Use "now" as new sync point.
			sei();					// Enable global int
		}
		else
			while((TIFR1 & OCF1A_bm) == 0); // Wait before callback
		do
		{
			TX_MAIN_PAUSE_on;
			tx_pause();
			next_callback=remote_callback();
			TX_MAIN_PAUSE_off;
			tx_resume();
			while(next_callback>4000)
			{ // start to wait here as much as we can...
				next_callback-=2000;				// We will wait below for 2ms
				cli();								// Disable global int due to RW of 16 bits registers
				OCR1A += 2000*2 ;					// set compare A for callback
				TIFR1=OCF1A_bm;						// clear compare A=callback flag
				sei();								// enable global int
				Update_All();
				if(IS_CHANGE_PROTOCOL_FLAG_on)
					break; // Protocol has been changed
				while((TIFR1 & OCF1A_bm) == 0);		// wait 2ms...
			}
			// at this point we have a maximum of 4ms in next_callback
			next_callback *= 2 ;
			cli();									// Disable global int due to RW of 16 bits registers
			OCR1A+= next_callback ;					// set compare A for callback
			TIFR1=OCF1A_bm;							// clear compare A=callback flag
			diff=OCR1A-TCNT1;						// compare timer and comparator
			sei();									// enable global int
		}
		while(diff&0x8000);	 						// Callback did not took more than requested time for next callback
													// so we can launch Update_All before next callback
	}
}

void Update_All()
{
	#ifdef ENABLE_SERIAL
		if(mode_select==MODE_SERIAL && IS_RX_FLAG_on)	// Serial mode and something has been received
		{
			update_serial_data();	// Update protocol and data
			update_aux_flags();
			if(IS_CHANGE_PROTOCOL_FLAG_on)
			{ // Protocol needs to be changed
				LED_off;									//led off during protocol init
				modules_reset();							//reset all modules
				protocol_init();							//init new protocol
			}
		}
	#endif //ENABLE_SERIAL
	#ifdef ENABLE_PPM
		if(mode_select!=MODE_SERIAL && IS_PPM_FLAG_on)	// PPM mode and a full frame has been received
		{
			for(uint8_t i=0;i<NUM_CHN;i++)
			{ // update servo data without interrupts to prevent bad read in protocols
				uint16_t temp_ppm ;
				cli();	// disable global int
				temp_ppm = PPM_data[i] ;
				sei();	// enable global int
				if(temp_ppm<PPM_MIN_125) temp_ppm=PPM_MIN_125;
				else if(temp_ppm>PPM_MAX_125) temp_ppm=PPM_MAX_125;
				Servo_data[i]= temp_ppm ;
			}
			update_aux_flags();
			PPM_FLAG_off;	// wait for next frame before update
		}
	#endif //ENABLE_PPM
	update_led_status();
	#if defined(TELEMETRY)
		if((protocol==MODE_FRSKYD) || (protocol==MODE_HUBSAN) || (protocol==MODE_FRSKYX) || (protocol==MODE_DSM) )
			TelemetryUpdate();
	#endif
>>>>>>> refs/remotes/pascallanger/master
}

// Update Servo_AUX flags based on servo AUX positions
static void update_aux_flags(void)
{
	Servo_AUX=0;
	for(uint8_t i=0;i<8;i++)
		if(Servo_data[AUX1+i]>PPM_SWITCH)
			Servo_AUX|=1<<i;
}

// Update led status based on binding and serial
static void update_led_status(void)
{
	if(blink<millis())
	{
<<<<<<< HEAD
		if(cur_protocol[0]==0)  // No valid serial received at least once
			blink+=BLINK_SERIAL_TIME;         //blink slowly while waiting a valid serial input
=======
		if(cur_protocol[1]==0)							//No valid serial received at least once
			blink+=BLINK_SERIAL_TIME;					//blink slowly while waiting a valid serial input
>>>>>>> refs/remotes/pascallanger/master
		else
			if(remote_callback == 0)
			{ // Invalid protocol
			if(IS_LED_on)             //flash to indicate invalid protocol
				blink+=BLINK_BAD_PROTO_TIME_LOW;
			else
				blink+=BLINK_BAD_PROTO_TIME_HIGH;
		}
			else
				if(IS_BIND_DONE_on)
<<<<<<< HEAD
			LED_OFF;                  //bind completed -> led on
		else
			blink+=BLINK_BIND_TIME;         //blink fastly during binding
		LED_TOGGLE;
	}
}

// Protocol scheduler
static void CheckTimer(uint16_t (*cb)(void))
{ 
	uint16_t next_callback;
	uint32_t prev;
	if( (TIFR1 & (1<<OCF1A)) != 0)
	{
		cli();      // disable global int
		OCR1A=TCNT1;  // Callback should already have been called... Use "now" as new sync point.
		sei();      // enable global int
	}
	else
	while((TIFR1 & (1<<OCF1A)) == 0); // wait before callback
		prev=micros();
	next_callback=cb();
	if(prev+next_callback+50 > micros())
	{ // Callback did not took more than requested time for next callback
		if(next_callback>32000)
		{ // next_callback should not be more than 32767 so we will wait here...
			delayMicroseconds(next_callback-2000);
			cli();      // disable global int
			OCR1A=TCNT1+4000;
			sei();      // enable global int
		}
		else
		{
			cli();      // disable global int
			OCR1A+=next_callback*2;   // set compare A for callback
			sei();      // enable global int
		}
		TIFR1=(1<<OCF1A); // clear compare A=callback flag
	}
=======
					LED_off;							//bind completed -> led on
				else
					blink+=BLINK_BIND_TIME;				//blink fastly during binding
		LED_toggle;
	}
}

inline void tx_pause()
{
	#ifdef TELEMETRY
		#ifdef XMEGA
			USARTC0.CTRLA &= ~0x03 ;		// Pause telemetry by disabling transmitter interrupt
		#else
			#ifndef BASH_SERIAL
				UCSR0B &= ~_BV(UDRIE0);		// Pause telemetry by disabling transmitter interrupt
			#endif
		#endif
	#endif
}

inline void tx_resume()
{
	#ifdef TELEMETRY
		if(!IS_TX_PAUSE_on)
		{
			#ifdef XMEGA
				cli() ;
				USARTC0.CTRLA = (USARTC0.CTRLA & 0xFC) | 0x01 ;	// Resume telemetry by enabling transmitter interrupt
				sei() ;
			#else
				#ifndef BASH_SERIAL
					UCSR0B |= _BV(UDRIE0);			// Resume telemetry by enabling transmitter interrupt
				#else
					resumeBashSerial() ;
				#endif
			#endif
		}
	#endif
>>>>>>> refs/remotes/pascallanger/master
}

// Protocol start
static void protocol_init()
{
	uint16_t next_callback=0;		// Default is immediate call back
	remote_callback = 0;

	// reset telemetry
	#ifdef TELEMETRY
		tx_pause();
		pass=0;
		telemetry_link=0;
		#ifndef BASH_SERIAL
			tx_tail=0;
			tx_head=0;
		#endif
	#endif

	blink=millis();
	if(IS_BIND_BUTTON_FLAG_on)
		AUTOBIND_FLAG_on;
	if(IS_AUTOBIND_FLAG_on)
		BIND_IN_PROGRESS;     // Indicates bind in progress for blinking bind led
	else
		BIND_DONE;

<<<<<<< HEAD
	CTRL1_on; //NRF24L01 antenna RF3 by default
	CTRL2_off;  //NRF24L01 antenna RF3 by default
  
	switch(cur_protocol[0]&0x1F)	// Init the requested protocol
	{
#if defined(HM830_NRF24L01_INO)
		case MODE_HM830:
			next_callback=HM830_setup();
			remote_callback = HM830_callback;
			break;
#endif
#if defined(CFlie_NRF24L01_INO)
		case MODE_CFLIE:
			next_callback=Cflie_setup();
			remote_callback = cflie_callback;
			break;
#endif
#if defined(JOYSWAY_A7105_INO)
		case MODE_JOYSWAY:
			next_callback=JOYSWAY_Setup();
			remote_callback = joysway_cb;
			break;
#endif
#if defined(H377_NRF24L01_INO)
		case MODE_H377:
			next_callback=h377_setup();
			remote_callback = h377_cb;
			break;
#endif
#if defined(J6PRO_CYRF6936_INO)
		case MODE_J6PRO:
			next_callback=j6pro_setup();
			remote_callback = j6pro_cb;
			break;
#endif
#if defined(WK2x01_CYRF6936_INO)
		case MODE_WK2x01:
			next_callback=wk_setup();
			remote_callback = wk_cb;
			break;
#endif
#if defined(ESKY150_NRF24L01_INO)
		case MODE_ESKY150:
			next_callback=esky150_setup();
			remote_callback = esky150_callback;
			break;
#endif
#if defined(BlueFly_NRF24L01_INO)
		case MODE_BlueFly:
			next_callback=BlueFly_setup();
			remote_callback = bluefly_cb;
			break;
#endif
#if defined(HonTai_NRF24L01_INO)
		case MODE_HonTai:
			next_callback=ht_setup();
			remote_callback = ht_callback;
			break;
#endif
#if defined(UDI_NRF24L01_INO)
		case MODE_UDI:
			next_callback=UDI_setup();
			remote_callback = UDI_callback;
			break;
#endif
#if defined(NE260_NRF24L01_INO)
		case MODE_NE260:
			next_callback=NE260_setup();
			remote_callback = ne260_cb;
			break;
#endif
#if defined(SKYARTEC_CC2500_INO)
		case MODE_SKYARTEC:
			next_callback=skyartec_setup();
			remote_callback = skyartec_cb;
			break;
#endif
#if defined(FBL100_NRF24L01_INO)
		case MODE_FBL100:
			next_callback=fbl_setup();
			remote_callback = ne260_cb;
			break;
#endif

#if defined(FLYSKY_A7105_INO)
    case MODE_FLYSKY:
      CTRL1_off;  //antenna RF1
      next_callback = initFlySky();
      remote_callback = ReadFlySky;
      break;
#endif
#if defined(HUBSAN_A7105_INO)
    case MODE_HUBSAN:
      CTRL1_off;  //antenna RF1
      if(IS_BIND_BUTTON_FLAG_on) random_id(10,true); // Generate new ID if bind button is pressed.
      next_callback = initHubsan();
      remote_callback = ReadHubsan;
      break;
#endif
#if defined(FRSKY_CC2500_INO)
    case MODE_FRSKY:
      CTRL1_off;  //antenna RF2
      CTRL2_on;
      next_callback = initFrSky_2way();
      remote_callback = ReadFrSky_2way;
      break;
#endif
#if defined(FRSKYX_CC2500_INO)
    case MODE_FRSKYX:
      CTRL1_off;  //antenna RF2
      CTRL2_on;
      next_callback = initFrSkyX();
      remote_callback = ReadFrSkyX;
      break;
#endif
#if defined(DSM2_CYRF6936_INO)
    case MODE_DSM2:
      CTRL2_on; //antenna RF4
      next_callback = initDsm2();
      //Servo_data[2]=1500;//before binding
      remote_callback = ReadDsm2;
      break;
#endif
#if defined(DEVO_CYRF6936_INO)
    case MODE_DEVO:
      CTRL2_on; //antenna RF4
      next_callback = DevoInit();
      remote_callback = devo_callback;
      break;
#endif
#if defined(HISKY_NRF24L01_INO)
    case MODE_HISKY:
      next_callback=initHiSky();
      remote_callback = hisky_cb;
      break;
#endif
#if defined(V2X2_NRF24L01_INO)
    case MODE_V2X2:
      next_callback = initV2x2();
      remote_callback = ReadV2x2;
      break;
#endif
#if defined(YD717_NRF24L01_INO)
    case MODE_YD717:
      next_callback=initYD717();
      remote_callback = yd717_callback;
      break;
#endif
#if defined(KN_NRF24L01_INO)
    case MODE_KN:
      next_callback = initKN();
      remote_callback = kn_callback;
      break;
#endif
#if defined(SYMAX_NRF24L01_INO)
    case MODE_SYMAX:
      next_callback = initSymax();
      remote_callback = symax_callback;
      break;
#endif
#if defined(SLT_NRF24L01_INO)
    case MODE_SLT:
      next_callback=initSLT();
      remote_callback = SLT_callback;
      break;
#endif
#if defined(CX10_NRF24L01_INO)
    case MODE_CX10:
      next_callback=initCX10();
      remote_callback = CX10_callback;
      break;
#endif
#if defined(CG023_NRF24L01_INO)
    case MODE_CG023:
      next_callback=initCG023();
      remote_callback = CG023_callback;
      break;
#endif
#if defined(BAYANG_NRF24L01_INO)
    case MODE_BAYANG:
      next_callback=initBAYANG();
      remote_callback = BAYANG_callback;
      break;
#endif
#if defined(ESKY_NRF24L01_INO)
    case MODE_ESKY:
      next_callback=initESKY();
      remote_callback = ESKY_callback;
      break;
#endif
#if defined(MT99XX_NRF24L01_INO)
		case MODE_MT99XX:
			next_callback=initMT99XX();
			remote_callback = MT99XX_callback;
      break;
#endif
#if defined(MJXQ_NRF24L01_INO)
		case MODE_MJXQ:
			next_callback=initMJXQ();
			remote_callback = MJXQ_callback;
			break;
#endif
#if defined(SHENQI_NRF24L01_INO)
		case MODE_SHENQI:
			next_callback=initSHENQI();
			remote_callback = SHENQI_callback;
			break;
#endif
#if defined(FY326_NRF24L01_INO)
		case MODE_FY326:
			next_callback=initFY326();
			remote_callback = FY326_callback;
			break;
#endif
  }
=======
	PE1_on;						//NRF24L01 antenna RF3 by default
	PE2_off;						//NRF24L01 antenna RF3 by default
	
	switch(protocol)				// Init the requested protocol
	{
		#ifdef A7105_INSTALLED
			#if defined(FLYSKY_A7105_INO)
				case MODE_FLYSKY:
					PE1_off;	//antenna RF1
					next_callback = initFlySky();
					remote_callback = ReadFlySky;
					break;
			#endif
			#if defined(HUBSAN_A7105_INO)
				case MODE_HUBSAN:
					PE1_off;	//antenna RF1
					if(IS_BIND_BUTTON_FLAG_on) random_id(10,true); // Generate new ID if bind button is pressed.
					next_callback = initHubsan();
					remote_callback = ReadHubsan;
					break;
			#endif
		#endif
		#ifdef CC2500_INSTALLED
			#if defined(FRSKYD_CC2500_INO)
				case MODE_FRSKYD:
					PE1_off;	//antenna RF2
					PE2_on;
					next_callback = initFrSky_2way();
					remote_callback = ReadFrSky_2way;
					break;
			#endif
			#if defined(FRSKYV_CC2500_INO)
				case MODE_FRSKYV:
					PE1_off;	//antenna RF2
					PE2_on;
					next_callback = initFRSKYV();
					remote_callback = ReadFRSKYV;
					break;
			#endif
			#if defined(FRSKYX_CC2500_INO)
				case MODE_FRSKYX:
					PE1_off;	//antenna RF2
					PE2_on;
					next_callback = initFrSkyX();
					remote_callback = ReadFrSkyX;
					break;
			#endif
			#if defined(SFHSS_CC2500_INO)
				case MODE_SFHSS:
					PE1_off;	//antenna RF2
					PE2_on;
					next_callback = initSFHSS();
					remote_callback = ReadSFHSS;
					break;
			#endif
		#endif
		#ifdef CYRF6936_INSTALLED
			#if defined(DSM_CYRF6936_INO)
				case MODE_DSM:
					PE2_on;	//antenna RF4
					next_callback = initDsm();
					//Servo_data[2]=1500;//before binding
					remote_callback = ReadDsm;
					break;
			#endif
			#if defined(DEVO_CYRF6936_INO)
				case MODE_DEVO:
					#ifdef ENABLE_PPM
						if(mode_select) //PPM mode
						{
							if(IS_BIND_BUTTON_FLAG_on)
							{
								eeprom_write_byte((uint8_t*)(30+mode_select),0x00);		// reset to autobind mode for the current model
								option=0;
							}
							else
							{	
								option=eeprom_read_byte((uint8_t*)(30+mode_select));	// load previous mode: autobind or fixed id
								if(option!=1) option=0;									// if not fixed id mode then it should be autobind
							}
						}
					#endif //ENABLE_PPM
					PE2_on;	//antenna RF4
					next_callback = DevoInit();
					remote_callback = devo_callback;
					break;
			#endif
			#if defined(J6PRO_CYRF6936_INO)
				case MODE_J6PRO:
					PE2_on;	//antenna RF4
					next_callback = initJ6Pro();
					remote_callback = ReadJ6Pro;
					break;
			#endif
		#endif
		#ifdef NRF24L01_INSTALLED
			#if defined(HISKY_NRF24L01_INO)
				case MODE_HISKY:
					next_callback=initHiSky();
					remote_callback = hisky_cb;
					break;
			#endif
			#if defined(V2X2_NRF24L01_INO)
				case MODE_V2X2:
					next_callback = initV2x2();
					remote_callback = ReadV2x2;
					break;
			#endif
			#if defined(YD717_NRF24L01_INO)
				case MODE_YD717:
					next_callback=initYD717();
					remote_callback = yd717_callback;
					break;
			#endif
			#if defined(KN_NRF24L01_INO)
				case MODE_KN:
					next_callback = initKN();
					remote_callback = kn_callback;
					break;
			#endif
			#if defined(SYMAX_NRF24L01_INO)
				case MODE_SYMAX:
					next_callback = initSymax();
					remote_callback = symax_callback;
					break;
			#endif
			#if defined(SLT_NRF24L01_INO)
				case MODE_SLT:
					next_callback=initSLT();
					remote_callback = SLT_callback;
					break;
			#endif
			#if defined(CX10_NRF24L01_INO)
				case MODE_CX10:
					next_callback=initCX10();
					remote_callback = CX10_callback;
					break;
			#endif
			#if defined(CG023_NRF24L01_INO)
				case MODE_CG023:
					next_callback=initCG023();
					remote_callback = CG023_callback;
					break;
			#endif
			#if defined(BAYANG_NRF24L01_INO)
				case MODE_BAYANG:
					next_callback=initBAYANG();
					remote_callback = BAYANG_callback;
					break;
			#endif
			#if defined(ESKY_NRF24L01_INO)
				case MODE_ESKY:
					next_callback=initESKY();
					remote_callback = ESKY_callback;
					break;
			#endif
			#if defined(MT99XX_NRF24L01_INO)
				case MODE_MT99XX:
					next_callback=initMT99XX();
					remote_callback = MT99XX_callback;
					break;
			#endif
			#if defined(MJXQ_NRF24L01_INO)
				case MODE_MJXQ:
					next_callback=initMJXQ();
					remote_callback = MJXQ_callback;
					break;
			#endif
			#if defined(SHENQI_NRF24L01_INO)
				case MODE_SHENQI:
					next_callback=initSHENQI();
					remote_callback = SHENQI_callback;
					break;
			#endif
			#if defined(FY326_NRF24L01_INO)
				case MODE_FY326:
					next_callback=initFY326();
					remote_callback = FY326_callback;
					break;
			#endif
			#if defined(FQ777_NRF24L01_INO)
				case MODE_FQ777:
					next_callback=initFQ777();
					remote_callback = FQ777_callback;
					break;
			#endif
			#if defined(ASSAN_NRF24L01_INO)
				case MODE_ASSAN:
					next_callback=initASSAN();
					remote_callback = ASSAN_callback;
					break;
			#endif
			#if defined(HONTAI_NRF24L01_INO)
				case MODE_HONTAI:
					next_callback=initHONTAI();
					remote_callback = HONTAI_callback;
					break;
			#endif
		#endif
	}
>>>>>>> refs/remotes/pascallanger/master

	if(next_callback>32000)
	{ // next_callback should not be more than 32767 so we will wait here...
		uint16_t temp=(next_callback>>10)-2;
		delayMilliseconds(temp);
		next_callback-=temp<<10;				// between 2-3ms left at this stage
	}
<<<<<<< HEAD
	cli();              // disable global int
	OCR1A=TCNT1+next_callback*2;  // set compare A for callback
	sei();              // enable global int
	TIFR1=(1<<OCF1A);       // clear compare A flag
	BIND_BUTTON_FLAG_off;     // do not bind/reset id anymore even if protocol change
=======
	cli();										// disable global int
	OCR1A = TCNT1 + next_callback*2;			// set compare A for callback
	sei();										// enable global int
	TIFR1 = OCF1A_bm ;							// clear compare A flag
	BIND_BUTTON_FLAG_off;						// do not bind/reset id anymore even if protocol change
>>>>>>> refs/remotes/pascallanger/master
}

void update_serial_data()
{
<<<<<<< HEAD
  if(rx_ok_buff[0]&0x20)            //check range
    RANGE_FLAG_on;
  else
    RANGE_FLAG_off;   
  if(rx_ok_buff[0]&0xC0)            //check autobind(0x40) & bind(0x80) together
    AUTOBIND_FLAG_on;
  else
    AUTOBIND_FLAG_off;
  if(rx_ok_buff[1]&0x80)            //if rx_ok_buff[1] ==1,power is low ,0-power high
    POWER_FLAG_off; //power low
  else
    POWER_FLAG_on;  //power high
        
  option=rx_ok_buff[2];
=======
	RX_DONOTUPDTAE_on;
	RX_FLAG_off;								//data is being processed
	if(rx_ok_buff[1]&0x20)						//check range
		RANGE_FLAG_on;
	else
		RANGE_FLAG_off;
	if(rx_ok_buff[1]&0xC0)						//check autobind(0x40) & bind(0x80) together
		AUTOBIND_FLAG_on;
	else
		AUTOBIND_FLAG_off;
	if(rx_ok_buff[2]&0x80)						//if rx_ok_buff[2] ==1,power is low ,0-power high
		POWER_FLAG_off;							//power low
	else
		POWER_FLAG_on;							//power high
>>>>>>> refs/remotes/pascallanger/master

	option=rx_ok_buff[3];

	if( (rx_ok_buff[0] != cur_protocol[0]) || ((rx_ok_buff[1]&0x5F) != (cur_protocol[1]&0x5F)) || ( (rx_ok_buff[2]&0x7F) != (cur_protocol[2]&0x7F) ) )
	{ // New model has been selected
<<<<<<< HEAD
    prev_protocol=cur_protocol[0]&0x1F;   //store previous protocol so we can reset the module
    cur_protocol[1] = rx_ok_buff[1]&0x7F; //store current protocol
    CHANGE_PROTOCOL_FLAG_on;        //change protocol
    sub_protocol=(rx_ok_buff[1]>>4)& 0x07;          //subprotocol no (0-7) bits 4-6
		RX_num=rx_ok_buff[1]& 0x0F;
		MProtocol_id=MProtocol_id_master+RX_num;	//personalized RX bind + rx num // rx_num bits 0---3
  }
	else
	  if( ((rx_ok_buff[0]&0x80)!=0) && ((cur_protocol[0]&0x80)==0) )	// Bind flag has been set
		CHANGE_PROTOCOL_FLAG_on;      //restart protocol with bind
  cur_protocol[0] = rx_ok_buff[0];      //store current protocol

// decode channel values
  volatile uint8_t *p=rx_ok_buff+2;
  uint8_t dec=-3;
=======
		CHANGE_PROTOCOL_FLAG_on;				//change protocol
		protocol=(rx_ok_buff[0]==0x55?0:32) + (rx_ok_buff[1]&0x1F);	//protocol no (0-63) bits 4-6 of buff[1] and bit 0 of buf[0]
		sub_protocol=(rx_ok_buff[2]>>4)& 0x07;	//subprotocol no (0-7) bits 4-6
		RX_num=rx_ok_buff[2]& 0x0F;				// rx_num bits 0---3
		MProtocol_id=MProtocol_id_master+RX_num;//personalized RX bind + rx num
		set_rx_tx_addr(MProtocol_id);			//set rx_tx_addr
	}
	else
		if( ((rx_ok_buff[1]&0x80)!=0) && ((cur_protocol[1]&0x80)==0) )	// Bind flag has been set
			CHANGE_PROTOCOL_FLAG_on;			//restart protocol with bind
		else
			CHANGE_PROTOCOL_FLAG_off;			//no need to restart
	//store current protocol values
	for(uint8_t i=0;i<3;i++)
		cur_protocol[i] =  rx_ok_buff[i];
	
	// decode channel values
	volatile uint8_t *p=rx_ok_buff+3;
	uint8_t dec=-3;
>>>>>>> refs/remotes/pascallanger/master
	for(uint8_t i=0;i<NUM_CHN;i++)
	{
    dec+=3;
		if(dec>=8)
		{
<<<<<<< HEAD
      dec-=8;
      p++;
    }
    p++;
    Servo_data[i]=((((*((uint32_t *)p))>>dec)&0x7FF)*5)/8+860;  //value range 860<->2140 -125%<->+125%
  }
  RX_FLAG_off;                //data has been processed
}

static void module_reset()
{
	if(remote_callback)
	{		// previous protocol loaded
		remote_callback = 0;
		switch(prev_protocol)
		{
			case MODE_FLYSKY:
			case MODE_HUBSAN:
				A7105_Reset();
				break;
			case MODE_FRSKY:
			case MODE_FRSKYX:
				CC2500_Reset();
				break;
			case MODE_DSM2:
			case MODE_DEVO:
				CYRF_Reset();
				break;
			default:	// MODE_HISKY, MODE_V2X2, MODE_YD717, MODE_KN, MODE_SYMAX, MODE_SLT, MODE_CX10, MODE_CG023, MODE_BAYANG, MODE_ESKY, MODE_MT99XX, MODE_MJXQ, MODE_SHENQI, MODE_FY326
				NRF24L01_Reset();
				break;
		}
=======
			dec-=8;
			p++;
		}
		p++;
		Servo_data[i]=((((*((uint32_t *)p))>>dec)&0x7FF)*5)/8+860;	//value range 860<->2140 -125%<->+125%
	}
	RX_DONOTUPDTAE_off;
	#ifdef XMEGA
		cli();
	#else
		UCSR0B &= ~_BV(RXCIE0);					// RX interrupt disable
	#endif
	if(IS_RX_MISSED_BUFF_on)					// If the buffer is still valid
	{	memcpy((void*)rx_ok_buff,(const void*)rx_buff,RXBUFFER_SIZE);// Duplicate the buffer
		RX_FLAG_on;								// data to be processed next time...
		RX_MISSED_BUFF_off;
>>>>>>> refs/remotes/pascallanger/master
	}
	#ifdef XMEGA
		sei();
	#else
		UCSR0B |= _BV(RXCIE0) ;					// RX interrupt enable
	#endif
}

void modules_reset()
{
	#ifdef	CC2500_INSTALLED
		CC2500_Reset();
	#endif
	#ifdef	A7105_INSTALLED
		A7105_Reset();
	#endif
	#ifdef	CYRF6936_INSTALLED
		CYRF_Reset();
	#endif
	#ifdef	NRF24L01_INSTALLED
		NRF24L01_Reset();
	#endif

	//Wait for every component to reset
	delayMilliseconds(100);
	prev_power=0xFD;		// unused power value
}

void Mprotocol_serial_init()
{
	#ifdef XMEGA
		PORTC.OUTSET = 0x08 ;
		PORTC.DIRSET = 0x08 ;

		USARTC0.BAUDCTRLA = 19 ;
		USARTC0.BAUDCTRLB = 0 ;
		
		USARTC0.CTRLB = 0x18 ;
		USARTC0.CTRLA = (USARTC0.CTRLA & 0xCF) | 0x10 ;
		USARTC0.CTRLC = 0x2B ;
		UDR0 ;
		#ifdef INVERT_TELEMETRY
			PORTC.PIN3CTRL |= 0x40 ;
		#endif
	#else
		#include <util/setbaud.h>	
		UBRR0H = UBRRH_VALUE;
		UBRR0L = UBRRL_VALUE;
		UCSR0A = 0 ;	// Clear X2 bit
		//Set frame format to 8 data bits, even parity, 2 stop bits
		UCSR0C = _BV(UPM01)|_BV(USBS0)|_BV(UCSZ01)|_BV(UCSZ00);
		while ( UCSR0A & (1 << RXC0) )//flush receive buffer
			UDR0;
		//enable reception and RC complete interrupt
		UCSR0B = _BV(RXEN0)|_BV(RXCIE0);//rx enable and interrupt
		#ifndef DEBUG_TX
			#if defined(TELEMETRY)
				initTXSerial( SPEED_100K ) ;
			#endif //TELEMETRY
		#endif //DEBUG_TX
	#endif //XMEGA
}

#if defined(TELEMETRY)
void PPM_Telemetry_serial_init()
{
<<<<<<< HEAD
	if(Servo_data[ch]>PPM_MAX_100)
		return PPM_MAX_100;
	else
	if (Servo_data[ch]<PPM_MIN_100)
		return PPM_MIN_100;
	return Servo_data[ch];
=======
	if( (protocol==MODE_FRSKYD) || (protocol==MODE_HUBSAN))
		initTXSerial( SPEED_9600 ) ;
	if(protocol==MODE_FRSKYX)
		initTXSerial( SPEED_57600 ) ;
	if(protocol==MODE_DSM)
		initTXSerial( SPEED_125K ) ;
>>>>>>> refs/remotes/pascallanger/master
}
#endif

<<<<<<< HEAD
#if defined(TELEMETRY)
void Serial_write(uint8_t data)
{
	cli();	// disable global int
	if(++tx_head>=TXBUFFER_SIZE)
		tx_head=0;
	tx_buff[tx_head]=data;
	sei();	// enable global int
	UCSR0B |= (1<<UDRIE0);//enable UDRE interrupt
=======
// Convert 32b id to rx_tx_addr
static void set_rx_tx_addr(uint32_t id)
{ // Used by almost all protocols
	rx_tx_addr[0] = (id >> 24) & 0xFF;
	rx_tx_addr[1] = (id >> 16) & 0xFF;
	rx_tx_addr[2] = (id >>  8) & 0xFF;
	rx_tx_addr[3] = (id >>  0) & 0xFF;
	rx_tx_addr[4] = (rx_tx_addr[2]&0xF0)|(rx_tx_addr[3]&0x0F);
}

#ifndef XMEGA
static void random_init(void)
{
	cli();					// Temporarily turn off interrupts, until WDT configured
	MCUSR = 0;				// Use the MCU status register to reset flags for WDR, BOR, EXTR, and POWR
	WDTCSR |= _BV(WDCE);	// WDT control register, This sets the Watchdog Change Enable (WDCE) flag, which is  needed to set the prescaler
	WDTCSR = _BV(WDIE);		// Watchdog interrupt enable (WDIE)
	sei();					// Turn interupts on
>>>>>>> refs/remotes/pascallanger/master
}

static uint32_t random_value(void)
{
<<<<<<< HEAD
	#include <util/setbaud.h> 
	UBRR0H = UBRRH_VALUE;
	UBRR0L = UBRRL_VALUE;
	UCSR0A = 0 ;	// Clear X2 bit
	//Set frame format to 8 data bits, even parity, 2 stop bits
	UCSR0C = (1<<UPM01)|(1<<USBS0)|(1<<UCSZ01)|(1<<UCSZ00);
	while ( UCSR0A & (1 << RXC0) )//flush receive buffer
		UDR0;
	//enable reception and RC complete interrupt
	UCSR0B = (1<<RXEN0)|(1<<RXCIE0);//rx enable and interrupt
	UCSR0B |= (1<<TXEN0);//tx enable
=======
	while (!gWDT_entropy);
	return gWDT_entropy;
>>>>>>> refs/remotes/pascallanger/master
}
#endif

#if defined(TELEMETRY)
static void PPM_Telemetry_serial_init()
{
	//9600 bauds
	UBRR0H = 0x00;
	UBRR0L = 0x67;
	UCSR0A = 0 ;	// Clear X2 bit
	//Set frame format to 8 data bits, none, 1 stop bit
	UCSR0C = (1<<UCSZ01)|(1<<UCSZ00);
	UCSR0B = (1<<TXEN0);//tx enable
}
#endif

// Convert 32b id to rx_tx_addr
static void set_rx_tx_addr(uint32_t id)
{ // Used by almost all protocols
	rx_tx_addr[0] = (id >> 24) & 0xFF;
	rx_tx_addr[1] = (id >> 16) & 0xFF;
	rx_tx_addr[2] = (id >>  8) & 0xFF;
	rx_tx_addr[3] = (id >>  0) & 0xFF;
	rx_tx_addr[4] = 0xC1;	// for YD717: always uses first data port
}

static uint32_t random_id(uint16_t adress, uint8_t create_new)
{
	uint32_t id;
	uint8_t txid[4];

	if(eeprom_read_byte((uint8_t*)(adress+10))==0xf0 && !create_new)
	{  // TXID exists in EEPROM
		eeprom_read_block((void*)txid,(const void*)adress,4);
		id=(txid[0] | ((uint32_t)txid[1]<<8) | ((uint32_t)txid[2]<<16) | ((uint32_t)txid[3]<<24));
		if(id!=0x2AD141A7)	//ID with seed=0
			return id;
	}
	// Generate a random ID
	id = random(0xfefefefe) + ((uint32_t)random(0xfefefefe) << 16);
	txid[0]=  (id &0xFF);
	txid[1] = ((id >> 8) & 0xFF);
	txid[2] = ((id >> 16) & 0xFF);
	txid[3] = ((id >> 24) & 0xFF);
	eeprom_write_block((const void*)txid,(void*)adress,4);
	eeprom_write_byte((uint8_t*)(adress+10),0xf0);//write bind flag in eeprom.
	return id;
}

/**************************/
/**************************/
/**  Interrupt routines  **/
/**************************/
/**************************/

//PPM
<<<<<<< HEAD
ISR(INT1_vect)
{	// Interrupt on PPM pin
	static int8_t chan=-1;
	static uint16_t Prev_TCNT1=0;
	uint16_t Cur_TCNT1;
=======
#ifdef ENABLE_PPM
	#ifdef XMEGA
		#if PPM_pin == 2
			ISR(PORTD_INT0_vect)
		#else
			ISR(PORTD_INT1_vect)
		#endif
	#else
		#if PPM_pin == 2
			ISR(INT0_vect, ISR_NOBLOCK)
		#else
			ISR(INT1_vect, ISR_NOBLOCK)
		#endif
	#endif
	{	// Interrupt on PPM pin
		static int8_t chan=-1;
		static uint16_t Prev_TCNT1=0;
		uint16_t Cur_TCNT1;
>>>>>>> refs/remotes/pascallanger/master

		Cur_TCNT1 = TCNT1 - Prev_TCNT1 ;	// Capture current Timer1 value
		if(Cur_TCNT1<1000)
			chan=-1;						// bad frame
		else
			if(Cur_TCNT1>4840)
			{
				chan=0;						// start of frame
				PPM_FLAG_on;				// full frame present (even at startup since PPM_data has been initialized)
			}
<<<<<<< HEAD
	Prev_TCNT1+=Cur_TCNT1;
}

//Serial RX
ISR(USART_RX_vect)
{	// RX interrupt
	if((UCSR0A&0x1C)==0)			// Check frame error, data overrun and parity error
	{ // received byte is ok to process
		if(idx==0)
		{	// Let's try to sync at this point
			if(UDR0==0x55)			// If 1st byte is 0x55 it looks ok
=======
			else
				if(chan!=-1)				// need to wait for start of frame
				{  //servo values between 500us and 2420us will end up here
					PPM_data[chan]= Cur_TCNT1>>1;;
					if(chan++>=NUM_CHN)
						chan=-1;			// don't accept any new channels
				}
		Prev_TCNT1+=Cur_TCNT1;
	}
#endif //ENABLE_PPM

//Serial RX
#ifdef ENABLE_SERIAL
	#ifdef XMEGA
		ISR(USARTC0_RXC_vect)
	#else
		ISR(USART_RX_vect)
	#endif
	{	// RX interrupt
		static uint8_t idx=0;
		#ifdef XMEGA
			if((USARTC0.STATUS & 0x1C)==0)		// Check frame error, data overrun and parity error
		#else
			UCSR0B &= ~_BV(RXCIE0) ;			// RX interrupt disable
			sei() ;
			if((UCSR0A&0x1C)==0)				// Check frame error, data overrun and parity error
		#endif
		{ // received byte is ok to process
			if(idx==0||discard_frame==1)
			{	// Let's try to sync at this point
				idx=0;discard_frame=0;
				RX_MISSED_BUFF_off;			// If rx_buff was good it's not anymore...
				rx_buff[0]=UDR0;
				if((rx_buff[0]&0xFE)==0x54)	// If 1st byte is 0x54 or 0x55 it looks ok
				{
					TX_RX_PAUSE_on;
					tx_pause();
					OCR1B = TCNT1+(6500L) ;	// Full message should be received within timer of 3250us
					TIFR1 = OCF1B_bm ;		// clear OCR1B match flag
					SET_TIMSK1_OCIE1B ;		// enable interrupt on compare B match
					idx++;
				}
			}
			else
>>>>>>> refs/remotes/pascallanger/master
			{
				rx_buff[idx++]=UDR0;		// Store received byte
				if(idx>=RXBUFFER_SIZE)
				{	// A full frame has been received
					if(!IS_RX_DONOTUPDTAE_on)
					{ //Good frame received and main is not working on the buffer
						memcpy((void*)rx_ok_buff,(const void*)rx_buff,RXBUFFER_SIZE);// Duplicate the buffer
						RX_FLAG_on;			// flag for main to process servo data
					}
					else
						RX_MISSED_BUFF_on;	// notify that rx_buff is good
					discard_frame=1; 		// start again
				}
			}
		}
		else
		{
			idx=UDR0;						// Dummy read
			discard_frame=1;				// Error encountered discard full frame...
		}
		if(discard_frame==1)
		{
			CLR_TIMSK1_OCIE1B;				// Disable interrupt on compare B match
			TX_RX_PAUSE_off;
			tx_resume();
		}
		#ifndef XMEGA
			cli() ;
			UCSR0B |= _BV(RXCIE0) ;			// RX interrupt enable
		#endif
	}

	//Serial timer
	#ifdef XMEGA
		ISR(TCC1_CCB_vect)
	#else
		ISR(TIMER1_COMPB_vect, ISR_NOBLOCK )
	#endif
	{	// Timer1 compare B interrupt
		discard_frame=1;
		CLR_TIMSK1_OCIE1B;					// Disable interrupt on compare B match
		tx_resume();
	}
#endif //ENABLE_SERIAL

#ifndef XMEGA
	// Random interrupt service routine called every time the WDT interrupt is triggered.
	// It is only enabled at startup to generate a seed.
	ISR(WDT_vect)
	{
		static uint8_t gWDT_buffer_position=0;
		#define gWDT_buffer_SIZE 32
		static uint8_t gWDT_buffer[gWDT_buffer_SIZE];
		gWDT_buffer[gWDT_buffer_position] = TCNT1L; // Record the Timer 1 low byte (only one needed) 
		gWDT_buffer_position++;                     // every time the WDT interrupt is triggered
		if (gWDT_buffer_position >= gWDT_buffer_SIZE)
		{
			// The following code is an implementation of Jenkin's one at a time hash
			for(uint8_t gWDT_loop_counter = 0; gWDT_loop_counter < gWDT_buffer_SIZE; ++gWDT_loop_counter)
			{
				gWDT_entropy += gWDT_buffer[gWDT_loop_counter];
				gWDT_entropy += (gWDT_entropy << 10);
				gWDT_entropy ^= (gWDT_entropy >> 6);
			}
			gWDT_entropy += (gWDT_entropy << 3);
			gWDT_entropy ^= (gWDT_entropy >> 11);
			gWDT_entropy += (gWDT_entropy << 15);
			WDTCSR = 0;	// Disable Watchdog interrupt
		}
	}
<<<<<<< HEAD
}

//Serial timer
ISR(TIMER1_COMPB_vect)
{	// Timer1 compare B interrupt
	idx=0;
}

#if defined(TELEMETRY)
//Serial TX
ISR(USART_UDRE_vect)
{	// Transmit interrupt
	uint8_t t = tx_tail;
	if(tx_head!=t)
	{
		if(++t>=TXBUFFER_SIZE)//head 
		t=0;
		UDR0=tx_buff[t];
		tx_tail=t;
	}
	if (t == tx_head)
		UCSR0B &= ~(1<<UDRIE0); // Check if all data is transmitted . if yes disable transmitter UDRE interrupt
}
=======
>>>>>>> refs/remotes/pascallanger/master
#endif
