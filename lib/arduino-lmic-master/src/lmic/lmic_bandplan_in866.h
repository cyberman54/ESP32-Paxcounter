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

#ifndef _lmic_in866_h_
# define _lmic_in866_h_

#ifndef _lmic_eu_like_h_
# include "lmic_eu_like.h"
#endif

uint8_t LMICin866_maxFrameLen(uint8_t dr);
#define maxFrameLen(dr) LMICin866_maxFrameLen(dr)

int8_t LMICin866_pow2dBm(uint8_t mcmd_ladr_p1);
#define pow2dBm(mcmd_ladr_p1)   LMICin866_pow2dBm(mcmd_ladr_p1)

// Times for half symbol per DR
// Per DR table to minimize rounding errors
ostime_t LMICin866_dr2hsym(uint8_t dr);
#define dr2hsym(dr) LMICin866_dr2hsym(dr)

static inline int
LMICin866_isValidBeacon1(const uint8_t *d) {
        return os_rlsbf2(&d[OFF_BCN_CRC1]) != os_crc16(d, OFF_BCN_CRC1);
}

#undef LMICbandplan_isValidBeacon1
#define LMICbandplan_isValidBeacon1(pFrame) LMICin866_isValidBeacon1(pFrame)

// override default for LMICbandplan_isFSK()
#undef LMICbandplan_isFSK
#define LMICbandplan_isFSK()    (/* TX datarate */LMIC.rxsyms == IN866_DR_FSK)

// txDone handling for FSK.
void
LMICin866_txDoneFSK(ostime_t delay, osjobcb_t func);

#define LMICbandplan_txDoneFsk(delay, func) LMICin866_txDoneFSK(delay, func)

#define LMICbandplan_getInitialDrJoin() (IN866_DR_SF7)

void LMICin866_setBcnRxParams(void);
#define LMICbandplan_setBcnRxParams()   LMICin866_setBcnRxParams()

u4_t LMICin866_convFreq(xref2cu1_t ptr);
#define LMICbandplan_convFreq(ptr)      LMICin866_convFreq(ptr)

void LMICin866_initJoinLoop(void);
#define LMICbandplan_initJoinLoop()     LMICin866_initJoinLoop()

ostime_t LMICin866_nextTx(ostime_t now);
#define LMICbandplan_nextTx(now)        LMICin866_nextTx(now)

ostime_t LMICin866_nextJoinState(void);
#define LMICbandplan_nextJoinState()    LMICin866_nextJoinState()

void LMICin866_initDefaultChannels(bit_t join);
#define LMICbandplan_initDefaultChannels(join)  LMICin866_initDefaultChannels(join)

#endif // _lmic_in866_h_
