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

#ifndef _lmic_eu_like_h_
# define _lmic_eu_like_h_

#ifndef _lmic_h_
# include "lmic.h"
#endif

// make sure we want US-like code
#if !CFG_LMIC_EU_like
# error "lmic not configured for EU-like bandplan"
#endif

// TODO(tmm@mcci.com): this should come from the lmic.h or lorabase.h file; and
// it's probably affected by the fix to this issue:
// https://github.com/mcci-catena/arduino-lmic/issues/2
#define DNW2_SAFETY_ZONE       ms2osticks(3000)

// provide a default for LMICbandplan_isValidBeacon1()
static inline int
LMICeulike_isValidBeacon1(const uint8_t *d) {
        return os_rlsbf2(&d[OFF_BCN_CRC1]) != os_crc16(d, OFF_BCN_CRC1);
}

#define LMICbandplan_isValidBeacon1(pFrame) LMICeulike_isValidBeacon1(pFrame)


// provide a default for LMICbandplan_isFSK()
#define LMICbandplan_isFSK()    (0)

// provide a default LMICbandplan_txDoneDoFSK()
#define LMICbandplan_txDoneFSK(delay, func)      do { } while (0)

#define LMICbandplan_joinAcceptChannelClear()   LMICbandplan_initDefaultChannels(/* normal, not join */ 0)

enum { BAND_MILLI = 0, BAND_CENTI = 1, BAND_DECI = 2, BAND_AUX = 3 };

// there's a CFList on joins for EU-like plans
#define LMICbandplan_hasJoinCFlist()    (1)

#define LMICbandplan_advanceBeaconChannel()     \
        do { /* nothing */ } while (0)

#define LMICbandplan_resetDefaultChannels()     \
        do { /* nothing */ } while (0)

#define LMICbandplan_setSessionInitDefaultChannels()    \
        do { LMICbandplan_initDefaultChannels(/* normal, not join */ 0); } while (0)

u1_t LMICeulike_mapChannels(u1_t chpage, u2_t chmap);
#define LMICbandplan_mapChannels(c, m)  LMICeulike_mapChannels(c, m)

void LMICeulike_initJoinLoop(u1_t nDefaultChannels, s1_t adrTxPow);

#define LMICbandplan_setRx1Params() \
        do { /*LMIC.freq/rps remain unchanged*/ } while (0)

void LMICeulike_updateTx(ostime_t txbeg);
#define LMICbandplan_updateTx(t)        LMICeulike_updateTx(t)

ostime_t LMICeulike_nextJoinState(uint8_t nDefaultChannels);

static inline ostime_t LMICeulike_nextJoinTime(ostime_t now) {
        return now;
}
#define LMICbandplan_nextJoinTime(now)     LMICeulike_nextJoinTime(now)

#define LMICbandplan_init()     \
        do { /* nothing */ } while (0)

#endif // _lmic_eu_like_h_
