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
 // Compatible with EAchine H8 mini, H10, BayangToys X6/X7/X9, JJRC JJ850 ...
 // Last sync with hexfet new_protocols/bayang_nrf24l01.c dated 2015-12-22

#if defined(BAYANG_NRF24L01_INO)

#include "iface_nrf24l01.h"

uint16_t Bayang_Rx_callback()
{
	return 1000;
}

uint16_t initBayang_Rx()
{
	return 1000;
}


#endif

