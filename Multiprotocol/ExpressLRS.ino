#if defined(ELRS_SX1276_INO) && defined(SX1276_INSTALLED)

#include "iface_sx1276.h"
#include "ExpressLRS.h"

uint8_t UID[6] = {10, 183, 12, 124, 56, 3}; // default UID for ExpressLRS (for testing)

uint8_t CRCCaesarCipher = UID[4];
uint8_t DeviceAddr = 0b111111 & UID[5];

uint8_t expresslrs_tx_data_buf[8] = {0};
uint8_t expresslrs_nonce_tx = 0;
uint32_t SyncPacketLastSent = 0;

expresslrs_mod_settings_s *ExpressLRS_currAirRate_Modparams = &ExpressLRS_AirRateConfig[0];

void expresslrs_dio0_isr()
{
  expresslrs_nonce_tx++;
  SX1276_ClearIRQFlags();
  expresslrs_fhss_handle();
}

uint16_t initExpressLRS()
{
  debugln("Setting ExpressLRS LoRa reg defaults");
  SX1276_Reset();
  SX1276_SetMode(true, false, SX1276_OPMODE_SLEEP);
  SX1276_SetMode(true, false, SX1276_OPMODE_STDBY);
  //SX1276_DetectChip(); // for debug

#define expresslrs_num_rf_rates 3
#define expresslrs_num_reg_domains 3

  uint8_t reg_domain = int(sub_protocol / expresslrs_num_reg_domains);
  uint8_t rf_rate = sub_protocol % expresslrs_num_rf_rates;

  expresslrs_fhss_init_freq(reg_domain);
  expresslrs_ota_set_rfrate((expresslrs_RFrates_e)rf_rate);

  debugln("%s%d", "Reg Domain: ", reg_domain);
  debugln("%s%d", "RFrate:     ", rf_rate);
  debugln("%s%d", "Interval: ", ExpressLRS_currAirRate_Modparams->interval);

  SX1276_SetSyncWord(UID[3]);
  SX1276_WriteReg(SX1276_22_PAYLOAD_LENGTH, 8);
  SX1276_WriteReg(SX1276_40_DIOMAPPING1, 0b11000000); //undocumented "hack", looking at Table 18 from datasheet SX127X_REG_DIO_MAPPING_1 = 11 appears to be unspported by infact it generates an intterupt on both RXdone and TXdone, this saves switching modes.
  SX1276_SetHopPeriod(0);                             // 0 = disabled, we hop frequencies manually
  SX1276_SetPaDac(true);
  SX1276_SetPaConfig(true, 7, 0);
  SX1276_SetTxRxMode(TX_EN);

  SX1276_SetFrequency_expresslrs(expresslrs_fhss_inital_freq());

  attachInterrupt(SX1276_DIO0_pin, expresslrs_dio0_isr, RISING); //attache expresslrs_dio0_isr() func

  return ExpressLRS_currAirRate_Modparams->interval;
}

uint16_t ExpressLRS_callback()
{
  expresslrs_switch_update_vals();
  expresslrs_build_ota_pkt();
  return ExpressLRS_currAirRate_Modparams->interval;
}

///////////////////////////////////////////////////////////////////// OTA Func /////////////////////////////////////////////////////////////////////

static inline uint8_t UINT11_to_BIT(uint16_t Val)
{
  if (Val > 1024)
    return 1;
  else
    return 0;
};

void expresslrs_fhss_handle()
{
  uint8_t modresult = (expresslrs_nonce_tx) % ExpressLRS_currAirRate_Modparams->FHSShopInterval;

  if (modresult == 0) // if it time to hop, do so.
  {
    SX1276_SetFrequency_expresslrs(expresslrs_fhss_get_next_freq());
  }
}

// TODO, for now we just impliment the hybrid switch mode 
// Hybrid switches
// The first switch is treated as a low-latency switch to be used for arm/disarm.
// It is sent with every packet. The remaining 7 switch channels are sent
// in the same change-prioritized round-robin fashion as described for sequential
// switches above. All switches are encoded for 3 position support. All analog
// channels are reduced to 10bit resolution to free up space in the rc packet
// for switches.

// current and sent switch values, used for prioritising sequential switch transmission
#define N_SWITCHES 8
uint8_t currentSwitches[N_SWITCHES] = {0};
uint8_t sentSwitches[N_SWITCHES] = {0};
uint8_t nextSwitchIndex = 0; // for round-robin sequential switches

void expresslrs_switch_update_vals()
{
#define INPUT_RANGE 2048
  const uint16_t SWITCH_DIVISOR = INPUT_RANGE / 3; // input is 0 - 2048
  for (int i = 0; i < N_SWITCHES; i++)
  {
    currentSwitches[i] = (Channel_data[i + 4] - 32) / SWITCH_DIVISOR;
  }
}

void expresslrs_switch_just_sent(uint8_t index, uint8_t value)
{
  sentSwitches[index] = value;
}

uint8_t expresslrs_switch_next_index()
{
  int firstSwitch = 0; // sequential switches includes switch 0

  firstSwitch = 1; // skip 0 since it is sent on every packet

  // look for a changed switch
  int i;
  for (i = firstSwitch; i < N_SWITCHES; i++)
  {
    if (currentSwitches[i] != sentSwitches[i])
      break;
  }
  // if we didn't find a changed switch, we get here with i==N_SWITCHES
  if (i == N_SWITCHES)
  {
    i = nextSwitchIndex;
  }

  // keep track of which switch to send next if there are no changed switches
  // during the next call.
  nextSwitchIndex = (i + 1) % 8;

  // for hydrid switches 0 is sent on every packet, so we can skip
  // that value for the round-robin
  if (nextSwitchIndex == 0)
  {
    nextSwitchIndex = 1;
  }

  return i;
}

void expresslrs_build_ota_hybrid_switches(uint8_t *Buffer, uint8_t addr)
{
  uint8_t PacketHeaderAddr;
  PacketHeaderAddr = (addr << 2) + ELRS_RC_DATA_PACKET;
  Buffer[0] = PacketHeaderAddr;
  Buffer[1] = ((Channel_data[0] - 32) >> 3);
  Buffer[2] = ((Channel_data[1] - 32) >> 3);
  Buffer[3] = ((Channel_data[2] - 32) >> 3);
  Buffer[4] = ((Channel_data[3] - 32) >> 3);
  Buffer[5] = (((Channel_data[0] - 32) & 0b110) << 5) +
              (((Channel_data[1] - 32) & 0b110) << 3) +
              (((Channel_data[2] - 32) & 0b110) << 1) +
              (((Channel_data[3] - 32) & 0b110) >> 1);

  // switch 0 is sent on every packet - intended for low latency arm/disarm
  Buffer[6] = (currentSwitches[0] & 0b11) << 5; // note this leaves the top bit of byte 6 unused

  // find the next switch to send
  uint8_t nextSwitchIndex = expresslrs_switch_next_index() & 0b111;  // mask for paranoia
  uint8_t value = currentSwitches[nextSwitchIndex] & 0b11; // mask for paranoia

  // put the bits into buf[6]. nextSwitchIndex is in the range 1 through 7 so takes 3 bits
  // currentSwitches[nextSwitchIndex] is in the range 0 through 2, takes 2 bits.
  Buffer[6] += (nextSwitchIndex << 2) + value;

  // update the sent value
  expresslrs_switch_just_sent(nextSwitchIndex, value);
}

void ExpressLRS_GenerateSyncPacketData()
{
  uint8_t PacketHeaderAddr;
  PacketHeaderAddr = (DeviceAddr << 2) + ELRS_SYNC_PACKET;
  expresslrs_tx_data_buf[0] = PacketHeaderAddr;
  expresslrs_tx_data_buf[1] = expresslrs_fhss_get_index();
  expresslrs_tx_data_buf[2] = expresslrs_nonce_tx;
  expresslrs_tx_data_buf[3] = ((ExpressLRS_currAirRate_Modparams->index & 0b111) << 5) + (0b000 << 2); // disable TLM
  expresslrs_tx_data_buf[4] = UID[3];
  expresslrs_tx_data_buf[5] = UID[4];
  expresslrs_tx_data_buf[6] = UID[5];
}

void expresslrs_build_ota_11bit_switches()
{
  uint8_t PacketHeaderAddr;
  PacketHeaderAddr = (DeviceAddr << 2) + ELRS_RC_DATA_PACKET;
  expresslrs_tx_data_buf[0] = PacketHeaderAddr;
  expresslrs_tx_data_buf[1] = ((Channel_data[0] - 32) >> 3); //- 32 is needed to make the scales correct with respect to the multi channel scaling
  expresslrs_tx_data_buf[2] = ((Channel_data[1] - 32) >> 3);
  expresslrs_tx_data_buf[3] = ((Channel_data[2] - 32) >> 3);
  expresslrs_tx_data_buf[4] = ((Channel_data[3] - 32) >> 3);
  expresslrs_tx_data_buf[5] = (((Channel_data[0] - 32) & 0b00000111) << 5) +
                              (((Channel_data[1] - 32) & 0b111) << 2) +
                              (((Channel_data[2] - 32) & 0b110) >> 1);
  expresslrs_tx_data_buf[6] = (((Channel_data[2] - 32) & 0b001) << 7) +
                              (((Channel_data[3] - 32) & 0b111) << 4); // 4 bits left over for something else?
#ifdef One_Bit_Switches
  expresslrs_tx_data_buf[6] += UINT11_to_BIT(Channel_data[4] - 32) << 3;
  expresslrs_tx_data_buf[6] += UINT11_to_BIT(Channel_data[5] - 32) << 2;
  expresslrs_tx_data_buf[6] += UINT11_to_BIT(Channel_data[6] - 32) << 1;
  expresslrs_tx_data_buf[6] += UINT11_to_BIT(Channel_data[7] - 32) << 0;
#endif
}

void expresslrs_build_ota_pkt()
{
#ifdef MULTI_SYNC
  //TODO
#endif

  if ((millis() > (SyncPacketLastSent + ExpressLRS_currAirRate_Modparams->SyncPktInterval)) && (expresslrs_fhss_get_curr_freq() == expresslrs_fhss_inital_freq()) && ((expresslrs_nonce_tx) % ExpressLRS_currAirRate_Modparams->FHSShopInterval == 1)) // sync just after we changed freqs (helps with hwTimer.init() being in sync from the get go)
  {
    ExpressLRS_GenerateSyncPacketData();
    SyncPacketLastSent = millis();
  }
  else
  {
    expresslrs_build_ota_hybrid_switches(expresslrs_tx_data_buf, DeviceAddr); // TODO impliment diff switch modes 
    // #if defined HYBRID_SWITCHES_8
    //     expresslrs_build_ota_hybrid_switches(&Radio, &crsf, DeviceAddr);
    // #elif defined SEQ_SWITCHES
    //     GenerateChannelDataSeqSwitch(&Radio, &crsf, DeviceAddr);
    // #else
    //     expresslrs_build_ota_11bit_switches();
    // #endif
  }
  ///// Next, Calculate the CRC and put it into the buffer /////
  uint8_t crc = expresslrs_crc(expresslrs_tx_data_buf, 7) + CRCCaesarCipher;
  expresslrs_tx_data_buf[7] = crc;
  SX1276_WritePayloadToFifo(expresslrs_tx_data_buf, 8);
  SX1276_SetMode(true, false, SX1276_OPMODE_TX);
}

///////////////////////////////////////////////////////////////////// RF Params /////////////////////////////////////////////////////////////////////

expresslrs_mod_settings_s ExpressLRS_AirRateConfig[RATE_MAX] = {
    {0, RATE_200HZ, SX1276_MODEM_CONFIG1_BW_500KHZ, 6, SX1276_MODEM_CONFIG1_CODING_RATE_4_7, 5000, 2, 8, 2000},
    {1, RATE_100HZ, SX1276_MODEM_CONFIG1_BW_500KHZ, 7, SX1276_MODEM_CONFIG1_CODING_RATE_4_7, 10000, 2, 8, 2000},
    {2, RATE_50HZ, SX1276_MODEM_CONFIG1_BW_500KHZ, 8, SX1276_MODEM_CONFIG1_CODING_RATE_4_7, 20000, 2, 8, 2000}};

void expresslrs_ota_set_rfrate(uint8_t rate) // Set speed of RF link (hz)
{
  expresslrs_mod_settings_s *const ModParams = get_elrs_airRateConfig(rate);

  if (ModParams->sf == 6)
  {
    SX1276_SetDetectionThreshold(SX1276_MODEM_DETECTION_THRESHOLD_SF6);
    SX1276_SetDetectOptimize(true, SX1276_DETECT_OPTIMIZE_SF6);
  }
  else
  {
    SX1276_SetDetectionThreshold(SX1276_MODEM_DETECTION_THRESHOLD_SF7_TO_SF12);
    SX1276_SetDetectOptimize(true, SX1276_DETECT_OPTIMIZE_SF7_TO_SF12);
  }

  SX1276_ConfigModem1(ModParams->bw, ModParams->cr, true);
  SX1276_ConfigModem2(ModParams->sf, false, false);
  SX1276_ConfigModem3(false, true);
  SX1276_SetPreambleLength(ModParams->PreambleLen);

  ExpressLRS_currAirRate_Modparams = ModParams;
}

//////////////////////////////////////////// RNG ROUTINES ////////////////////////////////////////////

#define RNG_MAX 0x7FFF
unsigned long ExpressLRS_seed = 0;

int32_t expresslrs_rng(void)
{
  const long m = 2147483648;
  const long a = 214013;
  const long c = 2531011;
  ExpressLRS_seed = (a * ExpressLRS_seed + c) % m;
  return ExpressLRS_seed >> 16;
}

void expresslrs_rngSeed(long newSeed)
{
  ExpressLRS_seed = newSeed;
}

unsigned int expresslrs_rngN(unsigned int max)
{
  unsigned long x = expresslrs_rng();
  unsigned int result = (x * max) / RNG_MAX;
  return result;
}

long expresslrs_rng8Bit(void)
{
  return expresslrs_rng() & 0b11111111; // 0..255 returned
}

long expresslrs_rng5Bit(void)
{
  return expresslrs_rng() & 0b11111; // 0..31 returned
}

long expresslrs_rng0to2(void)
{
  int randomNumber = expresslrs_rng() & 0b11; // 0..2 returned

  while (randomNumber == 3)
  {
    randomNumber = expresslrs_rng() & 0b11;
  }
  return randomNumber;
}

//////////////////////////////////////////// FHSS ROUTINES ////////////////////////////////////////////
uint32_t expresslrs_fhss_start_freq;
uint32_t expresslrs_fhss_interval;
uint32_t expresslrs_fhss_num_freqs;

uint8_t expresslrs_fhss_ptr = 0; //value of current index in expresslrs_fhss_sequence array

#define NR_SEQUENCE_ENTRIES 256

uint8_t expresslrs_fhss_sequence[NR_SEQUENCE_ENTRIES] = {0};

void expresslrs_fhss_init_freq(uint8_t regulatory_domain)
{
  switch (regulatory_domain)
  {
  case 0: //915 AU
    expresslrs_fhss_start_freq = 915500000;
    expresslrs_fhss_interval = 600000;
    expresslrs_fhss_num_freqs = 20;
    debugln("915 AU");
    break;
  case 1: // 915 FCC
    expresslrs_fhss_start_freq = 903500000;
    expresslrs_fhss_interval = 600000;
    expresslrs_fhss_num_freqs = 40;
    debugln("915 FCC");
    break;
  case 2: //868 EU
    expresslrs_fhss_start_freq = 863275000;
    expresslrs_fhss_interval = 525000;
    expresslrs_fhss_num_freqs = 13;
    debugln("868 EU");
    break;
  }

  expresslrs_fhss_generate_sequence(); // generate the pseudo random hop seq
}

uint32_t expresslrs_fhss_get_array_val(uint8_t index)
{
  return ((index * expresslrs_fhss_interval) + expresslrs_fhss_start_freq);
}

uint8_t expresslrs_fhss_get_index()
{
  return expresslrs_fhss_ptr;
}

uint32_t expresslrs_fhss_inital_freq()
{
  return expresslrs_fhss_start_freq;
}

uint32_t expresslrs_fhss_get_curr_freq()
{
  return expresslrs_fhss_get_array_val(expresslrs_fhss_sequence[expresslrs_fhss_ptr]);
}

uint32_t expresslrs_fhss_get_next_freq()
{
  expresslrs_fhss_ptr++;
  return expresslrs_fhss_get_curr_freq();
}

/**
Requirements:
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
void expresslrs_fhss_calc_reset_available(uint8_t *array)
{
  // channel 0 is the sync channel and is never considered available
  array[0] = 0;
  // all other entires to 1
  for (unsigned int i = 1; i < expresslrs_fhss_num_freqs; i++)
    array[i] = 1;
}

void expresslrs_fhss_generate_sequence()
{
  debug("Number of FHSS frequencies =");
  debugln(" %d", expresslrs_fhss_num_freqs);

  long macSeed = ((long)UID[2] << 24) + ((long)UID[3] << 16) + ((long)UID[4] << 8) + UID[5];
  expresslrs_rngSeed(macSeed);

  uint8_t expresslrs_fhss_is_available[expresslrs_fhss_num_freqs];

  expresslrs_fhss_calc_reset_available(expresslrs_fhss_is_available);

  // Fill the FHSSsequence with channel indices
  // The 0 index is special - the 'sync' channel. The sync channel appears every
  // syncInterval hops. The other channels are randomly distributed between the
  // sync channels
  int SYNC_INTERVAL = expresslrs_fhss_num_freqs - 1;

  int nLeft = expresslrs_fhss_num_freqs - 1; // how many channels are left to be allocated. Does not include the sync channel
  unsigned int prev = 0;                     // needed to prevent repeats of the same index

  // for each slot in the sequence table
  for (int i = 0; i < NR_SEQUENCE_ENTRIES; i++)
  {
    if (i % SYNC_INTERVAL == 0)
    {
      // assign sync channel 0
      expresslrs_fhss_sequence[i] = 0;
      prev = 0;
    }
    else
    {
      // pick one of the available channels. May need to loop to avoid repeats
      unsigned int index;
      do
      {
        int c = expresslrs_rngN(nLeft); // returnc 0<c<nLeft
        // find the c'th entry in the isAvailable array
        // can skip 0 as that's the sync channel and is never available for normal allocation
        index = 1;
        int found = 0;
        while (index < expresslrs_fhss_num_freqs)
        {
          if (expresslrs_fhss_is_available[index])
          {
            if (found == c)
              break;
            found++;
          }
          index++;
        }
        if (index == expresslrs_fhss_num_freqs)
        {
          // This should never happen
          debugln("FAILED to find the available entry!\n");
          // What to do? We don't want to hang as that will stop us getting to the wifi hotspot
          // Use the sync channel
          index = 0;
          break;
        }
      } while (index == prev); // can't use index if it repeats the previous value

      expresslrs_fhss_sequence[i] = index;     // assign the value to the sequence array
      expresslrs_fhss_is_available[index] = 0; // clear the flag
      prev = index;                            // remember for next iteration
      nLeft--;                                 // reduce the count of available channels
      if (nLeft == 0)
      {
        // we've assigned all of the channels, so reset for next cycle
        expresslrs_fhss_calc_reset_available(expresslrs_fhss_is_available);
        nLeft = expresslrs_fhss_num_freqs - 1;
      }
    }

    debug(" %02d", expresslrs_fhss_sequence[i]);
    if ((i + 1) % 10 == 0)
    {
      debugln(" ");
    }
    else
    {
      debug(" ");
    }
  } // for each element in FHSSsequence

  debugln("");

  debugln("List of Freqs:");

  for (int i = 0; i < expresslrs_fhss_num_freqs; i++)
  {
    debugln("%d", expresslrs_fhss_get_array_val(i));
  }

  debugln("");
}

/* CRC8 implementation with polynom = x​7​+ x​6​+ x​4​+ x​2​+ x​0 ​(0xD5) */ //from betaflight: https://github.com/betaflight/betaflight/blob/c8b5edb415c33916c91a7ccc8bd19c7276540cd1/src/test/unit/rx_crsf_unittest.cc#L65
const uint8_t expresslrs_crc8tab[256] = {
    0x00, 0xD5, 0x7F, 0xAA, 0xFE, 0x2B, 0x81, 0x54, 0x29, 0xFC, 0x56, 0x83, 0xD7, 0x02, 0xA8, 0x7D,
    0x52, 0x87, 0x2D, 0xF8, 0xAC, 0x79, 0xD3, 0x06, 0x7B, 0xAE, 0x04, 0xD1, 0x85, 0x50, 0xFA, 0x2F,
    0xA4, 0x71, 0xDB, 0x0E, 0x5A, 0x8F, 0x25, 0xF0, 0x8D, 0x58, 0xF2, 0x27, 0x73, 0xA6, 0x0C, 0xD9,
    0xF6, 0x23, 0x89, 0x5C, 0x08, 0xDD, 0x77, 0xA2, 0xDF, 0x0A, 0xA0, 0x75, 0x21, 0xF4, 0x5E, 0x8B,
    0x9D, 0x48, 0xE2, 0x37, 0x63, 0xB6, 0x1C, 0xC9, 0xB4, 0x61, 0xCB, 0x1E, 0x4A, 0x9F, 0x35, 0xE0,
    0xCF, 0x1A, 0xB0, 0x65, 0x31, 0xE4, 0x4E, 0x9B, 0xE6, 0x33, 0x99, 0x4C, 0x18, 0xCD, 0x67, 0xB2,
    0x39, 0xEC, 0x46, 0x93, 0xC7, 0x12, 0xB8, 0x6D, 0x10, 0xC5, 0x6F, 0xBA, 0xEE, 0x3B, 0x91, 0x44,
    0x6B, 0xBE, 0x14, 0xC1, 0x95, 0x40, 0xEA, 0x3F, 0x42, 0x97, 0x3D, 0xE8, 0xBC, 0x69, 0xC3, 0x16,
    0xEF, 0x3A, 0x90, 0x45, 0x11, 0xC4, 0x6E, 0xBB, 0xC6, 0x13, 0xB9, 0x6C, 0x38, 0xED, 0x47, 0x92,
    0xBD, 0x68, 0xC2, 0x17, 0x43, 0x96, 0x3C, 0xE9, 0x94, 0x41, 0xEB, 0x3E, 0x6A, 0xBF, 0x15, 0xC0,
    0x4B, 0x9E, 0x34, 0xE1, 0xB5, 0x60, 0xCA, 0x1F, 0x62, 0xB7, 0x1D, 0xC8, 0x9C, 0x49, 0xE3, 0x36,
    0x19, 0xCC, 0x66, 0xB3, 0xE7, 0x32, 0x98, 0x4D, 0x30, 0xE5, 0x4F, 0x9A, 0xCE, 0x1B, 0xB1, 0x64,
    0x72, 0xA7, 0x0D, 0xD8, 0x8C, 0x59, 0xF3, 0x26, 0x5B, 0x8E, 0x24, 0xF1, 0xA5, 0x70, 0xDA, 0x0F,
    0x20, 0xF5, 0x5F, 0x8A, 0xDE, 0x0B, 0xA1, 0x74, 0x09, 0xDC, 0x76, 0xA3, 0xF7, 0x22, 0x88, 0x5D,
    0xD6, 0x03, 0xA9, 0x7C, 0x28, 0xFD, 0x57, 0x82, 0xFF, 0x2A, 0x80, 0x55, 0x01, 0xD4, 0x7E, 0xAB,
    0x84, 0x51, 0xFB, 0x2E, 0x7A, 0xAF, 0x05, 0xD0, 0xAD, 0x78, 0xD2, 0x07, 0x53, 0x86, 0x2C, 0xF9};

uint8_t expresslrs_crc(uint8_t *data, int length)
{
  uint8_t crc = 0;
  for (uint8_t i = 0; i < length; i++)
  {
    crc = expresslrs_crc8tab[crc ^ *data++];
  }
  return crc;
}

#endif
