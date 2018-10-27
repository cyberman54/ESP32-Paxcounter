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

#ifndef _lorabase_eu868_h_
#define _lorabase_eu868_h_

#ifndef _LMIC_CONFIG_PRECONDITIONS_H_
# include "lmic_config_preconditions.h"
#endif

/****************************************************************************\
|
| Basic definitions for EU868 (always in scope)
|
\****************************************************************************/

//
// Default frequency plan for EU 868MHz ISM band
// data rates
// this is a little confusing: the integer values of these constants are the
// DataRates from the LoRaWAN Regional Parmaeter spec. The names are just
// convenient indications, so we can use them in the rare case that we need to
// choose a DataRate by SF and configuration, not by DR code.

enum _dr_eu868_t {
        EU868_DR_SF12 = 0,
        EU868_DR_SF11,
        EU868_DR_SF10,
        EU868_DR_SF9,
        EU868_DR_SF8,
        EU868_DR_SF7,
        EU868_DR_SF7B,
        EU868_DR_FSK,
        EU868_DR_NONE
};

// Bands:
//  g1 :   1%  14dBm
//  g2 : 0.1%  14dBm
//  g3 :  10%  27dBm
//                 freq             band     datarates
enum {
        EU868_F1 = 868100000,      // g1   SF7-12
        EU868_F2 = 868300000,      // g1   SF7-12 FSK SF7/250
        EU868_F3 = 868500000,      // g1   SF7-12
        EU868_F4 = 868850000,      // g2   SF7-12
        EU868_F5 = 869050000,      // g2   SF7-12
        EU868_F6 = 869525000,      // g3   SF7-12
        EU868_J4 = 864100000,      // g2   SF7-12  used during join
        EU868_J5 = 864300000,      // g2   SF7-12   ditto
        EU868_J6 = 864500000,      // g2   SF7-12   ditto
};
enum {
        EU868_FREQ_MIN = 863000000,
        EU868_FREQ_MAX = 870000000
};
enum {
        EU868_TX_EIRP_MAX_DBM = 16      // 16 dBm EIRP. So subtract 3 dBm for a 3 dBi antenna.
};

enum { EU868_LMIC_REGION_EIRP = 1 };         // region uses EIRP

enum { DR_PAGE_EU868 = 0x10 * (LMIC_REGION_eu868 - 1) };

#endif /* _lorabase_eu868_h_ */