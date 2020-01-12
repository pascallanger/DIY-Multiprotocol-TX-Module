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
//*******************
//***   Pinouts   ***
//*******************
#ifndef STM32_BOARD
	// TX
	#define SERIAL_TX_pin	1								//PD1
	#define SERIAL_TX_port	PORTD
	#define SERIAL_TX_ddr	DDRD
	#define SERIAL_TX_output SERIAL_TX_ddr	|= _BV(SERIAL_TX_pin)
	#define SERIAL_TX_on	SERIAL_TX_port |=  _BV(SERIAL_TX_pin)
	#define SERIAL_TX_off	SERIAL_TX_port &= ~_BV(SERIAL_TX_pin)
	#ifdef DEBUG_PIN
		#define DEBUG_PIN_on		SERIAL_TX_on
		#define DEBUG_PIN_off		SERIAL_TX_off
		#define DEBUG_PIN_toggle	SERIAL_TX_port ^=  _BV(SERIAL_TX_pin)
	#else
		#define DEBUG_PIN_on
		#define DEBUG_PIN_off
		#define DEBUG_PIN_toggle
	#endif

	// Dial
	#define PROTO_DIAL1_pin	2
	#define PROTO_DIAL1_port	PORTB
	#define PROTO_DIAL1_ipr  PINB
	#define PROTO_DIAL2_pin	3
	#define PROTO_DIAL2_port	PORTB
	#define PROTO_DIAL2_ipr  PINB
	#define PROTO_DIAL3_pin	4
	#define PROTO_DIAL3_port	PORTB
	#define PROTO_DIAL3_ipr  PINB
	#define PROTO_DIAL4_pin	0
	#define PROTO_DIAL4_port	PORTC
	#define PROTO_DIAL4_ipr  PINC

	// PPM
	#define PPM_pin	 3										//D3 = PD3
	#define PPM_port PORTD

	// SDIO
	#define SDI_pin	 5										//D5 = PD5
	#define SDI_port PORTD
	#define SDI_ipr  PIND
	#define SDI_ddr  DDRD
	#ifdef ORANGE_TX
		#define SDI_on	SDI_port.OUTSET = _BV(SDI_pin)
		#define SDI_off SDI_port.OUTCLR = _BV(SDI_pin)
	#else
		#define SDI_on	SDI_port |= _BV(SDI_pin)
		#define SDI_off	SDI_port &= ~_BV(SDI_pin)
		#define SDI_1	(SDI_ipr & _BV(SDI_pin))
		#define SDI_0	(SDI_ipr & _BV(SDI_pin)) == 0x00
	#endif
	#define SDI_input	SDI_ddr &= ~_BV(SDI_pin)
	#define SDI_output	SDI_ddr |=  _BV(SDI_pin)

	//SDO
	#define SDO_pin		6									//D6 = PD6
	#define SDO_port	PORTD
	#define SDO_ipr		PIND
	#ifdef ORANGE_TX
		#define SDO_1 (SDO_port.IN & _BV(SDO_pin))
		#define SDO_0 (SDO_port.IN & _BV(SDO_pin)) == 0x00
	#else
		#define SDO_1 (SDO_ipr & _BV(SDO_pin))
		#define SDO_0 (SDO_ipr & _BV(SDO_pin)) == 0x00
	#endif

	// SCLK
	#define SCLK_port PORTD
	#define SCLK_ddr DDRD
	#ifdef ORANGE_TX
		#define SCLK_pin	7								//PD7
		#define SCLK_on		SCLK_port.OUTSET = _BV(SCLK_pin)
		#define SCLK_off	SCLK_port.OUTCLR = _BV(SCLK_pin)
	#else
		#define SCLK_pin	4								//D4 = PD4
		#define SCLK_output	SCLK_ddr  |=  _BV(SCLK_pin)
		#define SCLK_on		SCLK_port |=  _BV(SCLK_pin)
		#define SCLK_off	SCLK_port &= ~_BV(SCLK_pin)
	#endif

	// A7105
	#define A7105_CSN_pin	2								//D2 = PD2
	#define A7105_CSN_port	PORTD
	#define A7105_CSN_ddr	DDRD
	#define A7105_CSN_output	A7105_CSN_ddr |= _BV(A7105_CSN_pin)
	#define A7105_CSN_on	A7105_CSN_port |=  _BV(A7105_CSN_pin)
	#define A7105_CSN_off	A7105_CSN_port &= ~_BV(A7105_CSN_pin)

	// CC2500
	#define CC25_CSN_pin	7								//D7 = PD7
	#define CC25_CSN_port	PORTD
	#define CC25_CSN_ddr	DDRD
	#define CC25_CSN_output	CC25_CSN_ddr  |=  _BV(CC25_CSN_pin)
	#define CC25_CSN_on		CC25_CSN_port |=  _BV(CC25_CSN_pin)
	#define CC25_CSN_off	CC25_CSN_port &= ~_BV(CC25_CSN_pin)

	// NRF24L01
	#define NRF_CSN_pin		0								//D8 = PB0
	#define NRF_CSN_port	PORTB
	#define NRF_CSN_ddr		DDRB
	#define NRF_CSN_output	NRF_CSN_ddr  |=  _BV(NRF_CSN_pin)
	#define NRF_CSN_on		NRF_CSN_port |=  _BV(NRF_CSN_pin)
	#define NRF_CSN_off		NRF_CSN_port &= ~_BV(NRF_CSN_pin)
	#define NRF_CE_on
	#define NRF_CE_off

	// CYRF6936
	#ifdef ORANGE_TX
		#define CYRF_CSN_pin	4							//PD4
		#define CYRF_CSN_port	PORTD
		#define CYRF_CSN_ddr	DDRD
		#define CYRF_CSN_on		CYRF_CSN_port.OUTSET = _BV(CYRF_CSN_pin)
		#define CYRF_CSN_off	CYRF_CSN_port.OUTCLR = _BV(CYRF_CSN_pin)

		#define CYRF_RST_pin	0							//PE0
		#define CYRF_RST_port	PORTE
		#define CYRF_RST_ddr	DDRE
		#define CYRF_RST_HI		CYRF_RST_port.OUTSET = _BV(CYRF_RST_pin)
		#define CYRF_RST_LO		CYRF_RST_port.OUTCLR = _BV(CYRF_RST_pin)
	#else
		#define CYRF_CSN_pin	1							//D9 = PB1
		#define CYRF_CSN_port	PORTB
		#define CYRF_CSN_ddr	DDRB
		#define CYRF_CSN_output	CYRF_CSN_ddr  |=  _BV(CYRF_CSN_pin)
		#define CYRF_CSN_on		CYRF_CSN_port |=  _BV(CYRF_CSN_pin)
		#define CYRF_CSN_off	CYRF_CSN_port &= ~_BV(CYRF_CSN_pin)

		#define CYRF_RST_pin	5							//A5 = PC5
		#define CYRF_RST_port	PORTC
		#define CYRF_RST_ddr	DDRC
		#define CYRF_RST_output	CYRF_RST_ddr  |=  _BV(CYRF_RST_pin)
		#define CYRF_RST_HI		CYRF_RST_port |=  _BV(CYRF_RST_pin)
		#define CYRF_RST_LO		CYRF_RST_port &= ~_BV(CYRF_RST_pin)
	#endif

	//RF Switch
	#ifdef ORANGE_TX
		#define PE1_on
		#define PE1_off
		#define PE2_on
		#define PE2_off
	#else
		#define PE1_pin		1								//A1 = PC1
		#define PE1_port	PORTC
		#define PE1_ddr		DDRC
		#define	PE1_output	PE1_ddr  |=  _BV(PE1_pin)
		#define PE1_on		PE1_port |=  _BV(PE1_pin)
		#define PE1_off		PE1_port &= ~_BV(PE1_pin)

		#define PE2_pin		2								//A2 = PC2
		#define PE2_port	PORTC
		#define PE2_ddr		DDRC
		#define	PE2_output	PE2_ddr  |=  _BV(PE2_pin)
		#define PE2_on		PE2_port |=  _BV(PE2_pin)
		#define PE2_off		PE2_port &= ~_BV(PE2_pin)
	#endif

	// LED
	#ifdef ORANGE_TX
		#define LED_pin		1								//PD1
		#define LED_port	PORTD
		#define LED_ddr		DDRD
		#define LED_on		LED_port.OUTCLR	= _BV(LED_pin)
		#define LED_off		LED_port.OUTSET	= _BV(LED_pin)
		#define LED_toggle	LED_port.OUTTGL	= _BV(LED_pin)
		#define LED_output	LED_port.DIRSET	= _BV(LED_pin)
		#define IS_LED_on	(LED_port.OUT & _BV(LED_pin))
	#else
		#define LED_pin		5								//D13 = PB5
		#define LED_port	PORTB
		#define LED_ddr		DDRB
		#define LED_on		LED_port |= _BV(LED_pin)
		#define LED_off		LED_port &= ~_BV(LED_pin)
		#define LED_toggle	LED_port ^= _BV(LED_pin)
		#define LED_output	LED_ddr  |= _BV(LED_pin)
		#define IS_LED_on	(LED_port & _BV(LED_pin))
	#endif

	#define	LED2_on
	#define	LED2_off
	#define	LED2_toggle
	#define	LED2_output
	#define	IS_LED2_on		0

	//BIND
	#ifdef ORANGE_TX
		#define BIND_pin			2						//PD2
		#define BIND_port			PORTD
		#define IS_BIND_BUTTON_on	( (BIND_port.IN & _BV(BIND_pin)) == 0x00 )
	#else
		#define BIND_pin			5						//D13 = PB5
		#define BIND_port			PORTB
		#define BIND_ipr			PINB
		#define BIND_ddr			DDRB
		#define BIND_SET_INPUT		BIND_ddr &= ~_BV(BIND_pin)
		#define BIND_SET_OUTPUT		BIND_ddr |=  _BV(BIND_pin)
		#define BIND_SET_PULLUP		BIND_port |= _BV(BIND_pin)
		#define IS_BIND_BUTTON_on	( (BIND_ipr & _BV(BIND_pin)) == 0x00 )
	#endif
#else //STM32_BOARD
	#define	BIND_pin		PA0
	#define	LED_pin			PA1
	#define	LED2_pin		PA2
	//
	#define	PPM_pin			PA8								//PPM  5V tolerant
	//
	#define	S1_pin			PA4								//Dial switch pins
	#define	S2_pin			PA5
	#define	S3_pin			PA6
	#define	S4_pin			PA7
	//
	#define	PE1_pin			PB4								//PE1
	#define	PE2_pin			PB5								//PE2
	//CS pins
	#define	CC25_CSN_pin	PB6								//CC2500
	#define	NRF_CSN_pin		PB7								//NRF24L01
	#define	CYRF_RST_pin	PB8								//CYRF RESET
	#define	A7105_CSN_pin	PB9								//A7105
	#define	CYRF_CSN_pin	PB12							//CYRF CSN
	#define SPI_CSN_pin		PA15
	//SPI pins	
	#define	SCK_pin			PB13							//SCK
	#define	SDO_pin			PB14							//MISO
	#define	SDI_pin			PB15							//MOSI
	//
	#define	TX_INV_pin		PB3
	#define	RX_INV_pin		PB1
	//
	#define	PE1_on  		digitalWrite(PE1_pin,HIGH)
	#define	PE1_off		 	digitalWrite(PE1_pin,LOW)
	//
	#define	PE2_on  		digitalWrite(PE2_pin,HIGH)
	#define	PE2_off 		digitalWrite(PE2_pin,LOW)

	#define	A7105_CSN_on	digitalWrite(A7105_CSN_pin,HIGH)
	#define	A7105_CSN_off	digitalWrite(A7105_CSN_pin,LOW)

	#define NRF_CE_on
	#define	NRF_CE_off

	#define	SCK_on			digitalWrite(SCK_pin,HIGH)
	#define	SCK_off			digitalWrite(SCK_pin,LOW)

	#define	SDI_on			digitalWrite(SDI_pin,HIGH)
	#define	SDI_off			digitalWrite(SDI_pin,LOW)

	#define	SDI_1			(digitalRead(SDI_pin)==HIGH)
	#define	SDI_0			(digitalRead(SDI_pin)==LOW)

	#define	CC25_CSN_on		digitalWrite(CC25_CSN_pin,HIGH)
	#define	CC25_CSN_off	digitalWrite(CC25_CSN_pin,LOW)

	#define	NRF_CSN_on		digitalWrite(NRF_CSN_pin,HIGH)
	#define	NRF_CSN_off		digitalWrite(NRF_CSN_pin,LOW)

	#define	CYRF_CSN_on		digitalWrite(CYRF_CSN_pin,HIGH)
	#define	CYRF_CSN_off	digitalWrite(CYRF_CSN_pin,LOW)

	#define	SPI_CSN_on		digitalWrite(SPI_CSN_pin,HIGH)
	#define	SPI_CSN_off		digitalWrite(SPI_CSN_pin,LOW)

	#define	CYRF_RST_HI		digitalWrite(CYRF_RST_pin,HIGH)	//reset cyrf
	#define	CYRF_RST_LO		digitalWrite(CYRF_RST_pin,LOW)	//

	#define	SDO_1			(digitalRead(SDO_pin)==HIGH)
	#define	SDO_0			(digitalRead(SDO_pin)==LOW)

	#define	TX_INV_on		digitalWrite(TX_INV_pin,HIGH)
	#define	TX_INV_off		digitalWrite(TX_INV_pin,LOW)

	#define	RX_INV_on		digitalWrite(RX_INV_pin,HIGH)
	#define	RX_INV_off		digitalWrite(RX_INV_pin,LOW)

	#define	LED_on			digitalWrite(LED_pin,HIGH)
	#define	LED_off			digitalWrite(LED_pin,LOW)
	#define	LED_toggle		digitalWrite(LED_pin ,!digitalRead(LED_pin))
	#define	LED_output		pinMode(LED_pin,OUTPUT)
	#define	IS_LED_on		( digitalRead(LED_pin)==HIGH)

	//iRangeX modules have a second LED
	#define	LED2_on			digitalWrite(LED2_pin,HIGH)
	#define	LED2_off		digitalWrite(LED2_pin,LOW)
	#define	LED2_toggle		digitalWrite(LED2_pin ,!digitalRead(LED2_pin))
	#define	LED2_output		pinMode(LED2_pin,OUTPUT)
	#define	IS_LED2_on		( digitalRead(LED2_pin)==HIGH)

	#define BIND_SET_INPUT		pinMode(BIND_pin,INPUT)
	#define BIND_SET_PULLUP		digitalWrite(BIND_pin,HIGH)	
	#define BIND_SET_OUTPUT		pinMode(BIND_pin,OUTPUT)
	#define IS_BIND_BUTTON_on	(digitalRead(BIND_pin)==LOW)

	#ifdef DEBUG_PIN
		#define DEBUG_PIN_on		digitalWrite(SPI_CSN_pin,HIGH)
		#define DEBUG_PIN_off		digitalWrite(SPI_CSN_pin,LOW)
		#define DEBUG_PIN_toggle	digitalWrite(SPI_CSN_pin,!digitalRead(SPI_CSN_pin))
	#else
		#define DEBUG_PIN_on
		#define DEBUG_PIN_off
		#define DEBUG_PIN_toggle
	#endif

	#define	cli() 			noInterrupts()
	#define	sei() 			interrupts()
	#define	delayMilliseconds(x) delay(x)
#endif

//*******************
//***    Timer    ***
//*******************
#ifdef ORANGE_TX
	#define TIFR1 TCC1.INTFLAGS
	#define OCF1A_bm TC1_CCAIF_bm
	#define OCR1A TCC1.CCA
	#define TCNT1 TCC1.CNT
	#define UDR0 USARTC0.DATA
	#define OCF1B_bm TC1_CCBIF_bm
	#define OCR1B TCC1.CCB
	#define TIMSK1 TCC1.INTCTRLB
	#define SET_TIMSK1_OCIE1B	TIMSK1  = (TIMSK1 & 0xF3) | 0x04
	#define CLR_TIMSK1_OCIE1B	TIMSK1 &= 0xF3
#else
	#ifdef STM32_BOARD
		#define OCR1A TIMER2_BASE->CCR1
		#define TCNT1 TIMER2_BASE->CNT
		#define TIFR1 TIMER2_BASE->SR
		#define OCF1A_bm TIMER_SR_CC1IF
		#define UDR0 USART2_BASE->DR
		#define UCSR0B USART2_BASE->CR1
		#define RXCIE0 USART_CR1_RXNEIE_BIT
		#define TXCIE0 USART_CR1_TXEIE_BIT
		//#define TIFR1 TIMER2_BASE->SR
	#else
		#define OCF1A_bm _BV(OCF1A)
		#define OCF1B_bm _BV(OCF1B)
		#define SET_TIMSK1_OCIE1B	TIMSK1 |= _BV(OCIE1B)
		#define CLR_TIMSK1_OCIE1B	TIMSK1 &=~_BV(OCIE1B)
	#endif
#endif

//*******************
//***    EEPROM   ***
//*******************
#ifdef STM32_BOARD
	#define EE_ADDR uint16
	#define eeprom_write_byte EEPROM.write
	#define eeprom_read_byte EEPROM.read
#else
	#define EE_ADDR uint8_t*
#endif