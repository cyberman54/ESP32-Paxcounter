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

#define LMIC_DR_LEGACY 0

#include "lmic_bandplan.h"

#if CFG_LMIC_US_like

#ifndef LMICuslike_getFirst500kHzDR
# error "LMICuslike_getFirst500kHzDR() not defined by bandplan"
#endif

static void setNextChannel(uint start, uint end, uint count) {
        ASSERT(count>0);
        ASSERT(start<end);
        ASSERT(count <= (end - start));
        // We used to pick a random channel once and then just increment. That is not per spec.
        // Now we use a new random number each time, because they are not very expensive.
        // Regarding the algo below, we cannot pick a number and scan until we hit an enabled channel.
        // That would result in the first enabled channel following a set of disabled ones
        // being used more frequently than the other enabled channels.

        // Last used channel is in range. It is not a candidate, per spec.
        uint lastTxChan = LMIC.txChnl;
        if (start <= lastTxChan && lastTxChan<end &&
                // Adjust count only if still enabled. Otherwise, no chance of selection.
                ENABLED_CHANNEL(lastTxChan)) {
                --count;
                if (count == 0) {
                        return; // Only one active channel, so keep using it.
                }
        }

        uint nth = os_getRndU1() % count;
        for (u1_t chnl = start; chnl<end; chnl++) {
                // Scan for nth enabled channel that is not the last channel used
                if (chnl != lastTxChan && ENABLED_CHANNEL(chnl) && (nth--) == 0) {
                        LMIC.txChnl = chnl;
                        return;
                }
        }
        // No feasible channel found! Keep old one.
}



bit_t LMIC_setupBand(u1_t bandidx, s1_t txpow, u2_t txcap) {
        LMIC_API_PARAMETER(bandidx);
        LMIC_API_PARAMETER(txpow);
        LMIC_API_PARAMETER(txcap);

        // nothing; just succeed.
	return 1;
}


void LMICuslike_initDefaultChannels(bit_t fJoin) {
        LMIC_API_PARAMETER(fJoin);

        // things work the same for join as normal.
        for (u1_t i = 0; i<4; i++)
                LMIC.channelMap[i] = 0xFFFF;
        LMIC.channelMap[4] = 0x00FF;
        LMIC.activeChannels125khz = 64;
        LMIC.activeChannels500khz = 8;
}

u1_t LMICuslike_mapChannels(u1_t chpage, u2_t chmap) {
	/*
	|| MCMD_LADR_CHP_125ON and MCMD_LADR_CHP_125OFF are special. The
	|| channel map appllies to 500kHz (ch 64..71) and in addition
	|| all channels 0..63 are turned off or on.  MCMC_LADR_CHP_BANK
	|| is also special, in that it enables subbands.
	*/
	u1_t base, top;

	if (chpage < MCMD_LADR_CHP_USLIKE_SPECIAL) {
		// operate on channels 0..15, 16..31, 32..47, 48..63
		base = chpage << 4;
		top = base + 16;
		if (base == 64) {
			if (chmap & 0xFF00) {
				// those are reserved bits, fail.
				return 0;
			}
			top = 72;
		}
	} else if (chpage == MCMD_LADR_CHP_BANK) {
		if (chmap & 0xFF00) {
			// those are resreved bits, fail.
			return 0;
		}
		// each bit enables a bank of channels
		for (u1_t subband = 0; subband < 8; ++subband, chmap >>= 1) {
			if (chmap & 1) {
				LMIC_enableSubBand(subband);
			} else {
				LMIC_disableSubBand(subband);
			}

		// don't change any channels below
		base = top = 0;
		}
	} else if (chpage == MCMD_LADR_CHP_125ON || chpage == MCMD_LADR_CHP_125OFF) {
                u1_t const en125 = chpage == MCMD_LADR_CHP_125ON;

		// enable or disable all 125kHz channels
		for (u1_t chnl = 0; chnl < 64; ++chnl) {
			if (en125)
				LMIC_enableChannel(chnl);
			else
				LMIC_disableChannel(chnl);
		}

		// then apply mask to top 8 channels.
		base = 64;
		top = 72;
	} else {
		return 0;
	}

	// apply chmap to channels in [base..top-1].
	// Use enable/disable channel to keep activeChannel counts in sync.
	for (u1_t chnl = base; chnl < top; ++chnl, chmap >>= 1) {
		if (chmap & 0x0001)
			LMIC_enableChannel(chnl);
		else
			LMIC_disableChannel(chnl);
        }
        return 1;
}

// US does not have duty cycling - return now as earliest TX time
// but also do the channel hopping dance.
ostime_t LMICuslike_nextTx(ostime_t now) {
        // TODO(tmm@mcci.com): use a static const for US-like
        if (LMIC.datarate >= LMICuslike_getFirst500kHzDR()) { // 500kHz
                ASSERT(LMIC.activeChannels500khz>0);
                setNextChannel(64, 64 + 8, LMIC.activeChannels500khz);
        }
        else { // 125kHz
                ASSERT(LMIC.activeChannels125khz>0);
                setNextChannel(0, 64, LMIC.activeChannels125khz);
        }
        return now;
}

#if !defined(DISABLE_JOIN)
void LMICuslike_initJoinLoop(void) {
        // set an initial condition so that setNextChannel()'s preconds are met
        LMIC.txChnl = 0;

        // then chose a new channel.  This gives us a random first channel for
        // the join. Minor nit: if channel 0 is enabled, it will never be used
        // as the first join channel.  The join logic uses the current txChnl,
        // then changes after the rx window expires; so we need to set a valid
        // starting point.
        setNextChannel(0, 64, LMIC.activeChannels125khz);

        // initialize the adrTxPower.
        // TODO(tmm@mcci.com): is this right for all US-like regions
        LMIC.adrTxPow = 20; // dBm
        ASSERT((LMIC.opmode & OP_NEXTCHNL) == 0);

        // make sure LMIC.txend is valid.
        LMIC.txend = os_getTime();

        // make sure the datarate is set to DR0 per LoRaWAN regional reqts V1.0.2,
        // section 2.2.2
        // TODO(tmm@mcci.com): parameterize this for US-like
        LMICcore_setDrJoin(DRCHG_SET, LORAWAN_DR0);

        // TODO(tmm@mcci.com) need to implement the transmit randomization and
        // duty cycle restrictions from LoRaWAN V1.0.2 section 7.
}
#endif // !DISABLE_JOIN

#if !defined(DISABLE_JOIN)
//
// TODO(tmm@mcci.com):
//
// The definition of this is a little strange. this seems to return a time, but
// in reality it returns 0 if the caller should continue scanning through
// channels, and 1 if the caller has scanned all channels on this session,
// and therefore should reset to the beginning.  The IBM 1.6 code is the
// same way, so apparently I just carried this across. We should declare
// as bool_t and change callers to use the result clearly as a flag.
//
ostime_t LMICuslike_nextJoinState(void) {
        // Try the following:
        //   DR0 (SF10)  on a random channel 0..63
        //      (honoring enable mask)
        //   DR4 (SF8C)  on a random 500 kHz channel 64..71
        //      (always determined by
        //       previously selected
        //       125 kHz channel)
        //
        u1_t failed = 0;
        // TODO(tmm@mcci.com) parameterize for US-like
        if (LMIC.datarate != LMICuslike_getFirst500kHzDR()) {
                // assume that 500 kHz equiv of last 125 kHz channel
                // is also enabled, and use it next.
                LMIC.txChnl = 64 + (LMIC.txChnl >> 3);
                LMICcore_setDrJoin(DRCHG_SET, LMICuslike_getFirst500kHzDR());
        }
        else {
                setNextChannel(0, 64, LMIC.activeChannels125khz);

                // TODO(tmm@mcci.com) parameterize
                s1_t dr = LORAWAN_DR0;
                if ((++LMIC.txCnt & 0x7) == 0) {
                        failed = 1; // All DR exhausted - signal failed
                }
                LMICcore_setDrJoin(DRCHG_SET, dr);
        }
        LMIC.opmode &= ~OP_NEXTCHNL;

        // TODO(tmm@mcci.com): change delay to (0:1) secs + a known t0, but randomized;
        // starting adding a bias after 1 hour, 25 hours, etc.; and limit the duty
        // cycle on power up. For testability, add a way to set the join start time
        // externally (a test API) so we can check this feature.
        // See https://github.com/mcci-catena/arduino-lmic/issues/2
	// Current code doesn't match LoRaWAN 1.0.2 requirements.

        LMIC.txend = os_getTime() +
                (isTESTMODE()
                        // Avoid collision with JOIN ACCEPT being sent by GW (but we missed it - GW is still busy)
                        ? DNW2_SAFETY_ZONE
                        // Otherwise: randomize join (street lamp case):
                        // SF10:16, SF9=8,..SF8C:1secs
                        : LMICcore_rndDelay(16 >> LMIC.datarate));
        // 1 - triggers EV_JOIN_FAILED event
        return failed;
}
#endif

#endif // CFG_LMIC_US_like
