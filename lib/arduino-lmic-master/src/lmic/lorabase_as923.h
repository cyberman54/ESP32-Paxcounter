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

#ifndef _lorabase_as923_h_
#define _lorabase_as923_h_

#ifndef _LMIC_CONFIG_PRECONDITIONS_H_
# include "lmic_config_preconditions.h"
#endif

/****************************************************************************\
|
| Basic definitions for AS923 (always in scope)
|
\****************************************************************************/

enum _dr_as923_t {
        AS923_DR_SF12 = 0,
        AS923_DR_SF11,
        AS923_DR_SF10,
        AS923_DR_SF9,
        AS923_DR_SF8,
        AS923_DR_SF7,
        AS923_DR_SF7B,
        AS923_DR_FSK,
        AS923_DR_NONE
};

// Bands:
//  g1 :   1%  16dBm
//                 freq                band     datarates
enum {
        AS923_F1    = 923200000,      // g1   SF7-12
        AS923_F2    = 923400000,      // g1   SF7-12
        AS923_FDOWN = 923200000,      //      (RX2 freq, DR2)
        AS923_FBCN  = 923400000,      //      default BCN, DR3
        AS923_FPING = 923400000,      //      default ping, DR3
};
enum {
        AS923_FREQ_MIN = 915000000,
        AS923_FREQ_MAX = 928000000
};
enum {
        AS923_TX_EIRP_MAX_DBM = 16      // 16 dBm
};
enum { DR_PAGE_AS923 = 0x10 * (LMIC_REGION_as923 - 1) };

enum { AS923_LMIC_REGION_EIRP = 1 };         // region uses EIRP

enum { AS923JP_LBT_US = 5000 };         // microseconds of LBT time -- 5000 ==>
					// 5 ms. We use us rather than ms for
					// future 128us support, and just for
					// backward compatibility -- there
					// is code that uses the _US constant,
					// and it's awkward to break it.

enum { AS923JP_LBT_DB_MAX = -80 };      // maximum channel strength in dB; if TX
					// we measure more than this, we don't tx.

// AS923 v1.1, all channels face a 1% duty cycle. So this will have to change
// in the future via a config. But this code base needs major changes for
// v1.1 in any case.
enum { AS923_V102_TX_CAP = 100 };		// v1.0.2 allows 100%

#ifndef AS923_TX_CAP
# define AS923_TX_CAP	AS923_V102_TX_CAP
#endif

#endif /* _lorabase_as923_h_ */
