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

#ifndef _lmic_as923_h_
# define _lmic_as923_h_

#ifndef _lmic_eu_like_h_
# include "lmic_eu_like.h"
#endif

uint8_t LMICas923_maxFrameLen(uint8_t dr);
#define maxFrameLen(dr) LMICas923_maxFrameLen(dr)

int8_t LMICas923_pow2dBm(uint8_t mcmd_ladr_p1);
#define pow2dBm(mcmd_ladr_p1)   LMICas923_pow2dBm(mcmd_ladr_p1)

// Times for half symbol per DR
// Per DR table to minimize rounding errors
ostime_t LMICas923_dr2hsym(uint8_t dr);
#define dr2hsym(dr) LMICas923_dr2hsym(dr)

static inline int
LMICas923_isValidBeacon1(const uint8_t *d) {
        return os_rlsbf2(&d[OFF_BCN_CRC1]) != os_crc16(d, OFF_BCN_CRC1);
}

#undef LMICbandplan_isValidBeacon1
#define LMICbandplan_isValidBeacon1(pFrame) LMICas923_isValidBeacon1(pFrame)

// override default for LMICbandplan_resetDefaultChannels
void
LMICas923_resetDefaultChannels(void);

#undef LMICbandplan_resetDefaultChannels
#define LMICbandplan_resetDefaultChannels()     \
        LMICas923_resetDefaultChannels()

// override default for LMICbandplan_init
void LMICas923_init(void);

#undef LMICbandplan_init
#define LMICbandplan_init()     \
        LMICas923_init()


// override default for LMICbandplan_isFSK()
#undef LMICbandplan_isFSK
#define LMICbandplan_isFSK()    (/* TX datarate */LMIC.rxsyms == AS923_DR_FSK)

// txDone handling for FSK.
void
LMICas923_txDoneFSK(ostime_t delay, osjobcb_t func);

#define LMICbandplan_txDoneFsk(delay, func) LMICas923_txDoneFSK(delay, func)

#define LMICbandplan_getInitialDrJoin() (AS923_DR_SF10)

void LMICas923_setBcnRxParams(void);
#define LMICbandplan_setBcnRxParams()   LMICas923_setBcnRxParams()

u4_t LMICas923_convFreq(xref2cu1_t ptr);
#define LMICbandplan_convFreq(ptr)      LMICas923_convFreq(ptr)

void LMICas923_initJoinLoop(void);
#define LMICbandplan_initJoinLoop()     LMICas923_initJoinLoop()

// for as923, depending on dwell, we may need to do something else
#undef LMICbandplan_setRx1Params
void LMICas923_setRx1Params(void);
#define LMICbandplan_setRx1Params() LMICas923_setRx1Params()

ostime_t LMICas923_nextTx(ostime_t now);
#define LMICbandplan_nextTx(now)        LMICas923_nextTx(now)

ostime_t LMICas923_nextJoinState(void);
#define LMICbandplan_nextJoinState()    LMICas923_nextJoinState()

void LMICas923_initDefaultChannels(bit_t join);
#define LMICbandplan_initDefaultChannels(join)  LMICas923_initDefaultChannels(join)

// override default for LMICbandplan_updateTX
#undef LMICbandplan_updateTx
void LMICas923_updateTx(ostime_t txbeg);
#define LMICbandplan_updateTx(txbeg)	LMICas923_updateTx(txbeg)

#undef LMICbandplan_nextJoinTime
ostime_t LMICas923_nextJoinTime(ostime_t now);
#define LMICbandplan_nextJoinTime(now)     LMICas923_nextJoinTime(now)

#endif // _lmic_as923_h_
