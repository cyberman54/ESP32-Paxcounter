/*
* Copyright (c) 2014-2016 IBM Corporation.
* All rights reserved.
*
* Copyright (c) 2017 MCCI Corporation
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

#ifndef _lorabase_us915_h_
#define _lorabase_us915_h_

#ifndef _LMIC_CONFIG_PRECONDITIONS_H_
# include "lmic_config_preconditions.h"
#endif

/****************************************************************************\
|
| Basic definitions for US915 (always in scope)
|
\****************************************************************************/

// Frequency plan for US 915MHz ISM band
// data rates
enum _dr_us915_t {
        US915_DR_SF10 = 0,
        US915_DR_SF9,
        US915_DR_SF8,
        US915_DR_SF7,
        US915_DR_SF8C,
        US915_DR_NONE,
        // Devices "behind a router" (and upper half of DR list):
        US915_DR_SF12CR = 8,
        US915_DR_SF11CR,
        US915_DR_SF10CR,
        US915_DR_SF9CR,
        US915_DR_SF8CR,
        US915_DR_SF7CR
};

// Default frequency plan for US 915MHz
enum {
        US915_125kHz_UPFBASE = 902300000,
        US915_125kHz_UPFSTEP = 200000,
        US915_500kHz_UPFBASE = 903000000,
        US915_500kHz_UPFSTEP = 1600000,
        US915_500kHz_DNFBASE = 923300000,
        US915_500kHz_DNFSTEP = 600000
};
enum {
        US915_FREQ_MIN = 902000000,
        US915_FREQ_MAX = 928000000
};
enum {
        US915_TX_MAX_DBM = 30           // 30 dBm (but not EIRP): assumes we're 
                                        // on an 64-channel bandplan. See code
                                        // that computes tx power.
};
enum { DR_PAGE_US915 = 0x10 * (LMIC_REGION_us915 - 1) };

enum { US915_LMIC_REGION_EIRP = 0 };         // region doesn't use EIRP, uses tx power

#endif /* _lorabase_us915_h_ */