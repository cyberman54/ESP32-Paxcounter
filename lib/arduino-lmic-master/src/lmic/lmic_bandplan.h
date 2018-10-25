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

#ifndef _lmic_bandplan_h_
# define _lmic_bandplan_h_

#ifndef _lmic_h_
# include "lmic.h"
#endif

#if defined(CFG_eu868)
# include "lmic_bandplan_eu868.h"
#elif defined(CFG_us915)
# include "lmic_bandplan_us915.h"
#elif defined(CFG_au921)
# include "lmic_bandplan_au921.h"
#elif defined(CFG_as923)
# include "lmic_bandplan_as923.h"
#elif defined(CFG_in866)
# include "lmic_bandplan_in866.h"
#else
# error "CFG_... not properly set for bandplan"
#endif

// check post-conditions
#ifndef DNW2_SAFETY_ZONE
# error "DNW2_SAFETY_ZONE not defined by bandplan"
#endif

#ifndef maxFrameLen
# error "maxFrameLen() not defined by bandplan"
#endif

#ifndef pow2dBm
# error "pow2dBm() not defined by bandplan"
#endif

#ifndef dr2hsym
# error "dr2hsym() not defined by bandplan"
#endif

#if !defined(LMICbandplan_isValidBeacon1) && !defined(DISABLE_BEACONS)
# error "LMICbandplan_isValidBeacon1 not defined by bandplan"
#endif

#if !defined(LMICbandplan_isFSK)
# error "LMICbandplan_isFSK() not defined by bandplan"
#endif

#if !defined(LMICbandplan_txDoneFSK)
# error "LMICbandplan_txDoneFSK() not defined by bandplan"
#endif

#if !defined(LMICbandplan_joinAcceptChannelClear)
# error "LMICbandplan_joinAcceptChannelClear() not defined by bandplan"
#endif

#if !defined(LMICbandplan_getInitialDrJoin)
# error "LMICbandplan_getInitialDrJoin() not defined by bandplan"
#endif

#if !defined(LMICbandplan_hasJoinCFlist)
# error "LMICbandplan_hasJoinCFlist() not defined by bandplan"
#endif

#if !defined(LMICbandplan_advanceBeaconChannel)
# error "LMICbandplan_advanceBeaconChannel() not defined by bandplan"
#endif

#if !defined(LMICbandplan_resetDefaultChannels)
# error "LMICbandplan_resetDefaultChannels() not defined by bandplan"
#endif

#if !defined(LMICbandplan_setSessionInitDefaultChannels)
# error "LMICbandplan_setSessionInitDefaultChannels() not defined by bandplan"
#endif

#if !defined(LMICbandplan_setBcnRxParams)
# error "LMICbandplan_setBcnRxParams() not defined by bandplan"
#endif

#if !defined(LMICbandplan_mapChannels)
# error "LMICbandplan_mapChannels() not defined by bandplan"
#endif

#if !defined(LMICbandplan_convFreq)
# error "LMICbandplan_convFreq() not defined by bandplan"
#endif

#if !defined(LMICbandplan_setRx1Params)
# error "LMICbandplan_setRx1Params() not defined by bandplan"
#endif

#if !defined(LMICbandplan_initJoinLoop)
# error "LMICbandplan_initJoinLoop() not defined by bandplan"
#endif

#if !defined(LMICbandplan_nextTx)
# error "LMICbandplan_nextTx() not defined by bandplan"
#endif

#if !defined(LMICbandplan_updateTx)
# error "LMICbandplan_updateTx() not defined by bandplan"
#endif

#if !defined(LMICbandplan_nextJoinState)
# error "LMICbandplan_nextJoinState() not defined by bandplan"
#endif

#if !defined(LMICbandplan_initDefaultChannels)
# error "LMICbandplan_initDefaultChannels() not defined by bandplan"
#endif

#if !defined(LMICbandplan_nextJoinTime)
# error "LMICbandplan_nextJoinTime() not defined by bandplan"
#endif

#if !defined(LMICbandplan_init)
# error "LMICbandplan_init() not defined by bandplan"
#endif
//
// Things common to lmic.c code
//
#if !defined(MINRX_SYMS)
#define MINRX_SYMS 5
#endif // !defined(MINRX_SYMS)
#define PAMBL_SYMS 8
#define PAMBL_FSK  5
#define PRERX_FSK  1
#define RXLEN_FSK  (1+5+2)

#define BCN_INTV_osticks       sec2osticks(BCN_INTV_sec)
#define TXRX_GUARD_osticks     ms2osticks(TXRX_GUARD_ms)
#define JOIN_GUARD_osticks     ms2osticks(JOIN_GUARD_ms)
#define DELAY_JACC1_osticks    sec2osticks(DELAY_JACC1)
#define DELAY_JACC2_osticks    sec2osticks(DELAY_JACC2)
#define DELAY_EXTDNW2_osticks  sec2osticks(DELAY_EXTDNW2)
#define BCN_RESERVE_osticks    ms2osticks(BCN_RESERVE_ms)
#define BCN_GUARD_osticks      ms2osticks(BCN_GUARD_ms)
#define BCN_WINDOW_osticks     ms2osticks(BCN_WINDOW_ms)
#define AIRTIME_BCN_osticks    us2osticks(AIRTIME_BCN)

// Special APIs - for development or testing
#define isTESTMODE() 0

// internal APIs
ostime_t LMICcore_rndDelay(u1_t secSpan);
void LMICcore_setDrJoin(u1_t reason, u1_t dr);

#endif // _lmic_bandplan_h_
