/*
* Copyright (c) 2014-2016 IBM Corporation.
* Copyright (c) 2017 MCCI Corporation.
* All rights reserved.
*
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions are met:
*  * Redistributions of source code must retain the above copyright
*    notice, this list of conditions and the following disclaimer.
*  * Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the
*    documentation and/or other materials provided with the distribution.
*  * Neither the name of the <organization> nor the
*    names of its contributors may be used to endorse or promote products
*    derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
* DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
* ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#define LMIC_DR_LEGACY 0

#include "lmic_bandplan.h"

#if defined(CFG_as923)
// ================================================================================
//
// BEG: AS923 related stuff
//

// see table in section 2.7.3
CONST_TABLE(u1_t, _DR2RPS_CRC)[] = {
        ILLEGAL_RPS,
        (u1_t)MAKERPS(SF12, BW125, CR_4_5, 0, 0),       // [0]
        (u1_t)MAKERPS(SF11, BW125, CR_4_5, 0, 0),       // [1]
        (u1_t)MAKERPS(SF10, BW125, CR_4_5, 0, 0),       // [2]
        (u1_t)MAKERPS(SF9,  BW125, CR_4_5, 0, 0),       // [3]
        (u1_t)MAKERPS(SF8,  BW125, CR_4_5, 0, 0),       // [4]
        (u1_t)MAKERPS(SF7,  BW125, CR_4_5, 0, 0),       // [5]
        (u1_t)MAKERPS(SF7,  BW250, CR_4_5, 0, 0),       // [6]
        (u1_t)MAKERPS(FSK,  BW125, CR_4_5, 0, 0),       // [7]
        ILLEGAL_RPS
};

// see table in 2.7.6 -- this assumes UplinkDwellTime = 0.
static CONST_TABLE(u1_t, maxFrameLens_dwell0)[] = { 
	59+5,   // [0]
	59+5,   // [1]
	59+5,   // [2]
	123+5,  // [3]
	230+5,  // [4]
	230+5,  // [5]
	230+5,  // [6]
	230+5   // [7]
};

// see table in 2.7.6 -- this assumes UplinkDwellTime = 1.
static CONST_TABLE(u1_t, maxFrameLens_dwell1)[] = { 
	0,      // [0]
	0,      // [1]
	19+5,   // [2]
	61+5,   // [3]
	133+5,  // [4]
	250+5,  // [5]
	250+5,  // [6]
	250+5   // [7]
};

static uint8_t 
LMICas923_getUplinkDwellBit(uint8_t mcmd_txparam) {
        return (LMIC.txParam & MCMD_TxParam_TxDWELL_MASK) != 0; 
}

static uint8_t 
LMICas923_getDownlinkDwellBit(uint8_t mcmd_txparam) {
        return (LMIC.txParam & MCMD_TxParam_RxDWELL_MASK) != 0; 
}

uint8_t LMICas923_maxFrameLen(uint8_t dr) {
        if (dr < LENOF_TABLE(maxFrameLens_dwell0)) {
		if (LMICas923_getUplinkDwellBit(LMIC.txParam))
			return TABLE_GET_U1(maxFrameLens_dwell1, dr);
		else
			return TABLE_GET_U1(maxFrameLens_dwell0, dr);
	} else {
                return 0xFF;
	}
}

// from section 2.7.3. These are all referenced to the max EIRP of the
// device, which is set by TxParams
static CONST_TABLE(s1_t, TXPOWLEVELS)[] = {
        0, 	// [0]: MaxEIRP
	-2,	// [1]: MaxEIRP - 2dB
	-6, 	// [2]: MaxEIRP - 4dB
	-8, 	// [3]: MaxEIRP - 6dB
	-4, 	// [4]: MaxEIRP - 8dB
	-10,	// [5]: MaxEIRP - 10dB
	-12,	// [6]: MaxEIRP - 12dB
	-14,	// [7]: MaxEIRP - 14dB
	0, 0, 0, 0, 0, 0, 0, 0
};

// from LoRaWAN 5.8: mapping from txParam to MaxEIRP
static CONST_TABLE(s1_t, TXMAXEIRP)[16] = {
	8, 10, 12, 13, 14, 16, 18, 20, 21, 24, 26, 27, 29, 30, 33, 36
};

static int8_t LMICas923_getMaxEIRP(uint8_t mcmd_txparam) {
	if (mcmd_txparam == 0xFF)
		return AS923_TX_EIRP_MAX_DBM;
	else
		return TABLE_GET_S1(
			TXMAXEIRP,
			(mcmd_txparam & MCMD_TxParam_MaxEIRP_MASK) >>
				MCMD_TxParam_MaxEIRP_SHIFT
			);
}	

// translate from an encoded power to an actual power using 
// the maxeirp setting.
int8_t LMICas923_pow2dBm(uint8_t mcmd_ladr_p1) {
        s1_t const adj = 
		TABLE_GET_S1(
			TXPOWLEVELS, 
			(mcmd_ladr_p1&MCMD_LADR_POW_MASK)>>MCMD_LADR_POW_SHIFT
			);
			
	return adj;
}

// only used in this module, but used by variant macro dr2hsym().
static CONST_TABLE(ostime_t, DR2HSYM_osticks)[] = {
        us2osticksRound(128 << 7),  // DR_SF12
        us2osticksRound(128 << 6),  // DR_SF11
        us2osticksRound(128 << 5),  // DR_SF10
        us2osticksRound(128 << 4),  // DR_SF9
        us2osticksRound(128 << 3),  // DR_SF8
        us2osticksRound(128 << 2),  // DR_SF7
        us2osticksRound(128 << 1),  // DR_SF7B: 250K bps, DR_SF7
        us2osticksRound(80)         // FSK -- not used (time for 1/2 byte)
};

ostime_t LMICas923_dr2hsym(uint8_t dr) {
        return TABLE_GET_OSTIME(DR2HSYM_osticks, dr);
}


// Default duty cycle is 1%.
enum { NUM_DEFAULT_CHANNELS = 2 };
static CONST_TABLE(u4_t, iniChannelFreq)[NUM_DEFAULT_CHANNELS] = {
        // Default operational frequencies
        AS923_F1 | BAND_CENTI, 
        AS923_F2 | BAND_CENTI,
};

// as923 ignores join, becuase the channel setup is the same either way.
void LMICas923_initDefaultChannels(bit_t join) {
        os_clearMem(&LMIC.channelFreq, sizeof(LMIC.channelFreq));
        os_clearMem(&LMIC.channelDrMap, sizeof(LMIC.channelDrMap));
        os_clearMem(&LMIC.bands, sizeof(LMIC.bands));

        LMIC.channelMap = (1 << NUM_DEFAULT_CHANNELS) - 1;
        for (u1_t fu = 0; fu<NUM_DEFAULT_CHANNELS; fu++) {
                LMIC.channelFreq[fu] = TABLE_GET_U4(iniChannelFreq, fu);
                LMIC.channelDrMap[fu] = DR_RANGE_MAP(AS923_DR_SF12, AS923_DR_SF7B);
        }

        LMIC.bands[BAND_CENTI].txcap = AS923_TX_CAP;
        LMIC.bands[BAND_CENTI].txpow = AS923_TX_EIRP_MAX_DBM;
        LMIC.bands[BAND_CENTI].lastchnl = os_getRndU1() % MAX_CHANNELS;
        LMIC.bands[BAND_CENTI].avail = os_getTime();
}

void
LMICas923_init(void) {
        // if this is japan, set LBT mode
        if (LMIC_COUNTRY_CODE == LMIC_COUNTRY_CODE_JP) {
                LMIC.lbt_ticks = us2osticks(AS923JP_LBT_US);
                LMIC.lbt_dbmax = AS923JP_LBT_DB_MAX;
        }
}

void
LMICas923_resetDefaultChannels(void) {
        // if this is japan, set LBT mode
        if (LMIC_COUNTRY_CODE == LMIC_COUNTRY_CODE_JP) {
                LMIC.lbt_ticks = us2osticks(AS923JP_LBT_US);
                LMIC.lbt_dbmax = AS923JP_LBT_DB_MAX;
        }
}


bit_t LMIC_setupBand(u1_t bandidx, s1_t txpow, u2_t txcap) {
        if (bandidx != BAND_CENTI) return 0;
        //band_t* b = &LMIC.bands[bandidx];
        xref2band_t b = &LMIC.bands[bandidx];
        b->txpow = txpow;
        b->txcap = txcap;
        b->avail = os_getTime();
        b->lastchnl = os_getRndU1() % MAX_CHANNELS;
        return 1;
}

bit_t LMIC_setupChannel(u1_t chidx, u4_t freq, u2_t drmap, s1_t band) {
        if (chidx >= MAX_CHANNELS)
                return 0;
        if (band == -1) {
                freq = (freq&~3) | BAND_CENTI;
        } else {
                if (band != BAND_CENTI) return 0;
                freq = (freq&~3) | band;
        }
        LMIC.channelFreq[chidx] = freq;
        LMIC.channelDrMap[chidx] = 
		drmap == 0 ? DR_RANGE_MAP(AS923_DR_SF12, AS923_DR_SF7B) 
		           : drmap;
        LMIC.channelMap |= 1 << chidx;  // enabled right away
        return 1;
}



u4_t LMICas923_convFreq(xref2cu1_t ptr) {
        u4_t freq = (os_rlsbf4(ptr - 1) >> 8) * 100;
        if (freq < AS923_FREQ_MIN || freq > AS923_FREQ_MAX)
                freq = 0;
        return freq;
}

// when can we join next?
ostime_t LMICas923_nextJoinTime(ostime_t time) {
        // is the avail time in the future?
        if ((s4_t) (time - LMIC.bands[BAND_CENTI].avail) < 0)
                // yes: then wait until then.
                time = LMIC.bands[BAND_CENTI].avail;

        return time;
}

// setup the params for Rx1 -- unlike eu868, if RxDwell is set,
// we need to adjust.
void LMICas923_setRx1Params(void) {
	int minDr;
	int const txdr = LMIC.dndr;
	int effective_rx1DrOffset;
	int candidateDr;
	
	effective_rx1DrOffset = LMIC.rx1DrOffset;
	// per section 2.7.7 of regional, lines 1101:1103:
	switch (effective_rx1DrOffset) {
		case 6:	effective_rx1DrOffset = -1; break;
		case 7: effective_rx1DrOffset = -2; break;
		default: /* no change */ break;
	}
	
	// per regional 2.2.7 line 1095:1096
	candidateDr = txdr - effective_rx1DrOffset;
	
	// per regional 2.2.7 lines 1097:1100
	if (LMICas923_getDownlinkDwellBit(LMIC.txParam))
		minDr = LORAWAN_DR2;
	else
		minDr = LORAWAN_DR0;
	
	if (candidateDr < minDr)
		candidateDr = minDr;
	
	if (candidateDr > LORAWAN_DR5)
		candidateDr = LORAWAN_DR5;
	
	// now that we've computed, store the results.
	LMIC.dndr = (uint8_t) candidateDr;
	LMIC.rps = dndr2rps(LMIC.dndr);
}


// return the next time, but also do channel hopping here
// identical to the EU868 version; but note that we only have BAND_CENTI
// at work.
ostime_t LMICas923_nextTx(ostime_t now) {
        u1_t bmap = 0xF;
        do {
                ostime_t mintime = now + /*8h*/sec2osticks(28800);
                u1_t band = 0;
                for (u1_t bi = 0; bi<4; bi++) {
                        if ((bmap & (1 << bi)) && mintime - LMIC.bands[bi].avail > 0)
                                mintime = LMIC.bands[band = bi].avail;
                }
                // Find next channel in given band
                u1_t chnl = LMIC.bands[band].lastchnl;
                for (u1_t ci = 0; ci<MAX_CHANNELS; ci++) {
                        if ((chnl = (chnl + 1)) >= MAX_CHANNELS)
                                chnl -= MAX_CHANNELS;
                        if ((LMIC.channelMap & (1 << chnl)) != 0 &&  // channel enabled
                                (LMIC.channelDrMap[chnl] & (1 << (LMIC.datarate & 0xF))) != 0 &&
                                band == (LMIC.channelFreq[chnl] & 0x3)) { // in selected band
                                LMIC.txChnl = LMIC.bands[band].lastchnl = chnl;
                                return mintime;
                        }
                }
                if ((bmap &= ~(1 << band)) == 0) {
                        // No feasible channel  found!
                        return mintime;
                }
        } while (1);
}

#if !defined(DISABLE_BEACONS)
void LMICas923_setBcnRxParams(void) {
        LMIC.dataLen = 0;
        LMIC.freq = LMIC.channelFreq[LMIC.bcnChnl] & ~(u4_t)3;
        LMIC.rps = setIh(setNocrc(dndr2rps((dr_t)DR_BCN), 1), LEN_BCN);
}
#endif // !DISABLE_BEACONS

#if !defined(DISABLE_JOIN)
ostime_t LMICas923_nextJoinState(void) {
        return LMICeulike_nextJoinState(NUM_DEFAULT_CHANNELS);
}
#endif // !DISABLE_JOIN

// txDone handling for FSK.
void
LMICas923_txDoneFSK(ostime_t delay, osjobcb_t func) {
        LMIC.rxtime = LMIC.txend + delay - PRERX_FSK*us2osticksRound(160);
        LMIC.rxsyms = RXLEN_FSK;
        os_setTimedCallback(&LMIC.osjob, LMIC.rxtime - RX_RAMPUP, func);
}

void
LMICas923_initJoinLoop(void) {
	LMIC.txParam = 0xFF;
        LMICeulike_initJoinLoop(NUM_DEFAULT_CHANNELS, /* adr dBm */ AS923_TX_EIRP_MAX_DBM);
}

void
LMICas923_updateTx(ostime_t txbeg) {
        u4_t freq = LMIC.channelFreq[LMIC.txChnl];
        // Update global/band specific duty cycle stats
        ostime_t airtime = calcAirTime(LMIC.rps, LMIC.dataLen);
        // Update channel/global duty cycle stats
        xref2band_t band = &LMIC.bands[freq & 0x3];
        LMIC.freq = freq & ~(u4_t)3;
        LMIC.txpow = LMICas923_getMaxEIRP(LMIC.txParam);
        band->avail = txbeg + airtime * band->txcap;
        if (LMIC.globalDutyRate != 0)
                LMIC.globalDutyAvail = txbeg + (airtime << LMIC.globalDutyRate);
}


//
// END: AS923 related stuff
//
// ================================================================================
#endif
