
#ifndef _ExpressLRS_H_
#define _ExpressLRS_H_

#include "iface_SX1276.h"

#define One_Bit_Switches

extern uint8_t UID[6];
extern uint8_t CRCCaesarCipher;
extern uint8_t DeviceAddr;

extern uint8_t ExpressLRS_TXdataBuffer[8];
extern uint8_t ExpressLRS_NonceTX;

#define ELRS_RC_DATA_PACKET 0b00
#define ELRS_MSP_DATA_PACKET 0b01
#define ELRS_TLM_PACKET 0b11
#define ELRS_SYNC_PACKET 0b10

#define ELRS_RC_DATA_PACKET 0b00
#define ELRS_MSP_DATA_PACKET 0b01
#define ELRS_TLM_PACKET 0b11
#define ELRS_SYNC_PACKET 0b10

typedef enum
{
    RATE_200HZ = 2,
    RATE_100HZ = 4,
    RATE_50HZ = 5,
} expresslrs_RFrates_e; // Max value of 16 since only 4 bits have been assigned in the sync package.

#define RATE_MAX 3


typedef struct expresslrs_mod_settings_s
{
    uint8_t index;
    expresslrs_RFrates_e enum_rate; // Max value of 16 since only 4 bits have been assigned in the sync package.
    uint8_t bw;
    uint8_t sf;
    uint8_t cr;
    uint32_t interval;                  //interval in us seconds that corresponds to that frequnecy
    uint8_t FHSShopInterval;            // every X packets we hope to a new frequnecy. Max value of 16 since only 4 bits have been assigned in the sync package.
    uint8_t PreambleLen;
    uint32_t SyncPktInterval;

} expresslrs_mod_settings_t;


extern expresslrs_mod_settings_s ExpressLRS_AirRateConfig[RATE_MAX];

expresslrs_mod_settings_s *get_elrs_airRateConfig(int8_t index)
{
  if (index < 0) // Protect against out of bounds rate
  {
    return &ExpressLRS_AirRateConfig[0]; // Set to first entry in the array (200HZ)
  }
  else if (index > (RATE_MAX - 1))
  {
    return &ExpressLRS_AirRateConfig[RATE_MAX - 1]; // Set to last usable entry in the array (currently 50HZ)
  }
  return &ExpressLRS_AirRateConfig[index];
}

extern expresslrs_mod_settings_s *ExpressLRS_currAirRate_Modparams;

void ExpressLRS_SetRFLinkRate(uint8_t rate);

//////////////////////////////////////////// RNG ROUTINES ///////////////////////////////////////////

#define RNG_MAX 0x7FFF
extern unsigned long ExpressLRS_seed;

int32_t expresslrs_rng(void); // returns values between 0 and 0x7FFF, NB rngN depends on this output range, so if we change the behaviour rngN will need updating
void expresslrs_rngSeed(long newSeed); // returns 0 <= x < max where max <= 256, (actual upper limit is higher, but there is one and I haven't thought carefully about what it is)
unsigned int expresslrs_rngN(unsigned int max);

long expresslrs_rng8Bit(void);
long expresslrs_rng5Bit(void);
long expresslrs_rng0to2(void);

//////////////////////////////////////////// FHSS ROUTINES ///////////////////////////////////////////

extern uint32_t expresslrs_fhss_start_freq;
extern uint32_t expresslrs_fhss_interval;
extern uint32_t expresslrs_fhss_num_freqs;

extern uint8_t expresslrs_fhss_ptr; //value of current index in expresslrs_fhss_sequence array

#define NR_SEQUENCE_ENTRIES 256 // length of peseudo random hop seq
extern uint8_t expresslrs_fhss_sequence[NR_SEQUENCE_ENTRIES];

void expresslrs_fhss_init_freq(uint8_t regulatory_domain);

/* 868 EU Frequency bands taken from https://wetten.overheid.nl/BWBR0036378/2016-12-28#Bijlagen
 * Note: these frequencies fall in the license free H-band, but in combination with 500kHz 
 * LoRa modem bandwidth used by ExpressLRS (EU allows up to 125kHz modulation BW only) they
 * will never pass RED certification and they are ILLEGAL to use. USE at your own risk 
 * 
 * Therefore we simply maximize the usage of available spectrum so laboratory testing of the software won't disturb existing
 * 868MHz ISM band traffic too much.

    863275000, // band H1, 863 - 865MHz, 0.1% duty cycle or CSMA techniques, 25mW EIRP
    863800000,
    864325000,
    864850000,
    865375000, // Band H2, 865 - 868.6MHz, 1.0% dutycycle or CSMA, 25mW EIRP
    865900000,
    866425000,
    866950000,
    867475000,
    868000000,
    868525000, // Band H3, 868.7-869.2MHz, 0.1% dutycycle or CSMA, 25mW EIRP
    869050000,
    869575000};
*/

uint32_t expresslrs_fhss_get_array_val(uint8_t index);
uint8_t expresslrs_fhss_get_index();      // get the current index of the FHSS pointer
uint32_t expresslrs_fhss_inital_freq();   // get inital freq (index 0)
uint32_t expresslrs_fhss_get_curr_freq(); // return curr freq
uint32_t expresslrs_fhss_get_next_freq(); // incriment pointer and return new freq

/**
Requirements for freq sequence generator:
1. 0 every n hops
2. No two repeated channels
3. Equal occurance of each (or as even as possible) of each channel
4. Pesudorandom

Approach:
  Initialise an array of flags indicating which channels have not yet been assigned and a counter of how many channels are available
  Iterate over the FHSSsequence array using index
    if index is a multiple of SYNC_INTERVAL assign the sync channel index (0)
    otherwise, generate a random number between 0 and the number of channels left to be assigned
    find the index of the nth remaining channel
    if the index is a repeat, generate a new random number
    if the index is not a repeat, assing it to the FHSSsequence array, clear the availability flag and decrement the available count
    if there are no available channels left, reset the flags array and the count
*/

// Set all of the flags in the array to true, except for the first one
// which corresponds to the sync channel and is never available for normal expresslrs_fhss_calc_reset_available
// allocation.

void expresslrs_fhss_calc_reset_available(uint8_t *array);
void expresslrs_fhss_generate_sequence();
uint8_t expresslrs_crc(uint8_t *data, int length);

//////////////////////////////////////////// CRC ROUTINES ///////////////////////////////////////////

extern const uint8_t expresslrs_crc8tab[256]; /* CRC8 implementation with polynom = x​7​+ x​6​+ x​4​+ x​2​+ x​0 ​(0xD5) */ //from betaflight: https://github.com/betaflight/betaflight/blob/c8b5edb415c33916c91a7ccc8bd19c7276540cd1/src/test/unit/rx_crsf_unittest.cc#L65

#endif
