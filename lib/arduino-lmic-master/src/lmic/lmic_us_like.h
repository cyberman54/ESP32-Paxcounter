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

#ifndef _lmic_us_like_h_
# define _lmic_us_like_h_

// make sure we want US-like code
#if !CFG_LMIC_US_like
# error "lmic not configured for us-like bandplan"
#endif

// TODO(tmm@mcci.com): this should come from the lmic.h or lorabase.h file; and
// it's probably affected by the fix to this issue:
// https://github.com/mcci-catena/arduino-lmic/issues/2
#define DNW2_SAFETY_ZONE       ms2osticks(750)

#define IS_CHANNEL_125khz(c) (c<64)
#define IS_CHANNEL_500khz(c) (c>=64 && c<72)
#define ENABLED_CHANNEL(chnl) ((LMIC.channelMap[(chnl >> 4)] & (1<<(chnl & 0x0F))) != 0)

// provide the isValidBeacon1 function -- int for bool.
static inline int
LMICuslike_isValidBeacon1(const uint8_t *d) {
        return os_rlsbf2(&d[OFF_BCN_CRC1]) != os_crc16(d, OFF_BCN_CRC1);
}

#define LMICbandplan_isValidBeacon1(pFrame) LMICuslike_isValidBeacon1(pFrame)

// provide a default for LMICbandplan_isFSK()
#define LMICbandplan_isFSK()    (0)

// provide a default LMICbandplan_txDoneFSK()
#define LMICbandplan_txDoneFSK(delay, func)      do { } while (0)

// provide a default LMICbandplan_joinAcceptChannelClear()
#define LMICbandplan_joinAcceptChannelClear() do { } while (0)

// no CFList on joins for US-like plans
#define LMICbandplan_hasJoinCFlist()    (0)

#define LMICbandplan_advanceBeaconChannel()     \
        do { LMIC.bcnChnl = (LMIC.bcnChnl+1) & 7; } while (0)

// TODO(tmm@mcci.com): decide whether we want to do this on every 
// reset or just restore the last sub-band selected by the user.
#define LMICbandplan_resetDefaultChannels()     \
        LMICbandplan_initDefaultChannels(/* normal */ 0)

void LMICuslike_initDefaultChannels(bit_t fJoin);
#define LMICbandplan_initDefaultChannels(fJoin) LMICuslike_initDefaultChannels(fJoin)

#define LMICbandplan_setSessionInitDefaultChannels()    \
        do { /* nothing */} while (0)

u1_t LMICuslike_mapChannels(u1_t chpage, u2_t chmap);
#define LMICbandplan_mapChannels(chpage, chmap) LMICuslike_mapChannels(chpage, chmap)

ostime_t LMICuslike_nextTx(ostime_t now);
#define LMICbandplan_nextTx(now)        LMICuslike_nextTx(now)

void LMICuslike_initJoinLoop(void);
#define LMICbandplan_initJoinLoop()     LMICuslike_initJoinLoop()

ostime_t LMICuslike_nextJoinState(void);
#define LMICbandplan_nextJoinState()    LMICuslike_nextJoinState();

static inline ostime_t LMICeulike_nextJoinTime(ostime_t now) {
        return now;
}
#define LMICbandplan_nextJoinTime(now)     LMICeulike_nextJoinTime(now)

#define LMICbandplan_init()     \
        do { /* nothing */ } while (0)

#endif // _lmic_us_like_h_
