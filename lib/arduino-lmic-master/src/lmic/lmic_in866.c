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

#if defined(CFG_in866)
// ================================================================================
//
// BEG: IN866 related stuff
//

CONST_TABLE(u1_t, _DR2RPS_CRC)[] = {
        ILLEGAL_RPS,
        (u1_t)MAKERPS(SF12, BW125, CR_4_5, 0, 0),       // [0]
        (u1_t)MAKERPS(SF11, BW125, CR_4_5, 0, 0),       // [1]
        (u1_t)MAKERPS(SF10, BW125, CR_4_5, 0, 0),       // [2]
        (u1_t)MAKERPS(SF9,  BW125, CR_4_5, 0, 0),       // [3]
        (u1_t)MAKERPS(SF8,  BW125, CR_4_5, 0, 0),       // [4]
        (u1_t)MAKERPS(SF7,  BW125, CR_4_5, 0, 0),       // [5]
        ILLEGAL_RPS,                                    // [6]
        (u1_t)MAKERPS(FSK,  BW125, CR_4_5, 0, 0),       // [7]
        ILLEGAL_RPS
};

static CONST_TABLE(u1_t, maxFrameLens)[] = { 59+5,59+5,59+5,123+5, 230+5, 230+5 };

uint8_t LMICin866_maxFrameLen(uint8_t dr) {
        if (dr < LENOF_TABLE(maxFrameLens))
                return TABLE_GET_U1(maxFrameLens, dr);
        else 
                return 0xFF;
}

static CONST_TABLE(s1_t, TXPOWLEVELS)[] = {
        20, 14, 11, 8, 5, 2, 0,0, 0,0,0,0, 0,0,0,0
};

int8_t LMICin866_pow2dBm(uint8_t mcmd_ladr_p1) {
        return TABLE_GET_S1(TXPOWLEVELS, (mcmd_ladr_p1&MCMD_LADR_POW_MASK)>>MCMD_LADR_POW_SHIFT);
}

// only used in this module, but used by variant macro dr2hsym().
static CONST_TABLE(ostime_t, DR2HSYM_osticks)[] = {
        us2osticksRound(128 << 7),  // DR_SF12
        us2osticksRound(128 << 6),  // DR_SF11
        us2osticksRound(128 << 5),  // DR_SF10
        us2osticksRound(128 << 4),  // DR_SF9
        us2osticksRound(128 << 3),  // DR_SF8
        us2osticksRound(128 << 2),  // DR_SF7
        us2osticksRound(128 << 1),  // --
        us2osticksRound(80)       // FSK -- not used (time for 1/2 byte)
};

ostime_t LMICin866_dr2hsym(uint8_t dr) {
        return TABLE_GET_OSTIME(DR2HSYM_osticks, dr);
}


// All frequencies are marked as BAND_MILLI, and we don't do duty-cycle. But this lets
// us reuse code.
enum { NUM_DEFAULT_CHANNELS = 3 };
static CONST_TABLE(u4_t, iniChannelFreq)[NUM_DEFAULT_CHANNELS] = {
        // Default operational frequencies
        IN866_F1 | BAND_MILLI, 
        IN866_F2 | BAND_MILLI, 
        IN866_F3 | BAND_MILLI,
};

// india ignores join, becuase the channel setup is the same either way.
void LMICin866_initDefaultChannels(bit_t join) {
        os_clearMem(&LMIC.channelFreq, sizeof(LMIC.channelFreq));
        os_clearMem(&LMIC.channelDrMap, sizeof(LMIC.channelDrMap));
        os_clearMem(&LMIC.bands, sizeof(LMIC.bands));

        LMIC.channelMap = (1 << NUM_DEFAULT_CHANNELS) - 1;
        for (u1_t fu = 0; fu<NUM_DEFAULT_CHANNELS; fu++) {
                LMIC.channelFreq[fu] = TABLE_GET_U4(iniChannelFreq, fu);
                LMIC.channelDrMap[fu] = DR_RANGE_MAP(IN866_DR_SF12, IN866_DR_SF7);
        }

        LMIC.bands[BAND_MILLI].txcap = 1;  // no limit, in effect.
        LMIC.bands[BAND_MILLI].txpow = IN866_TX_EIRP_MAX_DBM;
        LMIC.bands[BAND_MILLI].lastchnl = os_getRndU1() % MAX_CHANNELS;
        LMIC.bands[BAND_MILLI].avail = os_getTime();
}

bit_t LMIC_setupBand(u1_t bandidx, s1_t txpow, u2_t txcap) {
        if (bandidx > BAND_MILLI) return 0;
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
                freq |= BAND_MILLI;
        } else {
                if (band > BAND_MILLI) return 0;
                freq = (freq&~3) | band;
        }
        LMIC.channelFreq[chidx] = freq;
        LMIC.channelDrMap[chidx] = drmap == 0 ? DR_RANGE_MAP(IN866_DR_SF12, IN866_DR_SF7) : drmap;
        LMIC.channelMap |= 1 << chidx;  // enabled right away
        return 1;
}



u4_t LMICin866_convFreq(xref2cu1_t ptr) {
        u4_t freq = (os_rlsbf4(ptr - 1) >> 8) * 100;
        if (freq < IN866_FREQ_MIN || freq > IN866_FREQ_MAX)
                freq = 0;
        return freq;
}

// return the next time, but also do channel hopping here
// since there's no duty cycle limitation, and no dwell limitation,
// we simply loop through the channels sequentially.
ostime_t LMICin866_nextTx(ostime_t now) {
        const u1_t band = BAND_MILLI;

        for (u1_t ci = 0; ci < MAX_CHANNELS; ci++) {
                // Find next channel in given band
                u1_t chnl = LMIC.bands[band].lastchnl;
                for (u1_t ci = 0; ci<MAX_CHANNELS; ci++) {
                        if ((chnl = (chnl + 1)) >= MAX_CHANNELS)
                                chnl -= MAX_CHANNELS;
                        if ((LMIC.channelMap & (1 << chnl)) != 0 &&  // channel enabled
                                (LMIC.channelDrMap[chnl] & (1 << (LMIC.datarate & 0xF))) != 0 &&
                                band == (LMIC.channelFreq[chnl] & 0x3)) { // in selected band
                                LMIC.txChnl = LMIC.bands[band].lastchnl = chnl;
                                return now;
                        }
                }
        }

        // no enabled channel found! just use the last channel. 
        return now;
}

#if !defined(DISABLE_BEACONS)
void LMICin866_setBcnRxParams(void) {
        LMIC.dataLen = 0;
        LMIC.freq = LMIC.channelFreq[LMIC.bcnChnl] & ~(u4_t)3;
        LMIC.rps = setIh(setNocrc(dndr2rps((dr_t)DR_BCN), 1), LEN_BCN);
}
#endif // !DISABLE_BEACONS

#if !defined(DISABLE_JOIN)
ostime_t LMICin866_nextJoinState(void) {
        return LMICeulike_nextJoinState(NUM_DEFAULT_CHANNELS);
}
#endif // !DISABLE_JOIN

// txDone handling for FSK.
void
LMICin866_txDoneFSK(ostime_t delay, osjobcb_t func) {
        LMIC.rxtime = LMIC.txend + delay - PRERX_FSK*us2osticksRound(160);
        LMIC.rxsyms = RXLEN_FSK;
        os_setTimedCallback(&LMIC.osjob, LMIC.rxtime - RX_RAMPUP, func);
}

void
LMICin866_initJoinLoop(void) {
        LMICeulike_initJoinLoop(NUM_DEFAULT_CHANNELS, /* adr dBm */ IN866_TX_EIRP_MAX_DBM);
}

//
// END: IN866 related stuff
//
// ================================================================================
#endif