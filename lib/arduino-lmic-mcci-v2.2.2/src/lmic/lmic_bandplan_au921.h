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

#ifndef _lmic_au921_h_
# define _lmic_au921_h_

// preconditions for lmic_us_like.h
#define LMICuslike_getFirst500kHzDR()   (AU921_DR_SF8C)


#ifndef _lmic_us_like_h_
# include "lmic_us_like.h"
#endif

uint8_t LMICau921_maxFrameLen(uint8_t dr);
#define maxFrameLen(dr) LMICau921_maxFrameLen(dr)

#define pow2dBm(mcmd_ladr_p1) ((s1_t)(30 - (((mcmd_ladr_p1)&MCMD_LADR_POW_MASK)<<1)))

ostime_t LMICau921_dr2hsym(uint8_t dr);
#define dr2hsym(dr) LMICau921_dr2hsym(dr)


#define LMICbandplan_getInitialDrJoin() (EU868_DR_SF7)

void LMICau921_setBcnRxParams(void);
#define LMICbandplan_setBcnRxParams() LMICau921_setBcnRxParams()

u4_t LMICau921_convFreq(xref2cu1_t ptr);
#define LMICbandplan_convFreq(ptr)      LMICau921_convFreq(ptr)

void LMICau921_setRx1Params(void);
#define LMICbandplan_setRx1Params()     LMICau921_setRx1Params()

void LMICau921_updateTx(ostime_t txbeg);
#define LMICbandplan_updateTx(txbeg)    LMICau921_updateTx(txbeg)

#endif // _lmic_au921_h_
