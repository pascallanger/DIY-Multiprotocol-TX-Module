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


//uncomment for board selection.
#define STM32_board
#include <avr/pgmspace.h>

#ifdef STM32_board
//#undef __cplusplus
#include "Multiprotocol_STM32.h"
#include <libmaple/usart.h>
#include <libmaple/timer.h>
#include <SPI.h>	
#include <EEPROM.h>	
HardwareTimer timer(2);
#else
#include <avr/eeprom.h>
#include <util/delay.h>
#include "Multiprotocol.h"
#endif

#include "_Config.h"
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
uint16_t servo_max_100,servo_min_100,servo_max_125,servo_min_125;

// Protocol variables
uint8_t  cyrfmfg_id[6];//for dsm2 and devo
uint8_t  rx_tx_addr[5];
uint8_t  phase;
uint16_t bind_counter;
uint8_t  bind_phase;
uint8_t  binding_idx;
uint16_t packet_period;
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

// Mode_select variables
uint8_t mode_select;
uint8_t protocol_flags=0,protocol_flags2=0;

// PPM variable
volatile uint16_t PPM_data[NUM_CHN];

#ifndef XMEGA
//Random variable
volatile uint32_t gWDT_entropy=0;
#endif

//Serial protocol
uint8_t sub_protocol;
uint8_t protocol;
uint8_t option;
uint8_t cur_protocol[3];
uint8_t prev_option;
uint8_t  prev_power=0xFD; // unused power value

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




#ifdef STM32_board
void PPM_decode();
void ISR_COMPB();
#endif
// Telemetry
#define MAX_PKT 27
uint8_t pkt[MAX_PKT];//telemetry receiving packets
#if defined(TELEMETRY)
#ifdef INVERT_TELEMETRY
// enable bit bash for serial
#ifndef STM32_board
#ifndef XMEGA
#define	BASH_SERIAL 1
#endif
#define	INVERT_SERIAL 1
#endif
#endif

uint8_t pass = 0;
uint8_t pktt[MAX_PKT];//telemetry receiving packets
#ifndef BASH_SERIAL
#define TXBUFFER_SIZE 32
volatile uint8_t tx_buff[TXBUFFER_SIZE];
volatile uint8_t tx_head=0;
volatile uint8_t tx_tail=0;
#endif // BASH_SERIAL
uint8_t v_lipo;
int16_t RSSI_dBm;
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
	#if defined STM32_board	
	pinMode(A7105_CSN_pin,OUTPUT);
	pinMode(CC25_CSN_pin,OUTPUT);
	pinMode(NRF_CSN_pin,OUTPUT);
	pinMode(CYRF_CSN_pin,OUTPUT);
	pinMode(CYRF_RST_pin,OUTPUT);
	pinMode(CTRL1,OUTPUT);
	pinMode(CTRL2,OUTPUT);
	#if defined TELEMETRY
	pinMode(TX_INV_pin,OUTPUT);
	pinMode(RX_INV_pin,OUTPUT);
	#if defined INVERT_TELEMETRY
	TX_INV_on;//activated inverter for both serial TX and RX signals
	RX_INV_on;
	#else
	TX_INV_off;
	RX_INV_off;
	#endif	
	#endif
	//pinMode(SDI_pin,OUTPUT);
	//pinMode(SCK_pin,OUTPUT);//spi pins initialized with spi init
	//pinMode(SDO_pin,INPUT);			
	pinMode(BIND_pin,INPUT_PULLUP);
	pinMode(PPM_pin,INPUT);
	pinMode(S1_pin,INPUT_PULLUP);//dial switch
	pinMode(S2_pin,INPUT_PULLUP);
	pinMode(S3_pin,INPUT_PULLUP);
	pinMode(S4_pin,INPUT_PULLUP);
	//select the counter clock.
	start_timer2();//0.5us
	#else
	DDRD = (1<<CS_pin)|(1<<SDI_pin)|(1<<SCLK_pin)|(1<<A7105_CSN_pin)|(1<< CC25_CSN_pin);
	DDRC = (1<<CTRL1_pin)|(1<<CTRL2_pin); //output
	DDRC |= (1<<CYRF_RST_pin);//RST pin A5(C5) CYRF output
	DDRB  = _BV(0)|_BV(1);
	PORTB = _BV(2)|_BV(3)|_BV(4)|_BV(BIND_pin);//pullup 10,11,12 and bind button
	PORTC = _BV(0);//A0 high pullup
	#ifdef DEBUG_TX
	TX_SET_OUTPUT;
	#endif
	TCCR1A = 0;
	TCCR1B = (1 << CS11);	//prescaler8, set timer1 to increment every 0.5us(16Mhz) and start timer
	
	// Random
	random_init();
	#endif
	#endif
	
	// Set Chip selects
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
	#ifndef STM32_board
	SDI_on;
	SCK_off;
	#endif
	
	// Set servos positions
	for(uint8_t i=0;i<NUM_CHN;i++)
	Servo_data[i]=1500;
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
	mode_select = MODE_SERIAL ;
	#else
	#if defined STM32_board		
	mode_select= 0x0F -(uint8_t)(((GPIOA->regs->IDR)>>4)&0x0F);
	#else
	mode_select=0x0F - ( ( (PINB>>2)&0x07 ) | ( (PINC<<3)&0x08) );//encoder dip switches 1,2,4,8=>B2,B3,B4,C0
	#endif
	#endif
	
	//**********************************
	//mode_select=1;	// here to test PPM
	//**********************************
	// Update LED
	
	LED_off;
	LED_SET_OUTPUT;
	#ifdef STM32_board
	initSPI2();
	#endif
	
	modules_reset();
	// Read or create protocol id
#ifndef STM32_board	
#ifndef XMEGA
	//Init the seed with a random value created from watchdog timer for all protocols requiring random values
	randomSeed(random_value());
#endif
#endif
	MProtocol_id_master=random_id(10,false);


	//Init RF modules

	//LED_ON;	
	//Protocol and interrupts initialization
#ifdef ENABLE_PPM	
	//Protocol and interrupts initialization
	if(mode_select != MODE_SERIAL)
	{ // PPM
		mode_select--;
		protocol	=	PPM_prot[mode_select].protocol;
		cur_protocol[1] = protocol;
		sub_protocol   	=	PPM_prot[mode_select].sub_proto;
		RX_num			=	PPM_prot[mode_select].rx_num;
		MProtocol_id	=	RX_num + MProtocol_id_master;
		option			=	PPM_prot[mode_select].option;
		if(PPM_prot[mode_select].power)		POWER_FLAG_on;
		if(PPM_prot[mode_select].autobind)	AUTOBIND_FLAG_on;
		mode_select++;
		servo_max_100=PPM_MAX_100; servo_min_100=PPM_MIN_100;
		servo_max_125=PPM_MAX_125; servo_min_125=PPM_MIN_125;
		
		protocol_init();
		
		
		#ifndef STM32_board	
		//Configure PPM interrupt
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
		#endif		

		#ifdef STM32_board
		attachInterrupt(PPM_pin,PPM_decode,FALLING);
		#endif
		
		#if defined(TELEMETRY)
		PPM_Telemetry_serial_init();		// Configure serial for telemetry
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
		#ifdef STM32_board
		if((TIMER2_BASE->SR & TIMER_SR_CC1IF)!=0){
			cli();
			OCR1A = TCNT1;
			sei();
		}
		else
		while((TIMER2_BASE->SR & TIMER_SR_CC1IF )==0);//walit till compare match	
		#else
		if( (TIFR1 & OCF1A_bm) != 0)
		{
			cli();					// Disable global int due to RW of 16 bits registers
			OCR1A=TCNT1;	// Callback should already have been called... Use "now" as new sync point.
			sei();					// Enable global int
		}
		else
		while((TIFR1 & OCF1A_bm) == 0); // Wait before callback
		#endif
		do
		{
			#ifndef STM32_board			
			TX_MAIN_PAUSE_on;			
			tx_pause();
			#endif
			next_callback=remote_callback();
			#ifndef STM32_board	
			TX_MAIN_PAUSE_off;
			tx_resume();
			TX_OFF;
			#endif		   
			while(next_callback>4000)
			{ 										// start to wait here as much as we can...
				next_callback-=2000;				
				#ifdef STM32_board	
				cli();
				OCR1A+=2000*2;// clear compare A=callback flag 
				TIMER2_BASE->SR &= ~TIMER_SR_CC1IF;	//clear compare Flag
				sei();
				Update_All();
				if(IS_CHANGE_PROTOCOL_FLAG_on)
				break; // Protocol has been changed
				while((TIMER2_BASE->SR & TIMER_SR_CC1IF)==0);//2ms wait
				#else
				cli();
				OCR1A+=2000*2;	// clear compare A=callback flag 
				TIFR1=OCF1A_bm;						// clear compare A=callback flag
				sei();								// enable global int
				Update_All();
				if(IS_CHANGE_PROTOCOL_FLAG_on)
				break; // Protocol has been changed
				while((TIFR1 & OCF1A_bm) == 0);	// wait 2ms...
				#endif			
			}
			// at this point we have between 2ms and 4ms in next_callback
			next_callback *= 2 ;									// disable global int
			#ifdef STM32_board			
			cli();
			OCR1A+=next_callback;
			TIMER2_BASE->SR &= ~TIMER_SR_CC1IF;//clear compare Flag write zero 
			diff=OCR1A-TCNT1;						// compare timer and comparator
			sei();
			#else
			cli();
			OCR1A+=next_callback;					// set compare A for callback
			TIFR1=OCF1A_bm;							// clear compare A=callback flag
			diff=OCR1A-TCNT1;						// compare timer and comparator
			sei();									// enable global int
			#endif		
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
		if(cur_protocol[1]==0)	// No valid serial received at least once
		blink+=BLINK_SERIAL_TIME;					//blink slowly while waiting a valid serial input
		else
		if(remote_callback == 0)
		{ // Invalid protocol
			if(IS_LED_on)							//flash to indicate invalid protocol
			blink+=BLINK_BAD_PROTO_TIME_LOW;
			else
			blink+=BLINK_BAD_PROTO_TIME_HIGH;
		}
		else
		if(IS_BIND_DONE_on)
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
	#ifdef STM32_board
	USART3_BASE->CR1 &= ~ USART_CR1_TXEIE;//disable TX intrerupt
	#else
	UCSR0B &= ~_BV(UDRIE0);		// Pause telemetry by disabling transmitter interrupt
	#endif	
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
		#ifdef STM32_board
		USART3_BASE->CR1 |= USART_CR1_TXEIE;// TX intrrupt enabled
		#else		
		UCSR0B |= _BV(UDRIE0);			// Resume telemetry by enabling transmitter interrupt
		#endif	
		#endif
	}
#endif
}


#ifdef STM32_board	
void start_timer2(){	
	// Pause the timer while we're configuring it
	timer.pause();
	TIMER2_BASE->PSC = 35;//36-1;for 72 MHZ /0.5sec/(35+1)
	TIMER2_BASE->ARR = 0xFFFF;	//count till max
	timer.setMode(TIMER_CH1, TIMER_OUTPUT_COMPARE);
	timer.setMode(TIMER_CH2, TIMER_OUTPUT_COMPARE);
	// Refresh the timer's count, prescale, and overflow
	timer.refresh();
	timer.resume();
}
#endif

// Protocol start
static void protocol_init()
{
	uint16_t next_callback=0;		// Default is immediate call back
	remote_callback = 0;
	
	// reset telemetry
	#ifdef TELEMETRY
	#ifndef STM32_board
	tx_pause();
	#endif
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
	BIND_IN_PROGRESS;			// Indicates bind in progress for blinking bind led
	else
	BIND_DONE;
	
	PE1_on;						//NRF24L01 antenna RF3 by default
	PE2_off;						//NRF24L01 antenna RF3 by default
	
	switch(protocol)	// Init the requested protocol
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
				#ifdef STM32_board
				EEPROM.write((200+mode_select),0x00);		// reset to autobind mode for the current model
				#else
				eeprom_write_byte((uint8_t*)(30+mode_select),0x00);		// reset to autobind mode for the current model
				#endif	
				option=0;
			}
			else
			{	
				#ifdef STM32_board
				option=EEPROM.read((200+mode_select));	// load previous mode: autobind or fixed id
				#else
				option=eeprom_read_byte((uint8_t*)(30+mode_select));	// load previous mode: autobind or fixed id
				#endif	
				if(option!=1) option=0;									// if not fixed id mode then it should be autobind								// if not fixed id mode then it should be autobind
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
	
	if(next_callback>32000)
	{ // next_callback should not be more than 32767 so we will wait here...
		uint16_t temp=(next_callback>>10)-2;
		delayMilliseconds(temp);
		next_callback-=temp<<10;	// between 2-3ms left at this stage
	}

	#if defined STM32_board 
	cli();							// disable global int			
	OCR1A=TCNT1+next_callback*2;
	sei();
	TIMER2_BASE->SR &= ~TIMER_SR_CC1IF;//clear compare Flag write zero 
	#else
	cli();							// disable global int
	OCR1A = TCNT1 + next_callback*2;	// set compare A for callback
	sei();							// enable global int
	TIFR1 = OCF1A_bm ;				// clear compare A flag
	#endif	
	BIND_BUTTON_FLAG_off;			// do not bind/reset id anymore even if protocol change
}

static void update_serial_data()
{
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
	POWER_FLAG_off;	//power low
	else
	POWER_FLAG_on;	//power high
	
	option=rx_ok_buff[3];
	
	if( (rx_ok_buff[0] != cur_protocol[0]) || ((rx_ok_buff[1]&0x5F) != (cur_protocol[1]&0x5F)) || ( (rx_ok_buff[2]&0x7F) != (cur_protocol[2]&0x7F) ) )
	{ // New model has been selected
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
	for(uint8_t i=0;i<NUM_CHN;i++)
	{
		dec+=3;
		if(dec>=8)
		{
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
	if(IS_RX_MISSED_BUFF_on)	// If the buffer is still valid
	{	memcpy((void*)rx_ok_buff,(const void*)rx_buff,RXBUFFER_SIZE);// Duplicate the buffer
		RX_FLAG_on;				// data to be processed next time...
		RX_MISSED_BUFF_off;
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
	
	#if defined STM32_board
	Serial1.begin(100000,SERIAL_8E2);//USART2
	Serial2.begin(100000,SERIAL_8E2);//USART3 
	USART2_BASE->CR1 |= USART_CR1_PCE_BIT;
	USART3_BASE->CR1 &= ~ USART_CR1_RE;//disable 
	USART2_BASE->CR1 &= ~ USART_CR1_TE;//disable transmit
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
	#endif
	#endif
}

#if defined(TELEMETRY)
void PPM_Telemetry_serial_init()
{
	if( (protocol==MODE_FRSKYD) || (protocol==MODE_HUBSAN))
	initTXSerial( SPEED_9600 ) ;
	if(protocol==MODE_FRSKYX)
	initTXSerial( SPEED_57600 ) ;
	if(protocol==MODE_DSM)
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


#ifndef STM32_board
#ifndef XMEGA
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
#endif
#endif


#if defined STM32_board
static uint32_t random_id(uint16_t adress, uint8_t create_new)
{
	uint32_t id;
	uint8_t txid[4];
	pinMode(PB0, INPUT_ANALOG); // set up pin for analog input
	pinMode(PB1, INPUT_ANALOG); // set up pin for analog input
	
	if ((EEPROM.read(adress+100)==0xf0) && !create_new)
	{  // TXID exists in EEPROM
		for(uint8_t i=0;i<4;i++)
		txid[i]=EEPROM.read(adress+110);
		id=(txid[0] | ((uint32_t)txid[1]<<8) | ((uint32_t)txid[2]<<16) | ((uint32_t)txid[3]<<24));
		
	}
	else
	{ // if not generate a random ID
		randomSeed((uint32_t)analogRead(PB0)<<10|analogRead(PB1));//seed
		//
		id = random(0xfefefefe) + ((uint32_t)random(0xfefefefe) << 16);
		txid[0]=  (id &0xFF);
		txid[1] = ((id >> 8) & 0xFF);
		txid[2] = ((id >> 16) & 0xFF);
		txid[3] = ((id >> 24) & 0xFF);
		for(uint8_t i=0;i<4;i++) 
		EEPROM.write((adress+110),txid[i]);
		EEPROM.write(adress+100,0xF0);
	}
	return id;
}

#else
static uint32_t random_id(uint16_t adress, uint8_t create_new)
{
	uint32_t id;
	uint8_t txid[4];
	
	if (eeprom_read_byte((uint8_t*)(adress+10))==0xf0 && !create_new)
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

#endif




/**************************/
/**************************/
/**  Interrupt routines  **/
/**************************/
/**************************/

//PPM
#ifdef ENABLE_PPM
#ifdef XMEGA
#if PPM_pin == 2
ISR(PORTD_INT0_vect)
#else
ISR(PORTD_INT1_vect)
#endif
#else
#ifdef STM32_board
void PPM_decode()	
#else		
#if PPM_pin == 2
ISR(INT0_vect, ISR_NOBLOCK)
#else
ISR(INT1_vect, ISR_NOBLOCK)
#endif
#endif
#endif
{	// Interrupt on PPM pin
	static int8_t chan=-1;
	static uint16_t Prev_TCNT1=0;
	uint16_t Cur_TCNT1;
	Cur_TCNT1=TCNT1-Prev_TCNT1; // Capture current Timer1 value
	if(Cur_TCNT1<1000)
	chan=-1;				// bad frame
	else
	if(Cur_TCNT1>4840)
	{
		chan=0;				// start of frame
		PPM_FLAG_on;		// full frame present (even at startup since PPM_data has been initialized)
	}
	else
	if(chan!=-1)		// need to wait for start of frame
	{  //servo values between 500us and 2420us will end up here
		PPM_data[chan]= Cur_TCNT1>>1;;
		if(chan++>=NUM_CHN)
		chan=-1;	// don't accept any new channels
	}
	Prev_TCNT1+=Cur_TCNT1;
}
#endif //ENABLE_PPM



#ifdef ENABLE_SERIAL
//Serial RX
#ifdef XMEGA
ISR(USARTC0_RXC_vect)
#else
#if defined STM32_board
#ifdef __cplusplus
extern "C" {
	#endif	
	void __irq_usart2()			
	#else
	ISR(USART_RX_vect)
	#endif
	#endif
	
	{	// RX interrupt
		static uint8_t idx=0;
		#ifdef XMEGA
		if((USARTC0.STATUS & 0x1C)==0)			// Check frame error, data overrun and parity error
		#else
		#ifndef STM32_board
		UCSR0B &= ~_BV(RXCIE0) ;		// RX interrupt disable
		sei();
		#endif
		#if defined STM32_board
		if(USART2_BASE->SR & USART_SR_RXNE) {
			if((USART2_BASE->SR &0x0F)==0)					
			#else				
			if((UCSR0A&0x1C)==0)			// Check frame error, data overrun and parity error
			#endif	
			#endif
			{ // received byte is ok to process
				if(idx==0||discard_frame==1)
				{	// Let's try to sync at this point
					idx=0;discard_frame=0;
					RX_MISSED_BUFF_off;			// If rx_buff was good it's not anymore...
					rx_buff[0]=UDR0;
					if((rx_buff[0]&0xFE)==0x54)	// If 1st byte is 0x54 or 0x55 it looks ok
					{
						#ifndef STM32_board
						TX_RX_PAUSE_on;
						tx_pause();
						#endif
						#if defined STM32_board
						uint16_t OCR1B;
						OCR1B =TCNT1+(6500L);
						timer.setCompare(TIMER_CH2,OCR1B);
						timer.attachCompare2Interrupt(ISR_COMPB);
						#else
						OCR1B=TCNT1+(6500L);		// Full message should be received within timer of 3250us
						TIFR1 = OCF1B_bm ;		// clear OCR1B match flag
						SET_TIMSK1_OCIE1B ;		// enable interrupt on compare B match			
						#endif
						idx++;
					}
				}
				else
				{					
					rx_buff[idx++]=UDR0;			// Store received byte
					if(idx>=RXBUFFER_SIZE)
					{	// A full frame has been received
						if(!IS_RX_DONOTUPDTAE_on)
						{ //Good frame received and main is not working on the buffer
							memcpy((void*)rx_ok_buff,(const void*)rx_buff,RXBUFFER_SIZE);// Duplicate the buffer
							RX_FLAG_on;					// flag for main to process servo data
						}
						else
						RX_MISSED_BUFF_on;			// notify that rx_buff is good
						discard_frame=1; 				// start again
					}
				}
			}
			else
			{					
				idx=UDR0;	// Dummy read
				discard_frame=1;		// Error encountered discard full frame...
			}
			if(discard_frame==1)
			{
				#if defined STM32_board
				detachInterrupt(2);//disable interrupt on ch2	
				#else							
				CLR_TIMSK1_OCIE1B;			// Disable interrupt on compare B match
				TX_RX_PAUSE_off;
				tx_resume();
				#endif
			}
			#ifndef XMEGA
			#ifndef STM32_board
			cli() ;			
			UCSR0B |= _BV(RXCIE0) ;	// RX interrupt enable
			#endif
			#endif
			#if defined STM32_board	//If activated telemetry it doesn't work activated 
		}
		#endif			
	}			
	#if defined STM32_board			
	#ifdef __cplusplus
}
#endif
#endif




//Serial timer
#ifdef XMEGA
ISR(TCC1_CCB_vect)
#else
#if defined STM32_board
void ISR_COMPB()		
#else
ISR(TIMER1_COMPB_vect,ISR_NOBLOCK)
#endif
#endif

{	// Timer1 compare B interrupt
	discard_frame=1;	// Error encountered discard full frame...
	#ifdef STM32_board
	detachInterrupt(2);//disable interrupt on ch2
	#else
	CLR_TIMSK1_OCIE1B;			// Disable interrupt on compare B match
	#endif
	#ifndef STM32_board
	tx_resume();
	#endif
}


#ifndef STM32_board
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
#endif
#endif

#endif //ENABLE_SERIAL
