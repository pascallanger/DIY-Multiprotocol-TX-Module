/*********************************************************
					Multiprotocol Tx code
               by Midelic and Pascal Langer(hpnuts)
	http://www.rcgroups.com/forums/showthread.php?t=2165676
    https://github.com/pascallanger/DIY-Multiprotocol-TX-Module/edit/master/README.md

	Thanks to PhracturedBlue, Hexfet, Goebish, Victzh and all protocol developers
				Ported  from deviation firmware 

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
#include <avr/pgmspace.h>

//#define DEBUG_PIN		// Use pin TX for AVR and SPI_CS for STM32 => DEBUG_PIN_on, DEBUG_PIN_off, DEBUG_PIN_toggle
//#define DEBUG_SERIAL	// Only for STM32_BOARD, compiled with Upload method "Serial"->usart1, "STM32duino bootloader"->USB serial

#ifdef __arm__			// Let's automatically select the board if arm is selected
	#define STM32_BOARD
#endif
#if defined (ARDUINO_AVR_XMEGA32D4) || defined (ARDUINO_MULTI_ORANGERX)
	#include "MultiOrange.h"
#endif

#include "Multiprotocol.h"

//Multiprotocol module configuration file
#include "_Config.h"

//Personal config file
#if defined(USE_MY_CONFIG)
#include "_MyConfig.h"
#endif

#include "Pins.h"
#include "TX_Def.h"
#include "Validate.h"

#ifndef STM32_BOARD
	#include <avr/eeprom.h>
#else
	#include <libmaple/usart.h>
	#include <libmaple/timer.h>
	//#include <libmaple/spi.h>
	#include <SPI.h>
	#include <EEPROM.h>	
	HardwareTimer HWTimer2(2);
#if defined  SPORT_POLLING
#ifdef INVERT_TELEMETRY
	HardwareTimer HWTimer4(4);
#endif
#endif
	void PPM_decode();
	void ISR_COMPB();
	extern "C"
	{
		void __irq_usart2(void);
		void __irq_usart3(void);
	}
#endif

//Global constants/variables
uint32_t MProtocol_id;//tx id,
uint32_t MProtocol_id_master;
uint32_t blink=0,last_signal=0;
//
uint16_t counter;
uint8_t  channel;
uint8_t  packet[40];

#define NUM_CHN 16
// Servo data
uint16_t Channel_data[NUM_CHN];
uint8_t  Channel_AUX;
#ifdef FAILSAFE_ENABLE
	uint16_t Failsafe_data[NUM_CHN];
#endif

// Protocol variables
uint8_t  cyrfmfg_id[6];//for dsm2 and devo
uint8_t  rx_tx_addr[5];
uint8_t  rx_id[5];
uint8_t  phase;
uint16_t bind_counter;
uint8_t  bind_phase;
uint8_t  binding_idx;
uint16_t packet_period;
uint8_t  packet_count;
uint8_t  packet_sent;
uint8_t  packet_length;
uint8_t  hopping_frequency[50];
uint8_t  *hopping_frequency_ptr;
uint8_t  hopping_frequency_no=0;
uint8_t  rf_ch_num;
uint8_t  throttle, rudder, elevator, aileron;
uint8_t  flags;
uint16_t crc;
uint8_t  crc8;
uint16_t seed;
uint16_t failsafe_count;
uint16_t state;
uint8_t  len;
uint8_t  armed, arm_flags, arm_channel_previous;

#if defined(FRSKYX_CC2500_INO) || defined(SFHSS_CC2500_INO) || defined(HITEC_CC2500_INO)
	uint8_t calData[48];
#endif

#ifdef CHECK_FOR_BOOTLOADER
	uint8_t BootTimer ;
	uint8_t BootState ;
	uint8_t NotBootChecking ;
	uint8_t BootCount ;

	#define BOOT_WAIT_30_IDLE	0
	#define BOOT_WAIT_30_DATA	1
	#define BOOT_WAIT_20		2
	#define BOOT_READY			3
#endif

//Channel mapping for protocols
const uint8_t CH_AETR[]={AILERON, ELEVATOR, THROTTLE, RUDDER, CH5, CH6, CH7, CH8, CH9, CH10, CH11, CH12, CH13, CH14, CH15, CH16};
const uint8_t CH_TAER[]={THROTTLE, AILERON, ELEVATOR, RUDDER, CH5, CH6, CH7, CH8, CH9, CH10, CH11, CH12, CH13, CH14, CH15, CH16};
const uint8_t CH_RETA[]={RUDDER, ELEVATOR, THROTTLE, AILERON, CH5, CH6, CH7, CH8, CH9, CH10, CH11, CH12, CH13, CH14, CH15, CH16};
const uint8_t CH_EATR[]={ELEVATOR, AILERON, THROTTLE, RUDDER, CH5, CH6, CH7, CH8, CH9, CH10, CH11, CH12, CH13, CH14, CH15, CH16};

// Mode_select variables
uint8_t mode_select;
uint8_t protocol_flags=0,protocol_flags2=0;

#ifdef ENABLE_PPM
// PPM variable
volatile uint16_t PPM_data[NUM_CHN];
volatile uint8_t  PPM_chan_max=0;
#endif

#if not defined (ORANGE_TX) && not defined (STM32_BOARD)
//Random variable
volatile uint32_t gWDT_entropy=0;
#endif

//Serial protocol
uint8_t sub_protocol;
uint8_t protocol;
uint8_t option;
uint8_t cur_protocol[3];
uint8_t prev_option;
uint8_t prev_power=0xFD; // unused power value
uint8_t  RX_num;

//Serial RX variables
#define BAUD 100000
#define RXBUFFER_SIZE 26
volatile uint8_t rx_buff[RXBUFFER_SIZE];
volatile uint8_t rx_ok_buff[RXBUFFER_SIZE];
volatile uint8_t discard_frame = 0;

// Telemetry
#define MAX_PKT 29
uint8_t pkt[MAX_PKT];//telemetry receiving packets
#if defined(TELEMETRY)
	#ifdef INVERT_TELEMETRY
		#if not defined(ORANGE_TX) && not defined(STM32_BOARD)
			// enable bit bash for serial
			#define	BASH_SERIAL 1
		#endif
		#define	INVERT_SERIAL 1
	#endif
	uint8_t pass = 0;
	uint8_t pktt[MAX_PKT];//telemetry receiving packets
	#ifdef BASH_SERIAL
	// For bit-bashed serial output
		#define TXBUFFER_SIZE 192
		volatile struct t_serial_bash
		{
			uint8_t head ;
			uint8_t tail ;
			uint8_t data[TXBUFFER_SIZE] ;
			uint8_t busy ;
			uint8_t speed ;
		} SerialControl ;
	#else
		#define TXBUFFER_SIZE 96
		volatile uint8_t tx_buff[TXBUFFER_SIZE];
		volatile uint8_t tx_head=0;
		volatile uint8_t tx_tail=0;
	#endif // BASH_SERIAL
	uint8_t v_lipo1;
	uint8_t v_lipo2;
	uint8_t RX_RSSI;
	uint8_t TX_RSSI;
	uint8_t RX_LQI;
	uint8_t TX_LQI;
	uint8_t telemetry_link=0; 
	uint8_t telemetry_counter=0;
	uint8_t telemetry_lost;
	#ifdef SPORT_POLLING
		#define MAX_SPORT_BUFFER 64
		uint8_t	SportData[MAX_SPORT_BUFFER];
		bool	ok_to_send = false;
		uint8_t	sport_idx = 0;
		uint8_t	sport_index = 0;
	#endif
#endif // TELEMETRY

// Callback
typedef uint16_t (*void_function_t) (void);//pointer to a function with no parameters which return an uint16_t integer
void_function_t remote_callback = 0;

// Init
void setup()
{
	// Setup diagnostic uart before anything else
	#ifdef DEBUG_SERIAL
		Serial.begin(115200,SERIAL_8N1);
		while (!Serial); // Wait for ever for the serial port to connect...
		debugln("Multiprotocol version: %d.%d.%d.%d", VERSION_MAJOR, VERSION_MINOR, VERSION_REVISION, VERSION_PATCH_LEVEL);
	#endif

	// General pinout
	#ifdef ORANGE_TX
		//XMEGA
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
	#elif defined STM32_BOARD
		//STM32
		afio_cfg_debug_ports(AFIO_DEBUG_NONE);
		pinMode(LED2_pin,OUTPUT);
		pinMode(A7105_CSN_pin,OUTPUT);
		pinMode(CC25_CSN_pin,OUTPUT);
		pinMode(NRF_CSN_pin,OUTPUT);
		pinMode(CYRF_CSN_pin,OUTPUT);
		pinMode(SPI_CSN_pin,OUTPUT);
		pinMode(CYRF_RST_pin,OUTPUT);
		pinMode(PE1_pin,OUTPUT);
		pinMode(PE2_pin,OUTPUT);
		pinMode(TX_INV_pin,OUTPUT);
		pinMode(RX_INV_pin,OUTPUT);
		#if defined TELEMETRY
			#if defined INVERT_SERIAL
				TX_INV_on;	//activate inverter for both serial TX and RX signals
				RX_INV_on;
			#else
				TX_INV_off;
				RX_INV_off;
			#endif	
		#endif
		pinMode(BIND_pin,INPUT_PULLUP);
		pinMode(PPM_pin,INPUT);
		pinMode(S1_pin,INPUT_PULLUP);//dial switch
		pinMode(S2_pin,INPUT_PULLUP);
		pinMode(S3_pin,INPUT_PULLUP);
		pinMode(S4_pin,INPUT_PULLUP);
		//Random pins
		pinMode(PB0, INPUT_ANALOG); // set up pin for analog input
		pinMode(PB1, INPUT_ANALOG); // set up pin for analog input

		//Timers
		init_HWTimer();			//0.5us
	#else
		//ATMEGA328p
		// all inputs
		DDRB=0x00;DDRC=0x00;DDRD=0x00;
		// outputs
		SDI_output;
		SCLK_output;
		#ifdef A7105_CSN_pin
			A7105_CSN_output;
		#endif
		#ifdef CC25_CSN_pin
			CC25_CSN_output;
		#endif
		#ifdef CYRF_CSN_pin
			CYRF_RST_output;
			CYRF_CSN_output;
		#endif
		#ifdef NRF_CSN_pin
			NRF_CSN_output;
		#endif
		PE1_output;
		PE2_output;
		SERIAL_TX_output;

		// pullups
		PROTO_DIAL1_port |= _BV(PROTO_DIAL1_pin);
		PROTO_DIAL2_port |= _BV(PROTO_DIAL2_pin);
		PROTO_DIAL3_port |= _BV(PROTO_DIAL3_pin);
		PROTO_DIAL4_port |= _BV(PROTO_DIAL4_pin);
		BIND_port |= _BV(BIND_pin);

		// Timer1 config
		TCCR1A = 0;
		TCCR1B = (1 << CS11);	//prescaler8, set timer1 to increment every 0.5us(16Mhz) and start timer

		// Random
		random_init();
	#endif

	LED2_on;
	
	// Set Chip selects
	#ifdef A7105_CSN_pin
		A7105_CSN_on;
	#endif
	#ifdef CC25_CSN_pin
		CC25_CSN_on;
	#endif
	#ifdef CYRF_CSN_pin
		CYRF_CSN_on;
	#endif
	#ifdef NRF_CSN_pin
		NRF_CSN_on;
	#endif
	//	Set SPI lines
	#ifdef	STM32_BOARD
		initSPI2();
	#else
		SDI_on;
		SCLK_off;
	#endif

	//Wait for every component to start
	delayMilliseconds(100);
	
	// Read status of bind button
	if( IS_BIND_BUTTON_on )
	{
		BIND_BUTTON_FLAG_on;	// If bind button pressed save the status
		BIND_IN_PROGRESS;		// Request bind
	}
	else
		BIND_DONE;

	// Read status of mode select binary switch
	// after this mode_select will be one of {0000, 0001, ..., 1111}
	#ifndef ENABLE_PPM
		mode_select = MODE_SERIAL ;	// force serial mode
	#elif defined STM32_BOARD
		mode_select= 0x0F -(uint8_t)(((GPIOA->regs->IDR)>>4)&0x0F);
	#else
		mode_select =
			((PROTO_DIAL1_ipr & _BV(PROTO_DIAL1_pin)) ? 0 : 1) + 
			((PROTO_DIAL2_ipr & _BV(PROTO_DIAL2_pin)) ? 0 : 2) +
			((PROTO_DIAL3_ipr & _BV(PROTO_DIAL3_pin)) ? 0 : 4) +
			((PROTO_DIAL4_ipr & _BV(PROTO_DIAL4_pin)) ? 0 : 8);
	#endif
	//mode_select=1;
    debugln("Protocol selection switch reads as %d", mode_select);

	#ifdef ENABLE_PPM
		uint8_t bank=bank_switch();
	#endif

	// Set default channels' value
	InitChannel();
	#ifdef ENABLE_PPM
		InitPPM();
	#endif

	// Update LED
	LED_off;
	LED_output;

	//Init RF modules
	modules_reset();

#ifndef ORANGE_TX
	//Init the seed with a random value created from watchdog timer for all protocols requiring random values
	#ifdef STM32_BOARD
		randomSeed((uint32_t)analogRead(PB0) << 10 | analogRead(PB1));			
	#else
		randomSeed(random_value());
	#endif
#endif

	// Read or create protocol id
	MProtocol_id_master=random_id(10,false);

	debugln("Module Id: %lx", MProtocol_id_master);
	
#ifdef ENABLE_PPM
	//Protocol and interrupts initialization
	if(mode_select != MODE_SERIAL)
	{ // PPM
		#ifndef MY_PPM_PROT
			const PPM_Parameters *PPM_prot_line=&PPM_prot[bank*14+mode_select-1];
		#else
			const PPM_Parameters *PPM_prot_line=&My_PPM_prot[bank*14+mode_select-1];
		#endif
		
		protocol		=	PPM_prot_line->protocol;
		cur_protocol[1] = protocol;
		sub_protocol   	=	PPM_prot_line->sub_proto;
		RX_num			=	PPM_prot_line->rx_num;

		//Forced frequency tuning values for CC2500 protocols
		#if defined(FORCE_FRSKYD_TUNING) && defined(FRSKYD_CC2500_INO)
			if(protocol==PROTO_FRSKYD) 
				option			=	FORCE_FRSKYD_TUNING;		// Use config-defined tuning value for FrSkyD
			else
		#endif
		#if defined(FORCE_FRSKYV_TUNING) && defined(FRSKYV_CC2500_INO)
			if(protocol==PROTO_FRSKYV)
				option			=	FORCE_FRSKYV_TUNING;		// Use config-defined tuning value for FrSkyV
			else
		#endif
		#if defined(FORCE_FRSKYX_TUNING) && defined(FRSKYX_CC2500_INO)
			if(protocol==PROTO_FRSKYX)
				option			=	FORCE_FRSKYX_TUNING;		// Use config-defined tuning value for FrSkyX
			else
		#endif 
		#if defined(FORCE_SFHSS_TUNING) && defined(SFHSS_CC2500_INO)
			if (protocol==PROTO_SFHSS)
				option			=	FORCE_SFHSS_TUNING;			// Use config-defined tuning value for SFHSS
			else
		#endif
		#if defined(FORCE_CORONA_TUNING) && defined(CORONA_CC2500_INO)
			if (protocol==PROTO_CORONA)
				option			=	FORCE_CORONA_TUNING;		// Use config-defined tuning value for CORONA
			else
		#endif
		#if defined(FORCE_HITEC_TUNING) && defined(HITEC_CC2500_INO)
			if (protocol==PROTO_HITEC)
				option			=	FORCE_HITEC_TUNING;		// Use config-defined tuning value for HITEC
			else
		#endif
				option			=	PPM_prot_line->option;	// Use radio-defined option value

		if(PPM_prot_line->power)		POWER_FLAG_on;
		if(PPM_prot_line->autobind)
		{
			AUTOBIND_FLAG_on;
			BIND_IN_PROGRESS;	// Force a bind at protocol startup
		}

		protocol_init();

		#ifndef STM32_BOARD
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
		#else
			attachInterrupt(PPM_pin,PPM_decode,FALLING);
		#endif

		#if defined(TELEMETRY)
			PPM_Telemetry_serial_init();// Configure serial for telemetry
		#endif
	}
	else
#endif //ENABLE_PPM
	{ // Serial
		#ifdef ENABLE_SERIAL
			for(uint8_t i=0;i<3;i++)
				cur_protocol[i]=0;
			protocol=0;
			#ifdef CHECK_FOR_BOOTLOADER
				Mprotocol_serial_init(1); 	// Configure serial and enable RX interrupt
			#else
				Mprotocol_serial_init(); 	// Configure serial and enable RX interrupt
			#endif
		#endif //ENABLE_SERIAL
	}
	LED2_on;
	debugln("Init complete");
}

// Main
// Protocol scheduler
void loop()
{ 
	uint16_t next_callback,diff=0xFFFF;

	while(1)
	{
		if(remote_callback==0 || IS_WAIT_BIND_on || diff>2*200)
		{
			do
			{
				Update_All();
			}
			while(remote_callback==0 || IS_WAIT_BIND_on);
		}
		#ifndef STM32_BOARD
			if( (TIFR1 & OCF1A_bm) != 0)
			{
				cli();					// Disable global int due to RW of 16 bits registers
				OCR1A=TCNT1;			// Callback should already have been called... Use "now" as new sync point.
				sei();					// Enable global int
			}
			else
				while((TIFR1 & OCF1A_bm) == 0); // Wait before callback
		#else
			if((TIMER2_BASE->SR & TIMER_SR_CC1IF)!=0)
			{
				debugln("Callback miss");
				cli();
				OCR1A = TCNT1;
				sei();
			}
			else
				while((TIMER2_BASE->SR & TIMER_SR_CC1IF )==0); // Wait before callback
		#endif
		do
		{
			TX_MAIN_PAUSE_on;
			tx_pause();
			if(IS_INPUT_SIGNAL_on && remote_callback!=0)
				next_callback=remote_callback();
			else
				next_callback=2000;					// No PPM/serial signal check again in 2ms...
			TX_MAIN_PAUSE_off;
			tx_resume();
			while(next_callback>4000)
			{ // start to wait here as much as we can...
				next_callback-=2000;				// We will wait below for 2ms
				cli();								// Disable global int due to RW of 16 bits registers
				OCR1A += 2000*2 ;					// set compare A for callback
				#ifndef STM32_BOARD	
					TIFR1=OCF1A_bm;					// clear compare A=callback flag
				#else
					TIMER2_BASE->SR = 0x1E5F & ~TIMER_SR_CC1IF;	// Clear Timer2/Comp1 interrupt flag
				#endif
				sei();								// enable global int
				if(Update_All())					// Protocol changed?
				{
					next_callback=0;				// Launch new protocol ASAP
					break;
				}
				#ifndef STM32_BOARD	
					while((TIFR1 & OCF1A_bm) == 0);	// wait 2ms...
				#else
					while((TIMER2_BASE->SR & TIMER_SR_CC1IF)==0);//2ms wait
				#endif
			}
			// at this point we have a maximum of 4ms in next_callback
			next_callback *= 2 ;
			cli();									// Disable global int due to RW of 16 bits registers
			OCR1A+= next_callback ;					// set compare A for callback
			#ifndef STM32_BOARD			
				TIFR1=OCF1A_bm;						// clear compare A=callback flag
			#else
				TIMER2_BASE->SR = 0x1E5F & ~TIMER_SR_CC1IF;	// Clear Timer2/Comp1 interrupt flag
			#endif		
			diff=OCR1A-TCNT1;						// compare timer and comparator
			sei();									// enable global int
		}
		while(diff&0x8000);	 						// Callback did not took more than requested time for next callback
													// so we can launch Update_All before next callback
	}
}

uint8_t Update_All()
{
	#ifdef ENABLE_SERIAL
		#ifdef CHECK_FOR_BOOTLOADER
			if ( (mode_select==MODE_SERIAL) && (NotBootChecking == 0) )
				pollBoot() ;
			else
		#endif
		if(mode_select==MODE_SERIAL && IS_RX_FLAG_on)		// Serial mode and something has been received
		{
			update_serial_data();							// Update protocol and data
			update_channels_aux();
			INPUT_SIGNAL_on;								//valid signal received
			last_signal=millis();
		}
	#endif //ENABLE_SERIAL
	#ifdef ENABLE_PPM
		if(mode_select!=MODE_SERIAL && IS_PPM_FLAG_on)		// PPM mode and a full frame has been received
		{
			for(uint8_t i=0;i<PPM_chan_max;i++)
			{ // update servo data without interrupts to prevent bad read
				uint16_t val;
				cli();										// disable global int
				val = PPM_data[i];
				sei();										// enable global int
				val=map16b(val,PPM_MIN_100*2,PPM_MAX_100*2,CHANNEL_MIN_100,CHANNEL_MAX_100);
				if(val&0x8000) 					val=CHANNEL_MIN_125;
				else if(val>CHANNEL_MAX_125)	val=CHANNEL_MAX_125;
				Channel_data[i]=val;
			}
			PPM_FLAG_off;									// wait for next frame before update
			update_channels_aux();
			INPUT_SIGNAL_on;								// valid signal received
			last_signal=millis();
		}
	#endif //ENABLE_PPM
	update_led_status();
	#if defined(TELEMETRY)
		#if ( !( defined(MULTI_TELEMETRY) || defined(MULTI_STATUS) ) )
			if( (protocol==PROTO_FRSKYD) || (protocol==PROTO_BAYANG) || (protocol==PROTO_NCC1701) || (protocol==PROTO_BUGS) || (protocol==PROTO_BUGSMINI) || (protocol==PROTO_HUBSAN) || (protocol==PROTO_AFHDS2A) || (protocol==PROTO_FRSKYX) || (protocol==PROTO_DSM) || (protocol==PROTO_CABELL)  || (protocol==PROTO_HITEC))
		#endif
				TelemetryUpdate();
	#endif
	#ifdef ENABLE_BIND_CH
		if(IS_AUTOBIND_FLAG_on && IS_BIND_CH_PREV_off && Channel_data[BIND_CH-1]>CHANNEL_MAX_COMMAND && Channel_data[THROTTLE]<(CHANNEL_MIN_100+50))
		{ // Autobind is on and BIND_CH went up and Throttle is low
			CHANGE_PROTOCOL_FLAG_on;							//reload protocol
			BIND_IN_PROGRESS;									//enable bind
			BIND_CH_PREV_on;
		}
		if(IS_AUTOBIND_FLAG_on && IS_BIND_CH_PREV_on && Channel_data[BIND_CH-1]<CHANNEL_MIN_COMMAND)
		{ // Autobind is on and BIND_CH went down
			BIND_CH_PREV_off;
			//Request protocol to terminate bind
			#if defined(FRSKYD_CC2500_INO) || defined(FRSKYX_CC2500_INO) || defined(FRSKYV_CC2500_INO)
			if(protocol==PROTO_FRSKYD || protocol==PROTO_FRSKYX || protocol==PROTO_FRSKYV)
				BIND_DONE;
			else
			#endif
			if(bind_counter>2)
				bind_counter=2;
		}
	#endif //ENABLE_BIND_CH
	if(IS_CHANGE_PROTOCOL_FLAG_on)
	{ // Protocol needs to be changed or relaunched for bind
		protocol_init();									//init new protocol
		return 1;
	}
	return 0;
}

// Update channels direction and Channel_AUX flags based on servo AUX positions
static void update_channels_aux(void)
{
	//Reverse channels direction
	#ifdef REVERSE_AILERON
		reverse_channel(AILERON);
	#endif
	#ifdef REVERSE_ELEVATOR
		reverse_channel(ELEVATOR);
	#endif
	#ifdef REVERSE_THROTTLE
		reverse_channel(THROTTLE);
	#endif
	#ifdef REVERSE_RUDDER
		reverse_channel(RUDDER);
	#endif
		
	//Calc AUX flags
	Channel_AUX=0;
	for(uint8_t i=0;i<8;i++)
		if(Channel_data[CH5+i]>CHANNEL_SWITCH)
			Channel_AUX|=1<<i;
}

// Update led status based on binding and serial
static void update_led_status(void)
{
	if(IS_INPUT_SIGNAL_on)
		if(millis()-last_signal>70)
			INPUT_SIGNAL_off;							//no valid signal (PPM or Serial) received for 70ms
	if(blink<millis())
	{
		if(IS_INPUT_SIGNAL_off)
		{
			if(mode_select==MODE_SERIAL)
				blink+=BLINK_SERIAL_TIME;				//blink slowly if no valid serial input
			else
				blink+=BLINK_PPM_TIME;					//blink more slowly if no valid PPM input
		}
		else
			if(remote_callback == 0)
			{ // Invalid protocol
				if(IS_LED_on)							//flash to indicate invalid protocol
					blink+=BLINK_BAD_PROTO_TIME_LOW;
				else
					blink+=BLINK_BAD_PROTO_TIME_HIGH;
			}
			else
			{
				if(IS_WAIT_BIND_on)
				{
					if(IS_LED_on)							//flash to indicate WAIT_BIND
						blink+=BLINK_WAIT_BIND_TIME_LOW;
					else
						blink+=BLINK_WAIT_BIND_TIME_HIGH;
				}
				else
				{
					if(IS_BIND_DONE)
						LED_off;							//bind completed force led on
					blink+=BLINK_BIND_TIME;					//blink fastly during binding
				}
			}
		LED_toggle;
	}
}

#ifdef ENABLE_PPM
uint8_t bank_switch(void)
{
	uint8_t bank=eeprom_read_byte((EE_ADDR)EEPROM_BANK_OFFSET);
	if(bank>=NBR_BANKS)
	{ // Wrong number of bank
		eeprom_write_byte((EE_ADDR)EEPROM_BANK_OFFSET,0x00);	// set bank to 0
		bank=0;
	}
	debugln("Using bank %d", bank);

	phase=3;
	uint32_t check=millis();
	blink=millis();
	while(mode_select==15)
	{ //loop here if the dial is on position 15 for user to select the bank
		if(blink<millis())
		{
			switch(phase & 0x03)
			{ // Flash bank number of times
				case 0:
					LED_on;
					blink+=BLINK_BANK_TIME_HIGH;
					phase++;
					break;
				case 1:
					LED_off;
					blink+=BLINK_BANK_TIME_LOW;
					phase++;
					break;
				case 2:
					if( (phase>>2) >= bank)
					{
						phase=0;
						blink+=BLINK_BANK_REPEAT;
					}
					else
						phase+=2;
					break;
				case 3:
					LED_output;
					LED_off;
					blink+=BLINK_BANK_TIME_LOW;
					phase=0;
					break;
			}
		}
		if(check<millis())
		{
			//Test bind button: for AVR it's shared with the LED so some extra work is needed to check it...
			#ifndef STM32_BOARD
				bool led=IS_LED_on;
				BIND_SET_INPUT;
				BIND_SET_PULLUP;
			#endif
			bool test_bind=IS_BIND_BUTTON_on;
			#ifndef STM32_BOARD
				if(led)
					LED_on;
				else
					LED_off;
				LED_output;
			#endif
			if( test_bind )
			{	// Increase bank
				LED_on;
				bank++;
				if(bank>=NBR_BANKS)
					bank=0;
				eeprom_write_byte((EE_ADDR)EEPROM_BANK_OFFSET,bank);
				debugln("Using bank %d", bank);
				phase=3;
				blink+=BLINK_BANK_REPEAT;
				check+=2*BLINK_BANK_REPEAT;
			}
			check+=1;
		}
	}
	return bank;
}
#endif

inline void tx_pause()
{
	#ifdef TELEMETRY
	// Pause telemetry by disabling transmitter interrupt
		#ifdef ORANGE_TX
			USARTC0.CTRLA &= ~0x03 ;
		#else
			#ifndef BASH_SERIAL
				#ifdef STM32_BOARD
					USART3_BASE->CR1 &= ~ USART_CR1_TXEIE;
				#else
					UCSR0B &= ~_BV(UDRIE0);
				#endif
			#endif
		#endif
	#endif
}

inline void tx_resume()
{
	#ifdef TELEMETRY
	// Resume telemetry by enabling transmitter interrupt
		#ifndef SPORT_POLLING
		if(!IS_TX_PAUSE_on)
		#endif
		{
			#ifdef ORANGE_TX
				cli() ;
				USARTC0.CTRLA = (USARTC0.CTRLA & 0xFC) | 0x01 ;
				sei() ;
			#else
				#ifndef BASH_SERIAL
					#ifdef STM32_BOARD
						USART3_BASE->CR1 |= USART_CR1_TXEIE;
					#else
						UCSR0B |= _BV(UDRIE0);			
					#endif
				#else
					resumeBashSerial();
				#endif
			#endif
		}
	#endif
}

// Protocol start
static void protocol_init()
{
	static uint16_t next_callback;
	if(IS_WAIT_BIND_off)
	{
		remote_callback = 0;			// No protocol
		next_callback=0;				// Default is immediate call back
		LED_off;						// Led off during protocol init
		modules_reset();				// Reset all modules

		// reset telemetry
		#ifdef TELEMETRY
			tx_pause();
			pass=0;
			telemetry_link=0;
			telemetry_lost=1;
			#ifdef BASH_SERIAL
				TIMSK0 = 0 ;			// Stop all timer 0 interrupts
				#ifdef INVERT_SERIAL
					SERIAL_TX_off;
				#else
					SERIAL_TX_on;
				#endif
				SerialControl.tail=0;
				SerialControl.head=0;
				SerialControl.busy=0;
			#else
				tx_tail=0;
				tx_head=0;
			#endif
			TX_RX_PAUSE_off;
			TX_MAIN_PAUSE_off;
		#endif

		//Set global ID and rx_tx_addr
		MProtocol_id = RX_num + MProtocol_id_master;
		set_rx_tx_addr(MProtocol_id);
		
		#ifdef FAILSAFE_ENABLE
			InitFailsafe();
		#endif
	
		blink=millis();

		PE1_on;							//NRF24L01 antenna RF3 by default
		PE2_off;						//NRF24L01 antenna RF3 by default
		
		debugln("Protocol selected: %d, sub proto %d, rxnum %d, option %d", protocol, sub_protocol, RX_num, option);

		switch(protocol)				// Init the requested protocol
		{
			#ifdef A7105_INSTALLED
				#if defined(FLYSKY_A7105_INO)
					case PROTO_FLYSKY:
						PE1_off;	//antenna RF1
						next_callback = initFlySky();
						remote_callback = ReadFlySky;
						break;
				#endif
				#if defined(AFHDS2A_A7105_INO)
					case PROTO_AFHDS2A:
						PE1_off;	//antenna RF1
						next_callback = initAFHDS2A();
						remote_callback = ReadAFHDS2A;
						break;
				#endif
				#if defined(HUBSAN_A7105_INO)
					case PROTO_HUBSAN:
						PE1_off;	//antenna RF1
						if(IS_BIND_BUTTON_FLAG_on) random_id(EEPROM_ID_OFFSET,true); // Generate new ID if bind button is pressed.
						next_callback = initHubsan();
						remote_callback = ReadHubsan;
						break;
				#endif
				#if defined(BUGS_A7105_INO)
					case PROTO_BUGS:
						PE1_off;	//antenna RF1
						next_callback = initBUGS();
						remote_callback = ReadBUGS;
						break;
				#endif
			#endif
			#ifdef CC2500_INSTALLED
				#if defined(FRSKYD_CC2500_INO)
					case PROTO_FRSKYD:
						PE1_off;	//antenna RF2
						PE2_on;
						next_callback = initFrSky_2way();
						remote_callback = ReadFrSky_2way;
						break;
				#endif
				#if defined(FRSKYV_CC2500_INO)
					case PROTO_FRSKYV:
						PE1_off;	//antenna RF2
						PE2_on;
						next_callback = initFRSKYV();
						remote_callback = ReadFRSKYV;
						break;
				#endif
				#if defined(FRSKYX_CC2500_INO)
					case PROTO_FRSKYX:
						PE1_off;	//antenna RF2
						PE2_on;
						next_callback = initFrSkyX();
						remote_callback = ReadFrSkyX;
						break;
				#endif
				#if defined(SFHSS_CC2500_INO)
					case PROTO_SFHSS:
						PE1_off;	//antenna RF2
						PE2_on;
						next_callback = initSFHSS();
						remote_callback = ReadSFHSS;
						break;
				#endif
				#if defined(CORONA_CC2500_INO)
					case PROTO_CORONA:
						PE1_off;	//antenna RF2
						PE2_on;
						next_callback = initCORONA();
						remote_callback = ReadCORONA;
						break;
				#endif
				#if defined(HITEC_CC2500_INO)
					case PROTO_HITEC:
						PE1_off;	//antenna RF2
						PE2_on;
						next_callback = initHITEC();
						remote_callback = ReadHITEC;
						break;
				#endif
			#endif
			#ifdef CYRF6936_INSTALLED
				#if defined(DSM_CYRF6936_INO)
					case PROTO_DSM:
						PE2_on;	//antenna RF4
						next_callback = initDsm();
						remote_callback = ReadDsm;
						break;
				#endif
				#if defined(WFLY_CYRF6936_INO)
					case PROTO_WFLY:
						PE2_on;	//antenna RF4
						next_callback = initWFLY();
						remote_callback = ReadWFLY;
						break;
				#endif
				#if defined(DEVO_CYRF6936_INO)
					case PROTO_DEVO:
						#ifdef ENABLE_PPM
							if(mode_select) //PPM mode
							{
								if(IS_BIND_BUTTON_FLAG_on)
								{
									eeprom_write_byte((EE_ADDR)(MODELMODE_EEPROM_OFFSET+RX_num),0x00);	// reset to autobind mode for the current model
									option=0;
								}
								else
								{	
									option=eeprom_read_byte((EE_ADDR)(MODELMODE_EEPROM_OFFSET+RX_num));	// load previous mode: autobind or fixed id
									if(option!=1) option=0;								// if not fixed id mode then it should be autobind
								}
							}
						#endif //ENABLE_PPM
						PE2_on;	//antenna RF4
						next_callback = DevoInit();
						remote_callback = devo_callback;
						break;
				#endif
				#if defined(WK2x01_CYRF6936_INO)
					case PROTO_WK2x01:
						#ifdef ENABLE_PPM
							if(mode_select) //PPM mode
							{
								if(IS_BIND_BUTTON_FLAG_on)
								{
									eeprom_write_byte((EE_ADDR)(MODELMODE_EEPROM_OFFSET+RX_num),0x00);	// reset to autobind mode for the current model
									option=0;
								}
								else
								{	
									option=eeprom_read_byte((EE_ADDR)(MODELMODE_EEPROM_OFFSET+RX_num));	// load previous mode: autobind or fixed id
									if(option!=1) option=0;								// if not fixed id mode then it should be autobind
								}
							}
						#endif //ENABLE_PPM
						PE2_on;	//antenna RF4
						next_callback = WK_setup();
						remote_callback = WK_cb;
						break;
				#endif
				#if defined(J6PRO_CYRF6936_INO)
					case PROTO_J6PRO:
						PE2_on;	//antenna RF4
						next_callback = initJ6Pro();
						remote_callback = ReadJ6Pro;
						break;
				#endif
			#endif
			#ifdef NRF24L01_INSTALLED
				#if defined(HISKY_NRF24L01_INO)
					case PROTO_HISKY:
						next_callback=initHiSky();
						remote_callback = hisky_cb;
						break;
				#endif
				#if defined(V2X2_NRF24L01_INO)
					case PROTO_V2X2:
						next_callback = initV2x2();
						remote_callback = ReadV2x2;
						break;
				#endif
				#if defined(YD717_NRF24L01_INO)
					case PROTO_YD717:
						next_callback=initYD717();
						remote_callback = yd717_callback;
						break;
				#endif
				#if defined(KN_NRF24L01_INO)
					case PROTO_KN:
						next_callback = initKN();
						remote_callback = kn_callback;
						break;
				#endif
				#if defined(SYMAX_NRF24L01_INO)
					case PROTO_SYMAX:
						next_callback = initSymax();
						remote_callback = symax_callback;
						break;
				#endif
				#if defined(SLT_NRF24L01_INO)
					case PROTO_SLT:
						next_callback=initSLT();
						remote_callback = SLT_callback;
						break;
				#endif
				#if defined(CX10_NRF24L01_INO)
					case PROTO_Q2X2:
						sub_protocol|=0x08;		// Increase the number of sub_protocols for CX-10
					case PROTO_CX10:
						next_callback=initCX10();
						remote_callback = CX10_callback;
						break;
				#endif
				#if defined(CG023_NRF24L01_INO)
					case PROTO_CG023:
						next_callback=initCG023();
						remote_callback = CG023_callback;
						break;
				#endif
				#if defined(BAYANG_NRF24L01_INO)
					case PROTO_BAYANG:
						next_callback=initBAYANG();
						remote_callback = BAYANG_callback;
						break;
				#endif
				#if defined(ESKY_NRF24L01_INO)
					case PROTO_ESKY:
						next_callback=initESKY();
						remote_callback = ESKY_callback;
						break;
				#endif
				#if defined(MT99XX_NRF24L01_INO)
					case PROTO_MT99XX:
						next_callback=initMT99XX();
						remote_callback = MT99XX_callback;
						break;
				#endif
				#if defined(MJXQ_NRF24L01_INO)
					case PROTO_MJXQ:
						next_callback=initMJXQ();
						remote_callback = MJXQ_callback;
						break;
				#endif
				#if defined(SHENQI_NRF24L01_INO)
					case PROTO_SHENQI:
						next_callback=initSHENQI();
						remote_callback = SHENQI_callback;
						break;
				#endif
				#if defined(FY326_NRF24L01_INO)
					case PROTO_FY326:
						next_callback=initFY326();
						remote_callback = FY326_callback;
						break;
				#endif
				#if defined(FQ777_NRF24L01_INO)
					case PROTO_FQ777:
						next_callback=initFQ777();
						remote_callback = FQ777_callback;
						break;
				#endif
				#if defined(ASSAN_NRF24L01_INO)
					case PROTO_ASSAN:
						next_callback=initASSAN();
						remote_callback = ASSAN_callback;
						break;
				#endif
				#if defined(HONTAI_NRF24L01_INO)
					case PROTO_HONTAI:
						next_callback=initHONTAI();
						remote_callback = HONTAI_callback;
						break;
				#endif
				#if defined(Q303_NRF24L01_INO)
					case PROTO_Q303:
						next_callback=initQ303();
						remote_callback = Q303_callback;
						break;
				#endif
				#if defined(GW008_NRF24L01_INO)
					case PROTO_GW008:
						next_callback=initGW008();
						remote_callback = GW008_callback;
						break;
				#endif
				#if defined(DM002_NRF24L01_INO)
					case PROTO_DM002:
						next_callback=initDM002();
						remote_callback = DM002_callback;
						break;
				#endif
				#if defined(CABELL_NRF24L01_INO)
					case PROTO_CABELL:
						next_callback=initCABELL();
						remote_callback = CABELL_callback;
						break;
				#endif
				#if defined(ESKY150_NRF24L01_INO)
					case PROTO_ESKY150:
						next_callback=initESKY150();
						remote_callback = ESKY150_callback;
						break;
				#endif
				#if defined(H8_3D_NRF24L01_INO)
					case PROTO_H8_3D:
						next_callback=initH8_3D();
						remote_callback = H8_3D_callback;
						break;
				#endif
				#if defined(CFLIE_NRF24L01_INO)
					case PROTO_CFLIE:
						next_callback=initCFlie();
						remote_callback = cflie_callback;
						break;
				#endif
				#if defined(BUGSMINI_NRF24L01_INO)
					case PROTO_BUGSMINI:
						next_callback=initBUGSMINI();
						remote_callback = BUGSMINI_callback;
						break;
				#endif
				#if defined(NCC1701_NRF24L01_INO)
					case PROTO_NCC1701:
						next_callback=initNCC();
						remote_callback = NCC_callback;
						break;
				#endif
				#if defined(E01X_NRF24L01_INO)
					case PROTO_E01X:
						next_callback=initE01X();
						remote_callback = E01X_callback;
						break;
				#endif
				#if defined(V911S_NRF24L01_INO)
					case PROTO_V911S:
						next_callback=initV911S();
						remote_callback = V911S_callback;
						break;
				#endif
				#if defined(GD00X_NRF24L01_INO)
					case PROTO_GD00X:
						next_callback=initGD00X();
						remote_callback = GD00X_callback;
						break;
				#endif
				#if defined(TEST_NRF24L01_INO)
					case PROTO_TEST:
						next_callback=initTest();
						remote_callback = Test_callback;
						break;
				#endif
			#endif
		}
	}

	#if defined(WAIT_FOR_BIND) && defined(ENABLE_BIND_CH)
		if( IS_AUTOBIND_FLAG_on && IS_BIND_CH_PREV_off && (cur_protocol[1]&0x80)==0 && mode_select == MODE_SERIAL)
		{ // Autobind is active but no bind requested by either BIND_CH or BIND. But do not wait if in PPM mode...
			WAIT_BIND_on;
			return;
		}
	#endif
	WAIT_BIND_off;
	CHANGE_PROTOCOL_FLAG_off;

	if(next_callback>32000)
	{ // next_callback should not be more than 32767 so we will wait here...
		uint16_t temp=(next_callback>>10)-2;
		delayMilliseconds(temp);
		next_callback-=temp<<10;				// between 2-3ms left at this stage
	}
	cli();										// disable global int
	OCR1A = TCNT1 + next_callback*2;			// set compare A for callback
	#ifndef STM32_BOARD
		TIFR1 = OCF1A_bm ;						// clear compare A flag
	#else
		TIMER2_BASE->SR = 0x1E5F & ~TIMER_SR_CC1IF;	// Clear Timer2/Comp1 interrupt flag
	#endif	
	sei();										// enable global int
	BIND_BUTTON_FLAG_off;						// do not bind/reset id anymore even if protocol change
}

void update_serial_data()
{
	RX_DONOTUPDATE_on;
	RX_FLAG_off;								//data is being processed
	#ifdef SAMSON	// Extremely dangerous, do not enable this unless you know what you are doing...
		if( rx_ok_buff[0]==0x55 && (rx_ok_buff[1]&0x1F)==PROTO_FRSKYD && rx_ok_buff[2]==0x7F && rx_ok_buff[24]==217 && rx_ok_buff[25]==202 )
		{//proto==FRSKYD+sub==7+rx_num==7+CH15==73%+CH16==73%
			rx_ok_buff[1]=(rx_ok_buff[1]&0xE0) | PROTO_FLYSKY;			// change the protocol to Flysky
			memcpy((void*)(rx_ok_buff+4),(void*)(rx_ok_buff+4+11),11);	// reassign channels 9-16 to 1-8
		}
	#endif
	if(rx_ok_buff[1]&0x20)						//check range
		RANGE_FLAG_on;
	else
		RANGE_FLAG_off;
	if(rx_ok_buff[1]&0x40)						//check autobind
		AUTOBIND_FLAG_on;
	else
		AUTOBIND_FLAG_off;
	if(rx_ok_buff[2]&0x80)						//if rx_ok_buff[2] ==1,power is low ,0-power high
		POWER_FLAG_off;							//power low
	else
		POWER_FLAG_on;							//power high

	//Forced frequency tuning values for CC2500 protocols
	#if defined(FORCE_FRSKYD_TUNING) && defined(FRSKYD_CC2500_INO)
		if(protocol==PROTO_FRSKYD) 
			option=FORCE_FRSKYD_TUNING;	// Use config-defined tuning value for FrSkyD
		else
	#endif
	#if defined(FORCE_FRSKYV_TUNING) && defined(FRSKYV_CC2500_INO)
		if(protocol==PROTO_FRSKYV)
			option=FORCE_FRSKYV_TUNING;	// Use config-defined tuning value for FrSkyV
		else
	#endif
	#if defined(FORCE_FRSKYX_TUNING) && defined(FRSKYX_CC2500_INO)
		if(protocol==PROTO_FRSKYX)
			option=FORCE_FRSKYX_TUNING;	// Use config-defined tuning value for FrSkyX
		else
	#endif 
	#if defined(FORCE_SFHSS_TUNING) && defined(SFHSS_CC2500_INO)
		if (protocol==PROTO_SFHSS)
			option=FORCE_SFHSS_TUNING;	// Use config-defined tuning value for SFHSS
		else
	#endif
	#if defined(FORCE_CORONA_TUNING) && defined(CORONA_CC2500_INO)
		if (protocol==PROTO_CORONA)
			option=FORCE_CORONA_TUNING;	// Use config-defined tuning value for CORONA
		else
	#endif
	#if defined(FORCE_HITEC_TUNING) && defined(HITEC_CC2500_INO)
		if (protocol==PROTO_HITEC)
			option=FORCE_HITEC_TUNING;	// Use config-defined tuning value for HITEC
		else
	#endif
			option=rx_ok_buff[3];		// Use radio-defined option value
	
	#ifdef FAILSAFE_ENABLE
		bool failsafe=false;
		if(rx_ok_buff[0]&0x02)
		{ // Packet contains failsafe instead of channels
			failsafe=true;
			rx_ok_buff[0]&=0xFD;				//remove the failsafe flag
			FAILSAFE_VALUES_on;					//failsafe data has been received
		}
	#endif
	#ifdef BONI
		if(CH14_SW)
			rx_ok_buff[2]=(rx_ok_buff[2]&0xF0)|((rx_ok_buff[2]+1)&0x0F);	// Extremely dangerous, do not enable this!!! This is really for a special case...
	#endif
	if( (rx_ok_buff[0] != cur_protocol[0]) || ((rx_ok_buff[1]&0x5F) != (cur_protocol[1]&0x5F)) || ( (rx_ok_buff[2]&0x7F) != (cur_protocol[2]&0x7F) ) )
	{ // New model has been selected
		CHANGE_PROTOCOL_FLAG_on;				//change protocol
		WAIT_BIND_off;
		if((rx_ok_buff[1]&0x80)!=0 || IS_AUTOBIND_FLAG_on)
			BIND_IN_PROGRESS;					//launch bind right away if in autobind mode or bind is set
		else
			BIND_DONE;
		protocol=(rx_ok_buff[0]==0x55?0:32) + (rx_ok_buff[1]&0x1F);	//protocol no (0-63) bits 4-6 of buff[1] and bit 0 of buf[0]
		sub_protocol=(rx_ok_buff[2]>>4)& 0x07;	//subprotocol no (0-7) bits 4-6
		RX_num=rx_ok_buff[2]& 0x0F;				// rx_num bits 0---3
	}
	else
		if( ((rx_ok_buff[1]&0x80)!=0) && ((cur_protocol[1]&0x80)==0) )		// Bind flag has been set
		{ // Restart protocol with bind
			CHANGE_PROTOCOL_FLAG_on;
			BIND_IN_PROGRESS;
		}
		else
			if( ((rx_ok_buff[1]&0x80)==0) && ((cur_protocol[1]&0x80)!=0) )	// Bind flag has been reset
			{ // Request protocol to end bind
				#if defined(FRSKYD_CC2500_INO) || defined(FRSKYX_CC2500_INO) || defined(FRSKYV_CC2500_INO)
				if(protocol==PROTO_FRSKYD || protocol==PROTO_FRSKYX || protocol==PROTO_FRSKYV)
					BIND_DONE;
				else
				#endif
				if(bind_counter>2)
					bind_counter=2;
			}
			
	//store current protocol values
	for(uint8_t i=0;i<3;i++)
		cur_protocol[i] =  rx_ok_buff[i];

	// decode channel/failsafe values
	volatile uint8_t *p=rx_ok_buff+3;
	uint8_t dec=-3;
	for(uint8_t i=0;i<NUM_CHN;i++)
	{
		dec+=3;
		if(dec>=8)
		{
			dec-=8;
			p++;
		}
		p++;
		uint16_t temp=((*((uint32_t *)p))>>dec)&0x7FF;
		#ifdef FAILSAFE_ENABLE
			if(failsafe)
				Failsafe_data[i]=temp;			//value range 0..2047, 0=no pulses, 2047=hold
			else
		#endif
				Channel_data[i]=temp;			//value range 0..2047, 0=-125%, 2047=+125%
	}
	RX_DONOTUPDATE_off;
	#ifdef ORANGE_TX
		cli();
	#else
		UCSR0B &= ~_BV(RXCIE0);					// RX interrupt disable
	#endif
	if(IS_RX_MISSED_BUFF_on)					// If the buffer is still valid
	{	memcpy((void*)rx_ok_buff,(const void*)rx_buff,RXBUFFER_SIZE);// Duplicate the buffer
		RX_FLAG_on;								// data to be processed next time...
		RX_MISSED_BUFF_off;
	}
	#ifdef ORANGE_TX
		sei();
	#else
		UCSR0B |= _BV(RXCIE0) ;					// RX interrupt enable
	#endif
	#ifdef FAILSAFE_ENABLE
		if(failsafe)
			debugln("RX_FS:%d,%d,%d,%d",Failsafe_data[0],Failsafe_data[1],Failsafe_data[2],Failsafe_data[3]);
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

#ifdef CHECK_FOR_BOOTLOADER
	void Mprotocol_serial_init( uint8_t boot )
#else
	void Mprotocol_serial_init()
#endif
{
	#ifdef ORANGE_TX
		PORTC.OUTSET = 0x08 ;
		PORTC.DIRSET = 0x08 ;

		USARTC0.BAUDCTRLA = 19 ;
		USARTC0.BAUDCTRLB = 0 ;
		
		USARTC0.CTRLB = 0x18 ;
		USARTC0.CTRLA = (USARTC0.CTRLA & 0xCC) | 0x11 ;
		USARTC0.CTRLC = 0x2B ;
		UDR0 ;
		#ifdef INVERT_SERIAL
			PORTC.PIN3CTRL |= 0x40 ;
		#endif
		#ifdef CHECK_FOR_BOOTLOADER
			if ( boot )
			{
				USARTC0.BAUDCTRLB = 0 ;
				USARTC0.BAUDCTRLA = 33 ;		// 57600
				USARTC0.CTRLA = (USARTC0.CTRLA & 0xC0) ;
				USARTC0.CTRLC = 0x03 ;			// 8 bit, no parity, 1 stop
				USARTC0.CTRLB = 0x18 ;			// Enable Tx and Rx
				PORTC.PIN3CTRL &= ~0x40 ;
			}
		#endif // CHECK_FOR_BOOTLOADER
	#elif defined STM32_BOARD
		#ifdef CHECK_FOR_BOOTLOADER
			if ( boot )
			{
				usart2_begin(57600,SERIAL_8N1);
				USART2_BASE->CR1 &= ~USART_CR1_RXNEIE ;
				(void)UDR0 ;
			}
			else
		#endif // CHECK_FOR_BOOTLOADER
		{
			usart2_begin(100000,SERIAL_8E2);
			USART2_BASE->CR1 |= USART_CR1_PCE_BIT;
		}
		usart3_begin(100000,SERIAL_8E2);
		#ifndef SPORT_POLLING
			USART3_BASE->CR1 &= ~ USART_CR1_RE;	//disable receive
		#endif		
		USART2_BASE->CR1 &= ~ USART_CR1_TE;		//disable transmit
	#else
		//ATMEGA328p
		#include <util/setbaud.h>	
		UBRR0H = UBRRH_VALUE;
		UBRR0L = UBRRL_VALUE;
		UCSR0A = 0 ;	// Clear X2 bit
		//Set frame format to 8 data bits, even parity, 2 stop bits
		UCSR0C = _BV(UPM01)|_BV(USBS0)|_BV(UCSZ01)|_BV(UCSZ00);
		while ( UCSR0A & (1 << RXC0) )	//flush receive buffer
			UDR0;
		//enable reception and RC complete interrupt
		UCSR0B = _BV(RXEN0)|_BV(RXCIE0);//rx enable and interrupt
		#ifndef DEBUG_PIN
			#if defined(TELEMETRY)
				initTXSerial( SPEED_100K ) ;
			#endif //TELEMETRY
		#endif //DEBUG_PIN
		#ifdef CHECK_FOR_BOOTLOADER
			if ( boot )
			{
				UBRR0H = 0;
				UBRR0L = 33;			// 57600
				UCSR0C &= ~_BV(UPM01);	// No parity
				UCSR0B &= ~_BV(RXCIE0);	// No rx interrupt
				UCSR0A |= _BV(U2X0);	// Double speed mode USART0
			}
		#endif // CHECK_FOR_BOOTLOADER
	#endif //ORANGE_TX
}

#ifdef STM32_BOARD
	void usart2_begin(uint32_t baud,uint32_t config )
	{
		usart_init(USART2); 
		usart_config_gpios_async(USART2,GPIOA,PIN_MAP[PA3].gpio_bit,GPIOA,PIN_MAP[PA2].gpio_bit,config);
		LED2_output;
		usart_set_baud_rate(USART2, STM32_PCLK1, baud);
		usart_enable(USART2);
	}
	void usart3_begin(uint32_t baud,uint32_t config )
	{
		usart_init(USART3);
		usart_config_gpios_async(USART3,GPIOB,PIN_MAP[PB11].gpio_bit,GPIOB,PIN_MAP[PB10].gpio_bit,config);
		usart_set_baud_rate(USART3, STM32_PCLK1, baud);
		usart_enable(USART3);
	}
	void init_HWTimer()
	{	
		HWTimer2.pause();									// Pause the timer2 while we're configuring it
		
		TIMER2_BASE->PSC = 35;								// 36-1;for 72 MHZ /0.5sec/(35+1)
		TIMER2_BASE->ARR = 0xFFFF;							// Count until 0xFFFF
		
		HWTimer2.setMode(TIMER_CH1, TIMER_OUTPUT_COMPARE);	// Main scheduler
		HWTimer2.setMode(TIMER_CH2, TIMER_OUTPUT_COMPARE);	// Serial check

		TIMER2_BASE->SR = 0x1E5F & ~TIMER_SR_CC2IF;			// Clear Timer2/Comp2 interrupt flag
		HWTimer2.attachInterrupt(TIMER_CH2,ISR_COMPB);		// Assign function to Timer2/Comp2 interrupt
		TIMER2_BASE->DIER &= ~TIMER_DIER_CC2IE;				// Disable Timer2/Comp2 interrupt
		
		HWTimer2.refresh();									// Refresh the timer's count, prescale, and overflow
		HWTimer2.resume();
	}
#endif

#ifdef CHECK_FOR_BOOTLOADER
void pollBoot()
{
	uint8_t rxchar ;
	uint8_t lState = BootState ;
	uint8_t millisTime = millis();				// Call this once only

	#ifdef ORANGE_TX
	if ( USARTC0.STATUS & USART_RXCIF_bm )
	#elif defined STM32_BOARD
	if ( USART2_BASE->SR & USART_SR_RXNE )
	#else
	if ( UCSR0A & ( 1 << RXC0 ) )
	#endif
	{
		rxchar = UDR0 ;
		BootCount += 1 ;
		if ( ( lState == BOOT_WAIT_30_IDLE ) || ( lState == BOOT_WAIT_30_DATA ) )
		{
			if ( lState == BOOT_WAIT_30_IDLE )	// Waiting for 0x30
				BootTimer = millisTime ;		// Start timeout
			if ( rxchar == 0x30 )
				lState = BOOT_WAIT_20 ;
			else
				lState = BOOT_WAIT_30_DATA ;
		}
		else
			if ( lState == BOOT_WAIT_20 && rxchar == 0x20 )	// Waiting for 0x20
				lState = BOOT_READY ;
	}
	else // No byte received
	{
		if ( lState != BOOT_WAIT_30_IDLE )		// Something received
		{
			uint8_t time = millisTime - BootTimer ;
			if ( time > 5 )
			{
				#ifdef	STM32_BOARD
				if ( BootCount > 4 )
				#else
				if ( BootCount > 2 )
				#endif
				{ // Run normally
					NotBootChecking = 0xFF ;
					Mprotocol_serial_init( 0 ) ;
				}
				else if ( lState == BOOT_READY )
				{
					#ifdef	STM32_BOARD
						nvic_sys_reset();
						while(1);						/* wait until reset */
					#else
						cli();							// Disable global int due to RW of 16 bits registers
						void (*p)();
						#ifndef ORANGE_TX
							p = (void (*)())0x3F00 ;	// Word address (0x7E00 byte)
						#else
							p = (void (*)())0x4000 ;	// Word address (0x8000 byte)
						#endif
						(*p)() ;						// go to boot
					#endif
				}
				else
				{
					lState = BOOT_WAIT_30_IDLE ;
					BootCount = 0 ;
				}
			}
		}
	}
	BootState = lState ;
}
#endif //CHECK_FOR_BOOTLOADER

#if defined(TELEMETRY)
void PPM_Telemetry_serial_init()
{
	if( (protocol==PROTO_FRSKYD) || (protocol==PROTO_HUBSAN) || (protocol==PROTO_AFHDS2A) || (protocol==PROTO_BAYANG)|| (protocol==PROTO_NCC1701) || (protocol==PROTO_CABELL)  || (protocol==PROTO_HITEC) || (protocol==PROTO_BUGS) || (protocol==PROTO_BUGSMINI))
		initTXSerial( SPEED_9600 ) ;
	if(protocol==PROTO_FRSKYX)
		initTXSerial( SPEED_57600 ) ;
	if(protocol==PROTO_DSM)
		initTXSerial( SPEED_125K ) ;
}
#endif

// Convert 32b id to rx_tx_addr
static void set_rx_tx_addr(uint32_t id)
{ // Used by almost all protocols
	rx_tx_addr[0] = (id >> 24) & 0xFF;
	rx_tx_addr[1] = (id >> 16) & 0xFF;
	rx_tx_addr[2] = (id >>  8) & 0xFF;
	rx_tx_addr[3] = (id >>  0) & 0xFF;
	rx_tx_addr[4] = (rx_tx_addr[2]&0xF0)|(rx_tx_addr[3]&0x0F);
}

static uint32_t random_id(uint16_t address, uint8_t create_new)
{
	#ifndef FORCE_GLOBAL_ID
		uint32_t id=0;

		if(eeprom_read_byte((EE_ADDR)(address+10))==0xf0 && !create_new)
		{  // TXID exists in EEPROM
			for(uint8_t i=4;i>0;i--)
			{
				id<<=8;
				id|=eeprom_read_byte((EE_ADDR)address+i-1);
			}
			if(id!=0x2AD141A7)	//ID with seed=0
			{
				debugln("Read ID from EEPROM");
				return id;
			}
		}
		// Generate a random ID
		#if defined STM32_BOARD
			#define STM32_UUID ((uint32_t *)0x1FFFF7E8)
			if (!create_new)
			{
				id = STM32_UUID[0] ^ STM32_UUID[1] ^ STM32_UUID[2];
				debugln("Generated ID from STM32 UUID");
			}
			else
		#endif
				id = random(0xfefefefe) + ((uint32_t)random(0xfefefefe) << 16);

		for(uint8_t i=0;i<4;i++)
			eeprom_write_byte((EE_ADDR)address+i,id >> (i*8));
		eeprom_write_byte((EE_ADDR)(address+10),0xf0);//write bind flag in eeprom.
		return id;
	#else
		(void)address;
		(void)create_new;
		return FORCE_GLOBAL_ID;
	#endif
}

/**************************/
/**************************/
/**  Interrupt routines  **/
/**************************/
/**************************/

//PPM
#ifdef ENABLE_PPM
	#ifdef ORANGE_TX
		#if PPM_pin == 2
			ISR(PORTD_INT0_vect)
		#else
			ISR(PORTD_INT1_vect)
		#endif
	#elif defined STM32_BOARD
		void PPM_decode()
	#else
		#if PPM_pin == 2
			ISR(INT0_vect, ISR_NOBLOCK)
		#else
			ISR(INT1_vect, ISR_NOBLOCK)
		#endif
	#endif
	{	// Interrupt on PPM pin
		static int8_t chan=0,bad_frame=1;
		static uint16_t Prev_TCNT1=0;
		uint16_t Cur_TCNT1;

		Cur_TCNT1 = TCNT1 - Prev_TCNT1 ;	// Capture current Timer1 value
		if(Cur_TCNT1<1600)
			bad_frame=1;					// bad frame
		else
			if(Cur_TCNT1>4400)
			{  //start of frame
				if(chan>=MIN_PPM_CHANNELS)
				{
					PPM_FLAG_on;			// good frame received if at least 4 channels have been seen
					if(chan>PPM_chan_max) PPM_chan_max=chan;	// Saving the number of channels received
				}
				chan=0;						// reset channel counter
				bad_frame=0;
			}
			else
				if(bad_frame==0)			// need to wait for start of frame
				{  //servo values between 800us and 2200us will end up here
					PPM_data[chan]=Cur_TCNT1;
					if(chan++>=MAX_PPM_CHANNELS)
						bad_frame=1;		// don't accept any new channels
				}
		Prev_TCNT1+=Cur_TCNT1;
	}
#endif //ENABLE_PPM

//Serial RX
#ifdef ENABLE_SERIAL
	#ifdef ORANGE_TX
		ISR(USARTC0_RXC_vect)
	#elif defined STM32_BOARD
		void __irq_usart2()			
	#else
		ISR(USART_RX_vect)
	#endif
	{	// RX interrupt
		static uint8_t idx=0;
		#ifdef ORANGE_TX
			if((USARTC0.STATUS & 0x1C)==0)		// Check frame error, data overrun and parity error
		#elif defined STM32_BOARD
			if((USART2_BASE->SR & USART_SR_RXNE) && (USART2_BASE->SR &0x0F)==0)					
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
				#ifdef FAILSAFE_ENABLE
					if((rx_buff[0]&0xFC)==0x54)	// If 1st byte is 0x54, 0x55, 0x56 or 0x57 it looks ok
				#else
					if((rx_buff[0]&0xFE)==0x54)	// If 1st byte is 0x54 or 0x55 it looks ok
				#endif
				{
					TX_RX_PAUSE_on;
					tx_pause();
					#if defined STM32_BOARD
						TIMER2_BASE->CCR2=TIMER2_BASE->CNT+(6500L);	// Full message should be received within timer of 3250us
						TIMER2_BASE->SR = 0x1E5F & ~TIMER_SR_CC2IF;	// Clear Timer2/Comp2 interrupt flag
						TIMER2_BASE->DIER |= TIMER_DIER_CC2IE;		// Enable Timer2/Comp2 interrupt
					#else
						OCR1B = TCNT1+(6500L) ;	// Full message should be received within timer of 3250us
						TIFR1 = OCF1B_bm ;		// clear OCR1B match flag
						SET_TIMSK1_OCIE1B ;		// enable interrupt on compare B match
					#endif
					idx++;
				}
			}
			else
			{
				rx_buff[idx++]=UDR0;		// Store received byte
				if(idx>=RXBUFFER_SIZE)
				{	// A full frame has been received
					if(!IS_RX_DONOTUPDATE_on)
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
			debugln("Bad frame RX");
		}
		if(discard_frame==1)
		{
			#ifdef STM32_BOARD
				TIMER2_BASE->DIER &= ~TIMER_DIER_CC2IE;	// Disable Timer2/Comp2 interrupt
			#else							
				CLR_TIMSK1_OCIE1B;			// Disable interrupt on compare B match
			#endif
			TX_RX_PAUSE_off;
			tx_resume();
		}
		#if not defined (ORANGE_TX) && not defined (STM32_BOARD)
			cli() ;
			UCSR0B |= _BV(RXCIE0) ;			// RX interrupt enable
		#endif
	}

	//Serial timer
	#ifdef ORANGE_TX
		ISR(TCC1_CCB_vect)
	#elif defined STM32_BOARD
		void ISR_COMPB()
	#else
		ISR(TIMER1_COMPB_vect, ISR_NOBLOCK )
	#endif
	{	// Timer1 compare B interrupt
		discard_frame=1;
		#ifdef STM32_BOARD
			TIMER2_BASE->DIER &= ~TIMER_DIER_CC2IE;	// Disable Timer2/Comp2 interrupt
			debugln("Bad frame timer");
		#else
			CLR_TIMSK1_OCIE1B;						// Disable interrupt on compare B match
		#endif
		tx_resume();
	}
#endif //ENABLE_SERIAL

#if not defined (ORANGE_TX) && not defined (STM32_BOARD)
	static void random_init(void)
	{
		cli();					// Temporarily turn off interrupts, until WDT configured
		MCUSR = 0;				// Use the MCU status register to reset flags for WDR, BOR, EXTR, and POWR
		WDTCSR |= _BV(WDCE);	// WDT control register, This sets the Watchdog Change Enable (WDCE) flag, which is  needed to set the prescaler
		WDTCSR = _BV(WDIE);		// Watchdog interrupt enable (WDIE)
		sei();					// Turn interupts on
	}

	static uint32_t random_value(void)
	{
		while (!gWDT_entropy);
		return gWDT_entropy;
	}

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
#endif
