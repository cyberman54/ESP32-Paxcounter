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

#ifndef _lmic_eu868_h_
# define _lmic_eu868_h_

#ifndef _lmic_eu_like_h_
# include "lmic_eu_like.h"
#endif

uint8_t LMICeu868_maxFrameLen(uint8_t dr);
#define maxFrameLen(dr) LMICeu868_maxFrameLen(dr)

int8_t LMICeu868_pow2dBm(uint8_t mcmd_ladr_p1);
#define pow2dBm(mcmd_ladr_p1)   LMICeu868_pow2dBm(mcmd_ladr_p1)

// Times for half symbol per DR
// Per DR table to minimize rounding errors
ostime_t LMICeu868_dr2hsym(uint8_t dr);
#define dr2hsym(dr) LMICeu868_dr2hsym(dr)


// TODO(tmm@mcci.com) this looks bogus compared to current 1.02 regional
// spec. https://github.com/mcci-catena/arduino-lmic/issues/18
static inline int
LMICeu868_isValidBeacon1(const uint8_t *d) {
    return d[OFF_BCN_CRC1] != (u1_t)os_crc16(d, OFF_BCN_CRC1);
}

#undef LMICbandplan_isValidBeacon1
#define LMICbandplan_isValidBeacon1(pFrame) LMICeu868_isValidBeacon1(pFrame)

// override default for LMICbandplan_isFSK()
#undef LMICbandplan_isFSK
#define LMICbandplan_isFSK()    (/* TX datarate */LMIC.rxsyms == EU868_DR_FSK)

// txDone handling for FSK.
void
LMICeu868_txDoneFSK(ostime_t delay, osjobcb_t func);

#define LMICbandplan_txDoneFsk(delay, func) LMICeu868_txDoneFSK(delay, func)

#define LMICbandplan_getInitialDrJoin() (EU868_DR_SF7)

void LMICeu868_setBcnRxParams(void);
#define LMICbandplan_setBcnRxParams()   LMICeu868_setBcnRxParams()

u4_t LMICeu868_convFreq(xref2cu1_t ptr);
#define LMICbandplan_convFreq(ptr)      LMICeu868_convFreq(ptr)

void LMICeu868_initJoinLoop(void);
#define LMICbandplan_initJoinLoop()     LMICeu868_initJoinLoop()

ostime_t LMICeu868_nextTx(ostime_t now);
#define LMICbandplan_nextTx(now)        LMICeu868_nextTx(now)

ostime_t LMICeu868_nextJoinState(void);
#define LMICbandplan_nextJoinState()    LMICeu868_nextJoinState()

void LMICeu868_initDefaultChannels(bit_t join);
#define LMICbandplan_initDefaultChannels(join)  LMICeu868_initDefaultChannels(join)

#undef LMICbandplan_nextJoinTime
ostime_t LMICeu868_nextJoinTime(ostime_t now);
#define LMICbandplan_nextJoinTime(now)     LMICeu868_nextJoinTime(now)

#endif // _lmic_eu868_h_
