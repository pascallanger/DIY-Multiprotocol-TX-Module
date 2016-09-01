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
#define STM32_board
//#undef __cplusplus
#if defined STM32_board
#include "Multiprotocol_STM32.h"
#include <EEPROM.h>
#include <libmaple/usart.h>
#include <libmaple/timer.h>
#include <SPI.h>
uint16_t OCR1A = 0;
uint16_t TCNT1=0;
HardwareTimer timer(2);
#else
#include <avr/eeprom.h>
#include <util/delay.h>
#include "Multiprotocol.h"
#endif

#include <avr/pgmspace.h>
#include "_Config.h"
#include "TX_Def.h"
//Multiprotocol module configuration file

//Global constants/variables
uint32_t MProtocol_id;//tx id,
uint32_t MProtocol_id_master;
uint32_t Model_fixed_id=0;
uint32_t fixed_id;
uint32_t blink=0;
uint8_t  prev_option;//change option value on the fly.
uint8_t  prev_power=0xFD; // unused power value
//
uint16_t counter;
uint8_t  channel;
uint8_t  packet[40];

#define NUM_CHN 16
// Servo data
uint16_t Servo_data[NUM_CHN];
uint8_t  Servo_AUX;
uint16_t servo_max_100,servo_min_100,servo_max_125,servo_min_125;

#ifndef STM32_board
uint16_t servo_max_100,servo_min_100,servo_max_125,servo_min_125;
#endif
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

// Serial variables
#ifdef INVERT_TELEMETRY
// enable bit bash for serial
#define	BASH_SERIAL 1
#define	INVERT_SERIAL 1
#endif
#define BAUD 100000
#define RXBUFFER_SIZE 25
#define TXBUFFER_SIZE 32
volatile uint8_t rx_buff[RXBUFFER_SIZE];
volatile uint8_t rx_ok_buff[RXBUFFER_SIZE];
#ifndef BASH_SERIAL
volatile uint8_t tx_buff[TXBUFFER_SIZE];
#endif
volatile uint8_t discard_frame = 0;

//Serial protocol
uint8_t sub_protocol;
uint8_t option;
uint8_t cur_protocol[2];
uint8_t prev_protocol=0;

#ifdef STM32_board
void PPM_decode();
void ISR_COMPB();
#endif
// Telemetry
#define MAX_PKT 27
uint8_t pkt[MAX_PKT];//telemetry receiving packets
#if defined(TELEMETRY)
uint8_t pass = 0;
uint8_t pktt[MAX_PKT];//telemetry receiving packets
#ifndef BASH_SERIAL
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
static void CheckTimer(uint16_t (*cb)(void));

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
	PORTE.OUTSET = 0x01 ;
	
	for ( uint8_t count = 0 ; count < 20 ; count += 1 )
	asm("nop") ;
	PORTE.OUTCLR = 0x01 ;
	#else	
	// General pinout
	#if defined STM32_board	
	pinMode(CS_pin,OUTPUT);
	pinMode(CC25_CSN_pin,OUTPUT);
	pinMode(NRF_CSN_pin,OUTPUT);
	pinMode(CYRF_CSN_pin,OUTPUT);
	pinMode(CYRF_RST_pin,OUTPUT);
	pinMode(CTRL1,OUTPUT);
	pinMode(CTRL2,OUTPUT);
	#if defined TELEMETRY
	pinMode(TX_INV_pin,OUTPUT);
	pinMode(RX_INV_pin,OUTPUT);
	#if defined TARANIS
	TX_INV_on;
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
	#else
	DDRD = (1<<CS_pin)|(1<<SDI_pin)|(1<<SCLK_pin)|(1<<CS_pin)|(1<< CC25_CSN_pin);
	DDRC = (1<<CTRL1)|(1<<CTRL2); //output
	DDRC |= (1<<5);//RST pin A5(C5) CYRF output
	DDRB  = _BV(0)|_BV(1);
	PORTB = _BV(2)|_BV(3)|_BV(4)|_BV(5);//pullup 10,11,12 and bind button
	PORTC = _BV(0);//A0 high pullup
	#endif
	#endif
	
	// Set Chip selects
	CS_on;
	CC25_CSN_on;
	NRF_CSN_on;
	CYRF_CSN_on;
	//	Set SPI lines
	#ifndef STM32_board
	SDI_on;
	SCK_off;
	#endif
	//#ifdef XMEGA
	//	// SPI enable, master, prescale of 16
	//	SPID.CTRL = SPI_ENABLE_bm | SPI_MASTER_bm | SPI_PRESCALER0_bm ;
	//#endif	
	// Timer1 config
	#ifdef XMEGA
	// TCC1 16-bit timer, clocked at 0.5uS
	EVSYS.CH3MUX = 0x80 + 0x04 ;	// Prescaler of 16
	TCC1.CTRLB = 0; TCC1.CTRLC = 0; TCC1.CTRLD = 0; TCC1.CTRLE = 0;
	TCC1.INTCTRLA = 0; TCC1.INTCTRLB = 0;
	TCC1.PER = 0xFFFF ;
	TCC1.CNT = 0 ;
	TCC1.CTRLA = 0x0B ;	// Event3 (prescale of 16)
	#else
	#if defined STM32_board//16 bits timer for 0.5 us
	//select the counter clock.
	start_timer2();//0.5us
	#else	
	TCCR1A = 0;
	TCCR1B = (1 << CS11);	//prescaler8, set timer1 to increment every 0.5us(16Mhz) and start timer
	#endif
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
	// Read status of bind button
	if( IS_BIND_BUTTON_on )
	BIND_BUTTON_FLAG_on;	// If bind button pressed save the status for protocol id reset under hubsan

	// Read status of mode select binary switch
	// after this mode_select will be one of {0000, 0001, ..., 1111}
	#ifdef XMEGA
	mode_select = MODE_SERIAL ;
	#else
	#if defined STM32_board		
	mode_select= 0x0F -(uint8_t)(((GPIOA->regs->IDR)>>4)&0x0F);
	#else
	mode_select=0x0F - ( ( (PINB>>2)&0x07 ) | ( (PINC<<3)&0x08) );//encoder dip switches 1,2,4,8=>B2,B3,B4,C0
	#endif
	#endif
	
	//**********************************
	//mode_select=0;	// here to test PPM
	//**********************************
	// Update LED
	
	LED_OFF;
	LED_SET_OUTPUT; 	
	// Read or create protocol id
	MProtocol_id_master=random_id(10,false);
	
	#ifdef STM32_board
	initSPI2();
	#endif
	//Init RF modules
	modules_reset();
	//LED_ON;	
	//Protocol and interrupts initialization
#ifdef ENABLE_PPM	
	if(mode_select != MODE_SERIAL)
	{ // PPM
		mode_select--;
		cur_protocol[0]	=	PPM_prot[mode_select].protocol;
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
		
		#ifndef XMEGA
		#ifndef STM32_board	
		//Configure PPM interrupt
		EICRA |=(1<<ISC11);		// The rising edge of INT1 pin D3 generates an interrupt request
		EIMSK |= (1<<INT1);		// INT1 interrupt enable
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
#endif
	{ // Serial
#ifdef ENABLE_SERIAL
		cur_protocol[0]=0;
		cur_protocol[1]=0;
		prev_protocol=0;
		servo_max_100=SERIAL_MAX_100; servo_min_100=SERIAL_MIN_100;
		servo_max_125=SERIAL_MAX_125; servo_min_125=SERIAL_MIN_125;
		Mprotocol_serial_init(); // Configure serial and enable RX interrupt
#endif //ENABLE_SERIAL		
	}

}

// Main
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
		#ifdef XMEGA		
		if( (TCC1.INTFLAGS & TC1_CCAIF_bm) != 0)
		{
			cli();					// disable global int
			TCC1.CCA = TCC1.CNT ;	// Callback should already have been called... Use "now" as new sync point.
			sei();					// enable global int
		}
		else
		while((TCC1.INTFLAGS & TC1_CCAIF_bm) == 0); // wait before callback
		#else
		#if defined STM32_board
		if((TIMER2_BASE->SR & TIMER_SR_CC1IF)!=0){
			cli();
			OCR1A = TIMER2_BASE->CNT;
			TIMER2_BASE->CCR1=OCR1A; 
			sei();
		}
		else
		while((TIMER2_BASE->SR & TIMER_SR_CC1IF )==0);//walit till compare match	
		#else
		if( (TIFR1 & (1<<OCF1A)) != 0)
		{
			cli();			// disable global int
			OCR1A=TCNT1;	// Callback should already have been called... Use "now" as new sync point.
			sei();			// enable global int
		}
		else
		while((TIFR1 & (1<<OCF1A)) == 0); // wait before callback
		#endif		
		#endif
		do
		{
			#ifndef STM32_board			
			TX_ON;
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
				
				#ifdef XMEGA
				cli();								// disable global int
				TCC1.CCA +=2000*2;					// set compare A for callback
				TCC1.INTFLAGS = TC1_CCAIF_bm ;		// clear compare A=callback flag
				sei();								// enable global int
				Update_All();
				if(IS_CHANGE_PROTOCOL_FLAG_on)
				break; // Protocol has been changed
				while((TCC1.INTFLAGS & TC1_CCAIF_bm) == 0); // wait 2ms...
				#else
				
				#if defined STM32_board	
				cli();
				OCR1A+=2000*2;// clear compare A=callback flag 
				TIMER2_BASE->CCR1=OCR1A;
				TCNT1 = TIMER2_BASE->CNT; 
				TIMER2_BASE->SR &= ~TIMER_SR_CC1IF;	//clear compare Flag
				sei();
				Update_All();
				if(IS_CHANGE_PROTOCOL_FLAG_on)
				break; // Protocol has been changed
				while((TIMER2_BASE->SR &TIMER_SR_CC1IF)==0);//2ms wait
				#else
				cli();
				OCR1A+=2000*2;	// clear compare A=callback flag 
				TIFR1=(1<<OCF1A);					// clear compare A=callback flag
				sei();								// enable global int
				Update_All();
				if(IS_CHANGE_PROTOCOL_FLAG_on)
				break; // Protocol has been changed
				while((TIFR1 & (1<<OCF1A)) == 0);	// wait 2ms...
				#endif			
				#endif
			}
			// at this point we have between 2ms and 4ms in next_callback
			next_callback *= 2 ;									// disable global int
			#ifdef XMEGA
			cli();
			TCC1.CCA +=next_callback;				// set compare A for callback
			TCC1.INTFLAGS = TC1_CCAIF_bm ;			// clear compare A=callback flag
			diff=TCC1.CCA-TCC1.CNT;					// compare timer and comparator
			sei();// enable global int			
			#else
			#if defined STM32_board
			OCR1A+=next_callback;
			cli();
			TIMER2_BASE->CCR1 = OCR1A;
			TCNT1 = TIMER2_BASE->CNT;
			TIMER2_BASE->SR &= ~TIMER_SR_CC1IF;//clear compare Flag write zero 
			diff=OCR1A-TCNT1;						// compare timer and comparator
			sei();
			#else
			cli();
			OCR1A+=next_callback;					// set compare A for callback
			TIFR1=(1<<OCF1A);						// clear compare A=callback flag
			diff=OCR1A-TCNT1;						// compare timer and comparator
			sei();									// enable global int
			#endif		
			#endif
		}
		while(diff&0x8000);	 						// Callback did not took more than requested time for next callback
		// so we can launch Update_All before next callback
	}
}


void Update_All()
{
#ifndef STM32_board
	TX_ON;
	NOP();
	TX_OFF;
#endif

#ifdef ENABLE_SERIAL
	if(mode_select==MODE_SERIAL && IS_RX_FLAG_on)	// Serial mode and something has been received
	{
		update_serial_data();	// Update protocol and data
		update_aux_flags();
		if(IS_CHANGE_PROTOCOL_FLAG_on)
		{ // Protocol needs to be changed
			LED_OFF;									//led off during protocol init
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
	uint8_t protocol=cur_protocol[0]&0x1F;
	if( (protocol==MODE_FRSKY) || (protocol==MODE_HUBSAN) || (protocol==MODE_FRSKYX) || (protocol==MODE_DSM2) )
	TelemetryUpdate();
	#endif
#ifndef STM32_board
	TX_ON;
	NOP();
	TX_OFF;
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
		if(cur_protocol[0]==0)	// No valid serial received at least once
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
		LED_OFF;									//bind completed -> led on
		else
		blink+=BLINK_BIND_TIME;				//blink fastly during binding
		LED_TOGGLE;
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
	#ifdef XMEGA
	USARTC0.CTRLA = (USARTC0.CTRLA & 0xFC) | 0x01 ;	// Resume telemetry by enabling transmitter interrupt
	#else
	#ifdef STM32_board
	USART3_BASE->CR1 |= USART_CR1_TXEIE;// TX intrrupt enabled
	#else		
	UCSR0B |= _BV(UDRIE0);			// Resume telemetry by enabling transmitter interrupt
	#endif	
	#endif
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

// Protocol scheduler
static void CheckTimer(uint16_t (*cb)(void))
{ 
	uint16_t next_callback,diff;
	#ifdef XMEGA		
	if( (TCC1.INTFLAGS & TC1_CCAIF_bm) != 0)
	{
		cli();					// disable global int
		TCC1.CCA = TCC1.CNT ;	// Callback should already have been called... Use "now" as new sync point.
		sei();					// enable global int
	}
	else
	while((TCC1.INTFLAGS & TC1_CCAIF_bm) == 0); // wait before callback
	#else
	#if defined STM32_board
	if((TIMER2_BASE->SR & TIMER_SR_CC1IF)!=0){
		cli();
		OCR1A = TIMER2_BASE->CNT;
		TIMER2_BASE->CCR1=OCR1A; 
		sei();
	}
	else
	while((TIMER2_BASE->SR & TIMER_SR_CC1IF )==0);//walit till compare match	
	#else
	if( (TIFR1 & (1<<OCF1A)) != 0)
	{
		cli();			// disable global int
		OCR1A=TCNT1;	// Callback should already have been called... Use "now" as new sync point.
		sei();			// enable global int
	}
	else
	while((TIFR1 & (1<<OCF1A)) == 0); // wait before callback
	#endif		
	#endif
	do
	{
		next_callback=cb();
		while(next_callback>4000)
		{ 										// start to wait here as much as we can...
			next_callback=next_callback-2000;
			
			#ifdef XMEGA
			cli();								// disable global int
			TCC1.CCA +=2000*2;					// set compare A for callback
			TCC1.INTFLAGS = TC1_CCAIF_bm ;		// clear compare A=callback flag
			sei();								// enable global int
			while((TCC1.INTFLAGS & TC1_CCAIF_bm) == 0); // wait 2ms...
			#else
			
			#if defined STM32_board	
			cli();
			OCR1A+=2000*2;// clear compare A=callback flag 
			TIMER2_BASE->CCR1=OCR1A;
			TCNT1 = TIMER2_BASE->CNT; 
			TIMER2_BASE->SR &= ~TIMER_SR_CC1IF;	//clear compare Flag
			sei();
			while((TIMER2_BASE->SR &TIMER_SR_CC1IF)==0);//2ms wait
			#else
			cli();
			OCR1A+=2000*2;	// clear compare A=callback flag 
			TIFR1=(1<<OCF1A);					// clear compare A=callback flag
			sei();								// enable global int
			while((TIFR1 & (1<<OCF1A)) == 0);	// wait 2ms...
			#endif			
			#endif
		}
		// at this point we have between 2ms and 4ms in next_callback
		next_callback *= 2 ;									// disable global int
		#ifdef XMEGA
		cli();
		TCC1.CCA +=next_callback;				// set compare A for callback
		TCC1.INTFLAGS = TC1_CCAIF_bm ;			// clear compare A=callback flag
		diff=TCC1.CCA-TCC1.CNT;					// compare timer and comparator
		sei();// enable global int			
		#else
		#if defined STM32_board
		OCR1A+=next_callback;
		cli();
		TIMER2_BASE->CCR1 = OCR1A;
		TCNT1 = TIMER2_BASE->CNT;
		TIMER2_BASE->SR &= ~TIMER_SR_CC1IF;//clear compare Flag write zero 
		diff=OCR1A-TCNT1;						// compare timer and comparator
		sei();
		#else
		cli();
		OCR1A+=next_callback;					// set compare A for callback
		TIFR1=(1<<OCF1A);						// clear compare A=callback flag
		diff=OCR1A-TCNT1;						// compare timer and comparator
		sei();									// enable global int
		#endif		
		#endif
	}
	while(diff&0x8000);	 						// Callback did not took more than requested time for next callback
	// so we can let main do its stuff before next callback
}

// Protocol start
static void protocol_init()
{
	uint16_t next_callback=0;		// Default is immediate call back
	remote_callback = 0;
	
	set_rx_tx_addr(MProtocol_id);
	// reset telemetry
	#ifdef TELEMETRY
	#ifndef STM32_board
	tx_pause();
	#endif
	pass=0;
	telemetry_link=0;
	tx_tail=0;
	tx_head=0;
	#endif
	
	blink=millis();
	if(IS_BIND_BUTTON_FLAG_on)
	AUTOBIND_FLAG_on;
	if(IS_AUTOBIND_FLAG_on)
	BIND_IN_PROGRESS;			// Indicates bind in progress for blinking bind led
	else
	BIND_DONE;
	
	CTRL1_on;	//NRF24L01 antenna RF3 by default
	CTRL2_off;	//NRF24L01 antenna RF3 by default
	
	switch(cur_protocol[0]&0x1F)	// Init the requested protocol
	{
		#if defined(FLYSKY_A7105_INO)
	case MODE_FLYSKY:
		CTRL1_off;	//antenna RF1
		next_callback = initFlySky();
		remote_callback = ReadFlySky;
		break;
		#endif
		#if defined(HUBSAN_A7105_INO)
	case MODE_HUBSAN:
		CTRL1_off;	//antenna RF1
		if(IS_BIND_BUTTON_FLAG_on) random_id(10,true); // Generate new ID if bind button is pressed.	
		next_callback = initHubsan();
		remote_callback = ReadHubsan;
		break;
		#endif
		#if defined(FRSKY_CC2500_INO)
	case MODE_FRSKY:
		CTRL1_off;	//antenna RF2
		CTRL2_on;
		next_callback = initFrSky_2way();
		remote_callback = ReadFrSky_2way;
		break;
		#endif
		#if defined(FRSKYX_CC2500_INO)
	case MODE_FRSKYX:
		CTRL1_off;	//antenna RF2
		CTRL2_on;
		next_callback = initFrSkyX();
		remote_callback = ReadFrSkyX;
		break;
		#endif
		#if defined(SFHSS_CC2500_INO)
	case MODE_SFHSS:
		CTRL1_off;	//antenna RF2
		CTRL2_on;
		next_callback = initSFHSS();
		remote_callback = ReadSFHSS;
		break;
		#endif
		#if defined(DSM2_CYRF6936_INO)
	case MODE_DSM2:
		CTRL2_on;	//antenna RF4
		next_callback = initDsm2();
		//Servo_data[2]=1500;//before binding
		remote_callback = ReadDsm2;
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
		CTRL2_on;	//antenna RF4
		next_callback = DevoInit();
		remote_callback = devo_callback;
		break;
		#endif
		#if defined(J6PRO_CYRF6936_INO)
	case MODE_J6PRO:
		CTRL2_on;	//antenna RF4
		next_callback = initJ6Pro();
		remote_callback = ReadJ6Pro;
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
		
	}
	
	if(next_callback>32000)
	{ // next_callback should not be more than 32767 so we will wait here...
		uint16_t temp=(next_callback>>10)-2;
		delay(temp);
		next_callback-=temp<<10;	// between 2-3ms left at this stage
	}

	#ifdef XMEGA
	cli();							// disable global int
	TCC1.CCA = TCC1.CNT + next_callback*2;	// set compare A for callback
	sei();							// enable global int
	TCC1.INTFLAGS = TC1_CCAIF_bm ;	// clear compare A flag
	#else
	#if defined STM32_board 
	cli();							// disable global int
	TCNT1 = TIMER2_BASE->CNT;			
	OCR1A=TCNT1+next_callback*2;
	TIMER2_BASE->CCR1 = OCR1A;
	sei();
	TIMER2_BASE->SR &= ~TIMER_SR_CC1IF;//clear compare Flag write zero 
	#else
	cli();							// disable global int
	OCR1A=TCNT1+next_callback*2;	// set compare A for callback
	sei();							// enable global int
	TIFR1=(1<<OCF1A);				// clear compare A flag
	#endif	
	#endif
	BIND_BUTTON_FLAG_off;			// do not bind/reset id anymore even if protocol change
}

static void update_serial_data()
{
	RX_DONOTUPDTAE_on;
	RX_FLAG_off;								//data has been processed	
	if(rx_ok_buff[0]&0x20)						//check range
	RANGE_FLAG_on;
	else
	RANGE_FLAG_off;		
	if(rx_ok_buff[0]&0xC0)						//check autobind(0x40) & bind(0x80) together
	AUTOBIND_FLAG_on;
	else
	AUTOBIND_FLAG_off;
	if(rx_ok_buff[1]&0x80)						//if rx_ok_buff[1] ==1,power is low ,0-power high
	POWER_FLAG_off;	//power low
	else
	POWER_FLAG_on;	//power high
	
	option=rx_ok_buff[2];
	
	if( ((rx_ok_buff[0]&0x5F) != (cur_protocol[0]&0x5F)) || ( (rx_ok_buff[1]&0x7F) != cur_protocol[1] ) )
	{ // New model has been selected
		prev_protocol=cur_protocol[0]&0x1F;		//store previous protocol so we can reset the module
		cur_protocol[1] = rx_ok_buff[1]&0x7F;	//store current protocol
		CHANGE_PROTOCOL_FLAG_on;				//change protocol
		sub_protocol=(rx_ok_buff[1]>>4)& 0x07;					//subprotocol no (0-7) bits 4-6
		RX_num=rx_ok_buff[1]& 0x0F;
		MProtocol_id=MProtocol_id_master+RX_num;	//personalized RX bind + rx num // rx_num bits 0---3
	}
	else
	if( ((rx_ok_buff[0]&0x80)!=0) && ((cur_protocol[0]&0x80)==0) )	// Bind flag has been set
	CHANGE_PROTOCOL_FLAG_on;			//restart protocol with bind
	else
	cur_protocol[0] = rx_ok_buff[0];			//store current protocol
	
	// decode channel values
	volatile uint8_t *p=rx_ok_buff+2;
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
	#ifdef STM32_board
	//here code fro RX intrurpt disable
	USART3_BASE->CR1 &= ~ USART_CR1_RXNEIE;//disable 
	#else
	UCSR0B &= ~(1<<RXCIE0);	// RX interrupt disable
	#endif	
	#endif
	if(IS_RX_MISSED_BUFF_on)	// If the buffer is still valid
	{	memcpy((void*)rx_ok_buff,(const void*)rx_buff,RXBUFFER_SIZE);// Duplicate the buffer
		RX_FLAG_on;				// data to be processed next time...
		RX_MISSED_BUFF_off;
	}
	#ifdef XMEGA
	sei();
	#else
	#ifdef STM32_board
	//here code fro RX intrurpt enable
	USART3_BASE->CR1 |=  USART_CR1_RXNEIE ;//disable 
	#else
	UCSR0B |= (1<<RXCIE0) ;	// RX interrupt enable
	#endif	
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
	#ifdef	NFR24L01_INSTALLED
	NRF24L01_Reset();
	#endif

	//Wait for every component to reset
	delayMilliseconds(100);
	prev_power=0xFD;		// unused power value
}

#ifndef STM32_board
int16_t map( int16_t x, int16_t in_min, int16_t in_max, int16_t out_min, int16_t out_max)
{
	//  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
	long y ;
	x -= in_min ;
	y = out_max - out_min ;
	y *= x ;
	x = y / (in_max - in_min) ;
	return x  + out_min ;
}
#endif
// Channel value is converted to 8bit values full scale
uint8_t convert_channel_8b(uint8_t num)
{
	return (uint8_t) (map(limit_channel_100(num),servo_min_100,servo_max_100,0,255));	
}

// Channel value is converted to 8bit values to provided values scale
uint8_t convert_channel_8b_scale(uint8_t num,uint8_t min,uint8_t max)
{
	return (uint8_t) (map(limit_channel_100(num),servo_min_100,servo_max_100,min,max));	
}

// Channel value is converted sign + magnitude 8bit values
uint8_t convert_channel_s8b(uint8_t num)
{
	uint8_t ch;
	ch = convert_channel_8b(num);
	return (ch < 128 ? 127-ch : ch);	
}

// Channel value is converted to 10bit values
uint16_t convert_channel_10b(uint8_t num)
{
	return (uint16_t) (map(limit_channel_100(num),servo_min_100,servo_max_100,1,1023));
}

// Channel value is multiplied by 1.5
uint16_t convert_channel_frsky(uint8_t num)
{
	return Servo_data[num] + Servo_data[num]/2;
}

// Channel value is converted for HK310
void convert_channel_HK310(uint8_t num, uint8_t *low, uint8_t *high)
{
	uint16_t temp=0xFFFF-(4*Servo_data[num])/3;
	*low=(uint8_t)(temp&0xFF);
	*high=(uint8_t)(temp>>8);
}

// Channel value is limited to PPM_100
uint16_t limit_channel_100(uint8_t ch)
{
	if(Servo_data[ch]>servo_max_100)
	return servo_max_100;
	else
	if (Servo_data[ch]<servo_min_100)
	return servo_min_100;
	return Servo_data[ch];
}

//	void Serial_write(uint8_t data){
//		return;
//	}

static void Mprotocol_serial_init()
{
	
	#ifdef XMEGA
	
	PORTC.OUTSET = 0x08 ;
	PORTC.DIRSET = 0x08 ;
	
	USARTC0.BAUDCTRLA = 19 ;
	USARTC0.BAUDCTRLB = 0 ;
	
	USARTC0.CTRLB = 0x18 ;
	USARTC0.CTRLA = (USARTC0.CTRLA & 0xCF) | 0x10 ;
	USARTC0.CTRLC = 0x2B ;
	USARTC0.DATA ;
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
	UCSR0C = (1<<UPM01)|(1<<USBS0)|(1<<UCSZ01)|(1<<UCSZ00);
	while ( UCSR0A & (1 << RXC0) )//flush receive buffer
	UDR0;
	//enable reception and RC complete interrupt
	UCSR0B = (1<<RXEN0)|(1<<RXCIE0);//rx enable and interrupt
	#ifdef DEBUG_TX
	TX_SET_OUTPUT;
	#else
	#if defined(TELEMETRY)
	initTXSerial( SPEED_100K ) ;
	#endif //TELEMETRY
	#endif
	#endif
	#endif
}

#if defined(TELEMETRY)
void PPM_Telemetry_serial_init()
{
	initTXSerial( SPEED_9600 ) ;
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
	}
	else
	{ // if not generate a random ID
		randomSeed((uint32_t)analogRead(A6)<<10|analogRead(A7));//seed
		//
		id = random(0xfefefefe) + ((uint32_t)random(0xfefefefe) << 16);
		txid[0]=  (id &0xFF);
		txid[1] = ((id >> 8) & 0xFF);
		txid[2] = ((id >> 16) & 0xFF);
		txid[3] = ((id >> 24) & 0xFF);
		eeprom_write_block((const void*)txid,(void*)adress,4);
		eeprom_write_byte((uint8_t*)(adress+10),0xf0);//write bind flag in eeprom.
	}
	return id;
}

#endif

#ifndef XMEGA
#ifndef STM32_board 
/************************************/
/**  Arduino replacement routines  **/
/************************************/
// replacement millis() and micros()
// These work polled, no interrupts
// micros() MUST be called at least once every 32 milliseconds
uint16_t MillisPrecount ;
uint16_t lastTimerValue ;
uint32_t TotalMicros ;
uint32_t TotalMillis ;
uint8_t Correction ;

uint32_t micros()
{
	uint16_t elapsed ;
	uint8_t millisToAdd ;
	uint8_t oldSREG = SREG ;
	cli() ;
	uint16_t time = TCNT1 ;   // Read timer 1
	SREG = oldSREG ;

	elapsed = time - lastTimerValue ;
	elapsed += Correction ;
	Correction = elapsed & 0x01 ;
	elapsed >>= 1 ;

	uint32_t ltime = TotalMicros ;
	ltime += elapsed ;
	cli() ;
	TotalMicros = ltime ;   // Done this way for RPM to work correctly
	lastTimerValue = time ;
	SREG = oldSREG ;   // Still valid from above

	elapsed += MillisPrecount;
	millisToAdd = 0 ;

	if ( elapsed  > 15999 )
	{
		millisToAdd = 16 ;
		elapsed -= 16000 ;
	}
	if ( elapsed  > 7999 )
	{
		millisToAdd += 8 ;
		elapsed -= 8000 ;
	}
	if ( elapsed  > 3999 )
	{
		millisToAdd += 4 ;      
		elapsed -= 4000 ;
	}
	if ( elapsed  > 1999 )
	{
		millisToAdd += 2 ;
		elapsed -= 2000 ;
	}
	if ( elapsed  > 999 )
	{
		millisToAdd += 1 ;
		elapsed -= 1000 ;
	}
	TotalMillis += millisToAdd ;
	MillisPrecount = elapsed ;
	return TotalMicros ;
}

uint32_t millis()
{
	micros() ;
	return TotalMillis ;
}

void delayMilliseconds(unsigned long ms)
{
	uint16_t start = (uint16_t)micros();
	uint16_t lms = ms ;

	while (lms > 0) {
		if (((uint16_t)micros() - start) >= 1000) {
			lms--;
			start += 1000;
		}
	}
}

/* Important notes:
	- Max value is 16000µs
	- delay is not accurate due to interrupts happening */
void delayMicroseconds(unsigned int us)
{
	if (--us == 0)
	return;
	us <<= 2;	// * 4
	us -= 2;		// - 2
	__asm__ __volatile__ (
	"1: sbiw %0,1" "\n\t" // 2 cycles
	"brne 1b" : "=w" (us) : "0" (us) // 2 cycles
	);
}

void init()
{
	// this needs to be called before setup() or some functions won't work there
	sei();
}

#endif
#endif


/**************************/
/**************************/
/**  Interrupt routines  **/
/**************************/
/**************************/

//PPM
#ifdef ENABLE_PPM
#ifdef XMEGA
ISR(PORTD_INT0_vect)
#else
#ifdef STM32_board
void PPM_decode()	
#else		
ISR(INT1_vect)
#endif
#endif
{	// Interrupt on PPM pin
	static int8_t chan=-1;
	static uint16_t Prev_TCNT1=0;
	uint16_t Cur_TCNT1;
	
	#ifdef XMEGA
	Cur_TCNT1 = TCC1.CNT - Prev_TCNT1 ; // Capture current Timer1 value
	#else
	#if defined STM32_board
	uint16_t time=TIMER2_BASE->CNT;
	Cur_TCNT1=time-Prev_TCNT1; // Capture current Timer1 value			
	#else
	Cur_TCNT1=TCNT1-Prev_TCNT1; // Capture current Timer1 value
	#endif	
	#endif
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
		#endif
		sei();
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
					#ifdef XMEGA
					if(USARTC0.DATA==0x55)			// If 1st byte is 0x55 it looks ok
					
					#else
					#if defined STM32_board
					if(USART2_BASE->DR==0x55)						
					#else
					if(UDR0==0x55)			// If 1st byte is 0x55 it looks ok
					#endif			
					#endif
					{
						#ifdef XMEGA
						TCC1.CCB = TCC1.CNT+(6500L) ;		// Full message should be received within timer of 3250us
						TCC1.INTFLAGS = TC1_CCBIF_bm ;		// clear OCR1B match flag
						TCC1.INTCTRLB = (TCC1.INTCTRLB & 0xF3) | 0x04 ;	// enable interrupt on compare B match
						
						#else
						#if defined STM32_board
						uint16_t OCR1B = TIMER2_BASE->CNT;
						OCR1B +=6500L;
						timer.setCompare(TIMER_CH2,OCR1B);
						timer.attachCompare2Interrupt(ISR_COMPB);
						#else
						OCR1B=TCNT1+6500L;		// Full message should be received within timer of 3250us
						TIFR1=(1<<OCF1B);		// clear OCR1B match flag
						TIMSK1 |=(1<<OCIE1B);	// enable interrupt on compare B match
						#endif				
						#endif
						idx++;
					}
				}
				else
				{
					RX_MISSED_BUFF_off;					// if rx_buff was good it's not anymore...
					
					#ifdef XMEGA
					rx_buff[(idx++)-1]=USARTC0.DATA;	// Store received byte
					#else
					#if defined STM32_board
					
					rx_buff[(idx++)-1]=USART2_BASE->DR&0xff;			// Store received byte	
					#else						
					rx_buff[(idx++)-1]=UDR0;			// Store received byte
					#endif			
					#endif
					if(idx>RXBUFFER_SIZE)
					{	// A full frame has been received

						if(!IS_RX_DONOTUPDTAE_on)
						{ //Good frame received and main has finished with previous buffer
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
				#ifdef XMEGA
				idx = USARTC0.DATA ;	// Dummy read
				#else
				#if defined STM32_board					
				idx=USART2_BASE->DR&0xff;	
				#else					
				idx=UDR0;	// Dummy read
				#endif
				#endif
				discard_frame=1;		// Error encountered discard full frame...
			}
			
			if(discard_frame==1)
			{
				#ifdef XMEGA
				TCC1.INTCTRLB &=0xF3;			// disable interrupt on compare B match
				#else
				#if defined STM32_board
				detachInterrupt(2);//disable interrupt on ch2	
				#else							
				TIMSK1 &=~(1<<OCIE1B);			// disable interrupt on compare B match				
				#endif				
				#endif
				#ifndef STM32_board
				#ifndef XMEGA
				TX_RX_PAUSE_off;
				tx_resume();
				#endif
				#endif
			}
			#ifndef STM32_board)
			cli() ;
			UCSR0B |= _BV(RXCIE0) ;	// RX interrupt enable
			
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
	#ifdef XMEGA
	TCC1.INTCTRLB &=0xF3;			// Disable interrupt on compare B match
	#else
	#ifdef STM32_board
	detachInterrupt(2);//disable interrupt on ch2
	#else
	TIMSK1 &=~(1<<OCIE1B);			// Disable interrupt on compare B match
	#endif
	#endif
	#ifndef STM32_board
	tx_resume();
	#endif
}


#endif //ENABLE_SERIAL
