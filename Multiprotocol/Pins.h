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

// TX
#define SERIAL_TX_pin	1								//PD1
#define SERIAL_TX_port	PORTD
#define SERIAL_TX_ddr	DDRD
#define SERIAL_TX_output SERIAL_TX_ddr	|= _BV(SERIAL_TX_pin)
#define SERIAL_TX_on	SERIAL_TX_port |=  _BV(SERIAL_TX_pin)
#define SERIAL_TX_off	SERIAL_TX_port &= ~_BV(SERIAL_TX_pin)
#ifdef DEBUG_TX
	#define DEBUG_TX_on		SERIAL_TX_ON
	#define DEBUG_TX_off	SERIAL_TX_OFF
	#define DEBUG_TX_toggle	SERIAL_TX_port ^=  _BV(SERIAL_TX_pin)
#else
	#define DEBUG_TX_on
	#define DEBUG_TX_off
	#define DEBUG_TX_toggle
#endif

// Dial
#define MODE_DIAL1_pin	2
#define MODE_DIAL1_port	PORTB
#define MODE_DIAL1_ipr  PINB
#define MODE_DIAL2_pin	3
#define MODE_DIAL2_port	PORTB
#define MODE_DIAL2_ipr  PINB
#define MODE_DIAL3_pin	4
#define MODE_DIAL3_port	PORTB
#define MODE_DIAL3_ipr  PINB
#define MODE_DIAL4_pin	0
#define MODE_DIAL4_port	PORTC
#define MODE_DIAL4_ipr  PINC

// PPM
#define PPM_pin	 3										//D3 = PD3
#define PPM_port PORTD

// SDIO
#define SDI_pin	 5										//D5 = PD5
#define SDI_port PORTD
#define SDI_ipr  PIND
#define SDI_ddr  DDRD
#ifdef XMEGA
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
#ifdef XMEGA
	#define SDO_1 (SDO_port.IN & _BV(SDO_pin))
	#define SDO_0 (SDO_port.IN & _BV(SDO_pin)) == 0x00
#else
	#define SDO_1 (SDO_ipr & _BV(SDO_pin))
	#define SDO_0 (SDO_ipr & _BV(SDO_pin)) == 0x00
#endif

// SCLK
#define SCLK_port PORTD
#define SCLK_ddr DDRD
#ifdef XMEGA
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
#ifdef XMEGA
	#define CYRF_CSN_pin	4							//PD4
	#define CYRF_CSN_port	PORTD
	#define CYRF_CSN_ddr	DDRD
	#define CYRF_CSN_on		CYRF_CSN_port.OUTSET = _BV(CYRF_CSN_pin)
	#define CYRF_CSN_off	CYRF_CSN_port.OUTCLR = _BV(CYRF_CSN_pin)

	#define CYRF_RST_pin	0							//PE0
	#define CYRF_RST_port	PORTE
	#define CYRF_RST_ddr	DDRE
	#define CYRF_RST_HI		CYRF_RST_port.OUTSET |=  _BV(CYRF_RST_pin)
	#define CYRF_RST_LO		CYRF_RST_port.OUTCLR &= ~_BV(CYRF_RST_pin)
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
#ifdef XMEGA
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
#ifdef XMEGA
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

//BIND
#ifdef XMEGA
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
