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

#if defined(CFG_eu868)
// ================================================================================
//
// BEG: EU868 related stuff
//

CONST_TABLE(u1_t, _DR2RPS_CRC)[] = {
        ILLEGAL_RPS,
        (u1_t)MAKERPS(SF12, BW125, CR_4_5, 0, 0),
        (u1_t)MAKERPS(SF11, BW125, CR_4_5, 0, 0),
        (u1_t)MAKERPS(SF10, BW125, CR_4_5, 0, 0),
        (u1_t)MAKERPS(SF9,  BW125, CR_4_5, 0, 0),
        (u1_t)MAKERPS(SF8,  BW125, CR_4_5, 0, 0),
        (u1_t)MAKERPS(SF7,  BW125, CR_4_5, 0, 0),
        (u1_t)MAKERPS(SF7,  BW250, CR_4_5, 0, 0),
        (u1_t)MAKERPS(FSK,  BW125, CR_4_5, 0, 0),
        ILLEGAL_RPS
};

static CONST_TABLE(u1_t, maxFrameLens)[] = { 64,64,64,123 };

uint8_t LMICeu868_maxFrameLen(uint8_t dr) {
        if (dr < LENOF_TABLE(maxFrameLens))
                return TABLE_GET_U1(maxFrameLens, dr);
        else
                return 0xFF;
}

static CONST_TABLE(s1_t, TXPOWLEVELS)[] = {
        20, 14, 11, 8, 5, 2, 0,0, 0,0,0,0, 0,0,0,0
};

int8_t LMICeu868_pow2dBm(uint8_t mcmd_ladr_p1) {
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
        us2osticksRound(128 << 1),  // DR_SF7B
        us2osticksRound(80)       // FSK -- not used (time for 1/2 byte)
};

ostime_t LMICeu868_dr2hsym(uint8_t dr) {
        return TABLE_GET_OSTIME(DR2HSYM_osticks, dr);
}


enum { NUM_DEFAULT_CHANNELS = 3 };
static CONST_TABLE(u4_t, iniChannelFreq)[6] = {
        // Join frequencies and duty cycle limit (0.1%)
        EU868_F1 | BAND_MILLI, EU868_F2 | BAND_MILLI, EU868_F3 | BAND_MILLI,
        // Default operational frequencies and duty cycle limit (1%)
        EU868_F1 | BAND_CENTI, EU868_F2 | BAND_CENTI, EU868_F3 | BAND_CENTI,
};

void LMICeu868_initDefaultChannels(bit_t join) {
        os_clearMem(&LMIC.channelFreq, sizeof(LMIC.channelFreq));
        os_clearMem(&LMIC.channelDrMap, sizeof(LMIC.channelDrMap));
        os_clearMem(&LMIC.bands, sizeof(LMIC.bands));

        LMIC.channelMap = (1 << NUM_DEFAULT_CHANNELS) - 1;
        u1_t su = join ? 0 : NUM_DEFAULT_CHANNELS;
        for (u1_t fu = 0; fu<NUM_DEFAULT_CHANNELS; fu++, su++) {
                LMIC.channelFreq[fu] = TABLE_GET_U4(iniChannelFreq, su);
                // TODO(tmm@mcci.com): don't use EU DR directly, use something from the LMIC context or a static const
                LMIC.channelDrMap[fu] = DR_RANGE_MAP(EU868_DR_SF12, EU868_DR_SF7);
        }

        LMIC.bands[BAND_MILLI].txcap = 1000;  // 0.1%
        LMIC.bands[BAND_MILLI].txpow = 14;
        LMIC.bands[BAND_MILLI].lastchnl = os_getRndU1() % MAX_CHANNELS;
        LMIC.bands[BAND_CENTI].txcap = 100;   // 1%
        LMIC.bands[BAND_CENTI].txpow = 14;
        LMIC.bands[BAND_CENTI].lastchnl = os_getRndU1() % MAX_CHANNELS;
        LMIC.bands[BAND_DECI].txcap = 10;    // 10%
        LMIC.bands[BAND_DECI].txpow = 27;
        LMIC.bands[BAND_DECI].lastchnl = os_getRndU1() % MAX_CHANNELS;
        LMIC.bands[BAND_MILLI].avail =
                LMIC.bands[BAND_CENTI].avail =
                LMIC.bands[BAND_DECI].avail = os_getTime();
}

bit_t LMIC_setupBand(u1_t bandidx, s1_t txpow, u2_t txcap) {
        if (bandidx > BAND_AUX) return 0;
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
                if (freq >= 869400000 && freq <= 869650000)
                        freq |= BAND_DECI;   // 10% 27dBm
                else if ((freq >= 868000000 && freq <= 868600000) ||
                        (freq >= 869700000 && freq <= 870000000))
                        freq |= BAND_CENTI;  // 1% 14dBm
                else
                        freq |= BAND_MILLI;  // 0.1% 14dBm
        }
        else {
                if (band > BAND_AUX) return 0;
                freq = (freq&~3) | band;
        }
        LMIC.channelFreq[chidx] = freq;
        // TODO(tmm@mcci.com): don't use US SF directly, use something from the LMIC context or a static const
        LMIC.channelDrMap[chidx] = drmap == 0 ? DR_RANGE_MAP(EU868_DR_SF12, EU868_DR_SF7) : drmap;
        LMIC.channelMap |= 1 << chidx;  // enabled right away
        return 1;
}



u4_t LMICeu868_convFreq(xref2cu1_t ptr) {
        u4_t freq = (os_rlsbf4(ptr - 1) >> 8) * 100;
        if (freq < EU868_FREQ_MIN || freq > EU868_FREQ_MAX)
                freq = 0;
        return freq;
}

ostime_t LMICeu868_nextJoinTime(ostime_t time) {
        // is the avail time in the future?
        if ((s4_t) (time - LMIC.bands[BAND_MILLI].avail) < 0)
                // yes: then wait until then.
                time = LMIC.bands[BAND_MILLI].avail;

        return time;
}

ostime_t LMICeu868_nextTx(ostime_t now) {
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
void LMICeu868_setBcnRxParams(void) {
        LMIC.dataLen = 0;
        LMIC.freq = LMIC.channelFreq[LMIC.bcnChnl] & ~(u4_t)3;
        LMIC.rps = setIh(setNocrc(dndr2rps((dr_t)DR_BCN), 1), LEN_BCN);
}
#endif // !DISABLE_BEACONS

#if !defined(DISABLE_JOIN)
ostime_t LMICeu868_nextJoinState(void) {
        return LMICeulike_nextJoinState(NUM_DEFAULT_CHANNELS);
}
#endif // !DISABLE_JOIN

// txDone handling for FSK.
void
LMICeu868_txDoneFSK(ostime_t delay, osjobcb_t func) {
        LMIC.rxtime = LMIC.txend + delay - PRERX_FSK*us2osticksRound(160);
        LMIC.rxsyms = RXLEN_FSK;
        os_setTimedCallback(&LMIC.osjob, LMIC.rxtime - RX_RAMPUP, func);
}

void
LMICeu868_initJoinLoop(void) {
        LMICeulike_initJoinLoop(NUM_DEFAULT_CHANNELS, /* adr dBm */ EU868_TX_EIRP_MAX_DBM);
}

//
// END: EU868 related stuff
//
// ================================================================================
#endif