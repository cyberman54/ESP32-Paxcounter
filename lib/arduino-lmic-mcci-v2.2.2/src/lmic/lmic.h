/*
 * Copyright (c) 2014-2016 IBM Corporation.
 * Copyright (c) 2016 Matthijs Kooijman.
 * Copyright (c) 2016-2018 MCCI Corporation.
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

//! @file
//! @brief LMIC API

#ifndef _lmic_h_
#define _lmic_h_

#include "oslmic.h"
#include "lorabase.h"

#if LMIC_DEBUG_LEVEL > 0 || LMIC_X_DEBUG_LEVEL > 0
# if defined(LMIC_DEBUG_INCLUDE)
#   define LMIC_STRINGIFY_(x) #x
#   define LMIC_STRINGIFY(x) LMIC_STRINGIFY_(x)
#   include LMIC_STRINGIFY(LMIC_DEBUG_INCLUDE)
# endif
#  ifdef LMIC_DEBUG_PRINTF_FN
     extern void LMIC_DEBUG_PRINTF_FN(const char *f, ...);
#  endif // ndef LMIC_DEBUG_PRINTF_FN
#endif

// if LMIC_DEBUG_PRINTF is now defined, just use it. This lets you do anything
// you like with a sufficiently crazy header file.
#if LMIC_DEBUG_LEVEL > 0
# ifndef LMIC_DEBUG_PRINTF
//  otherwise, check whether someone configured a print-function to be used,
//  and use it if so.
#   ifdef LMIC_DEBUG_PRINTF_FN
#     define LMIC_DEBUG_PRINTF(f, ...) LMIC_DEBUG_PRINTF_FN(f, ## __VA_ARGS__)
#     ifndef LMIC_DEBUG_INCLUDE // If you use LMIC_DEBUG_INCLUDE, put the declaration in there
        void LMIC_DEBUG_PRINTF_FN(const char *f, ...);
#     endif // ndef LMIC_DEBUG_INCLUDE
#   else // ndef LMIC_DEBUG_PRINTF_FN
//    if there's no other info, just use printf. In a pure Arduino environment,
//    that's what will happen.
#     include <stdio.h>
#     define LMIC_DEBUG_PRINTF(f, ...) printf(f, ## __VA_ARGS__)
#   endif // ndef LMIC_DEBUG_PRINTF_FN
# endif // ndef LMIC_DEBUG_PRINTF
# ifndef LMIC_DEBUG_FLUSH
#   ifdef LMIC_DEBUG_FLUSH_FN
#     define LMIC_DEBUG_FLUSH() LMIC_DEBUG_FLUSH_FN()
#   else // ndef LMIC_DEBUG_FLUSH_FN
//    if there's no other info, assume that flush is not needed.
#     define LMIC_DEBUG_FLUSH() do { ; } while (0)
#   endif // ndef LMIC_DEBUG_FLUSH_FN
# endif // ndef LMIC_DEBUG_FLUSH
#else // LMIC_DEBUG_LEVEL == 0
// If debug level is zero, printf and flush expand to nothing.
# define LMIC_DEBUG_PRINTF(f, ...)      do { ; } while (0)
# define LMIC_DEBUG_FLUSH()             do { ; } while (0)
#endif // LMIC_DEBUG_LEVEL == 0

//
// LMIC_X_DEBUG_LEVEL enables additional, special print functions for debugging
// RSSI features. This is used sparingly.
#if LMIC_X_DEBUG_LEVEL > 0
#  ifdef LMIC_DEBUG_PRINTF_FN
#    define LMIC_X_DEBUG_PRINTF(f, ...) LMIC_DEBUG_PRINTF_FN(f, ## __VA_ARGS__)
#  else
#    error "LMIC_DEBUG_PRINTF_FN must be defined for LMIC_X_DEBUG_LEVEL > 0."
#  endif
#else
#  define LMIC_X_DEBUG_PRINTF(f, ...)  do {;} while(0)
#endif

#ifdef __cplusplus
extern "C"{
#endif

// LMIC version -- this is ths IBM LMIC version
#define LMIC_VERSION_MAJOR 1
#define LMIC_VERSION_MINOR 6
#define LMIC_VERSION_BUILD 1468577746

// Arduino LMIC version
#define ARDUINO_LMIC_VERSION_CALC(major, minor, patch, local)	\
	(((major) << 24u) | ((minor) << 16u) | ((patch) << 8u) | (local))

#define	ARDUINO_LMIC_VERSION	ARDUINO_LMIC_VERSION_CALC(2, 2, 2, 0)	/* v2.2.2 */

#define	ARDUINO_LMIC_VERSION_GET_MAJOR(v)	\
	(((v) >> 24u) & 0xFFu)

#define	ARDUINO_LMIC_VERSION_GET_MINOR(v)	\
	(((v) >> 16u) & 0xFFu)

#define	ARDUINO_LMIC_VERSION_GET_PATCH(v)	\
	(((v) >> 8u) & 0xFFu)

#define	ARDUINO_LMIC_VERSION_GET_LOCAL(v)	\
	((v) & 0xFFu)

//! Only For Antenna Tuning Tests !
//#define CFG_TxContinuousMode 1

enum { MAX_FRAME_LEN      =  64 };   //!< Library cap on max frame length
enum { TXCONF_ATTEMPTS    =   8 };   //!< Transmit attempts for confirmed frames
enum { MAX_MISSED_BCNS    =  20 };   // threshold for triggering rejoin requests
enum { MAX_RXSYMS         = 100 };   // stop tracking beacon beyond this

enum { LINK_CHECK_CONT    =  12 ,    // continue with this after reported dead link
       LINK_CHECK_DEAD    =  24 ,    // after this UP frames and no response from NWK assume link is dead
       LINK_CHECK_INIT    = -12 ,    // UP frame count until we inc datarate
       LINK_CHECK_OFF     =-128 };   // link check disabled

enum { TIME_RESYNC        = 6*128 }; // secs
enum { TXRX_GUARD_ms      =  6000 };  // msecs - don't start TX-RX transaction before beacon
enum { JOIN_GUARD_ms      =  9000 };  // msecs - don't start Join Req/Acc transaction before beacon
enum { TXRX_BCNEXT_secs   =     2 };  // secs - earliest start after beacon time
enum { RETRY_PERIOD_secs  =     3 };  // secs - random period for retrying a confirmed send

#if CFG_LMIC_EU_like // EU868 spectrum ====================================================

enum { MAX_CHANNELS = 16 };      //!< Max supported channels
enum { MAX_BANDS    =  4 };

enum { LIMIT_CHANNELS = (1<<4) };   // EU868 will never have more channels
//! \internal
struct band_t {
    u2_t     txcap;     // duty cycle limitation: 1/txcap
    s1_t     txpow;     // maximum TX power
    u1_t     lastchnl;  // last used channel
    ostime_t avail;     // channel is blocked until this time
};
TYPEDEF_xref2band_t; //!< \internal

#elif CFG_LMIC_US_like  // US915 spectrum =================================================

enum { MAX_XCHANNELS = 2 };      // extra channels in RAM, channels 0-71 are immutable

#endif // ==========================================================================

// Keep in sync with evdefs.hpp::drChange
enum { DRCHG_SET, DRCHG_NOJACC, DRCHG_NOACK, DRCHG_NOADRACK, DRCHG_NWKCMD };
enum { KEEP_TXPOW = -128 };


#if !defined(DISABLE_PING)
//! \internal
struct rxsched_t {
    u1_t     dr;
    u1_t     intvExp;   // 0..7
    u1_t     slot;      // runs from 0 to 128
    u1_t     rxsyms;
    ostime_t rxbase;
    ostime_t rxtime;    // start of next spot
    u4_t     freq;
};
TYPEDEF_xref2rxsched_t;  //!< \internal
#endif // !DISABLE_PING


#if !defined(DISABLE_BEACONS)
//! Parsing and tracking states of beacons.
enum { BCN_NONE    = 0x00,   //!< No beacon received
       BCN_PARTIAL = 0x01,   //!< Only first (common) part could be decoded (info,lat,lon invalid/previous)
       BCN_FULL    = 0x02,   //!< Full beacon decoded
       BCN_NODRIFT = 0x04,   //!< No drift value measured yet
       BCN_NODDIFF = 0x08 }; //!< No differential drift measured yet
//! Information about the last and previous beacons.
struct bcninfo_t {
    ostime_t txtime;  //!< Time when the beacon was sent
    s1_t     rssi;    //!< Adjusted RSSI value of last received beacon
    s1_t     snr;     //!< Scaled SNR value of last received beacon
    u1_t     flags;   //!< Last beacon reception and tracking states. See BCN_* values.
    u4_t     time;    //!< GPS time in seconds of last beacon (received or surrogate)
    //
    u1_t     info;    //!< Info field of last beacon (valid only if BCN_FULL set)
    s4_t     lat;     //!< Lat field of last beacon (valid only if BCN_FULL set)
    s4_t     lon;     //!< Lon field of last beacon (valid only if BCN_FULL set)
};
#endif // !DISABLE_BEACONS

// purpose of receive window - lmic_t.rxState
enum { RADIO_RST=0, RADIO_TX=1, RADIO_RX=2, RADIO_RXON=3 };
// Netid values /  lmic_t.netid
enum { NETID_NONE=(int)~0U, NETID_MASK=(int)0xFFFFFF };
// MAC operation modes (lmic_t.opmode).
enum { OP_NONE     = 0x0000,
       OP_SCAN     = 0x0001, // radio scan to find a beacon
       OP_TRACK    = 0x0002, // track my networks beacon (netid)
       OP_JOINING  = 0x0004, // device joining in progress (blocks other activities)
       OP_TXDATA   = 0x0008, // TX user data (buffered in pendTxData)
       OP_POLL     = 0x0010, // send empty UP frame to ACK confirmed DN/fetch more DN data
       OP_REJOIN   = 0x0020, // occasionally send JOIN REQUEST
       OP_SHUTDOWN = 0x0040, // prevent MAC from doing anything
       OP_TXRXPEND = 0x0080, // TX/RX transaction pending
       OP_RNDTX    = 0x0100, // prevent TX lining up after a beacon
       OP_PINGINI  = 0x0200, // pingable is initialized and scheduling active
       OP_PINGABLE = 0x0400, // we're pingable
       OP_NEXTCHNL = 0x0800, // find a new channel
       OP_LINKDEAD = 0x1000, // link was reported as dead
       OP_TESTMODE = 0x2000, // developer test mode
};
// TX-RX transaction flags - report back to user
enum { TXRX_ACK    = 0x80,   // confirmed UP frame was acked
       TXRX_NACK   = 0x40,   // confirmed UP frame was not acked
       TXRX_NOPORT = 0x20,   // set if a frame with a port was RXed, clr if no frame/no port
       TXRX_PORT   = 0x10,   // set if a frame with a port was RXed, LMIC.frame[LMIC.dataBeg-1] => port
       TXRX_DNW1   = 0x01,   // received in 1st DN slot
       TXRX_DNW2   = 0x02,   // received in 2dn DN slot
       TXRX_PING   = 0x04 }; // received in a scheduled RX slot
// Event types for event callback
enum _ev_t { EV_SCAN_TIMEOUT=1, EV_BEACON_FOUND,
             EV_BEACON_MISSED, EV_BEACON_TRACKED, EV_JOINING,
             EV_JOINED, EV_RFU1, EV_JOIN_FAILED, EV_REJOIN_FAILED,
             EV_TXCOMPLETE, EV_LOST_TSYNC, EV_RESET,
             EV_RXCOMPLETE, EV_LINK_DEAD, EV_LINK_ALIVE, EV_SCAN_FOUND,
             EV_TXSTART };
typedef enum _ev_t ev_t;

enum {
        // This value represents 100% error in LMIC.clockError
        MAX_CLOCK_ERROR = 65536,
};

// network time request callback function
// defined unconditionally, because APIs and types can't change based on config.
// This is called when a time-request succeeds or when we get a downlink
// without time request, "completing" the pending time request.
typedef void lmic_request_network_time_cb_t(void *pUserData, int flagSuccess);

// how the network represents time.
typedef u4_t lmic_gpstime_t;

// rather than deal with 1/256 second tick, we adjust ostime back
// (as it's high res) to match tNetwork.
typedef struct lmic_time_reference_s lmic_time_reference_t;

struct lmic_time_reference_s {
    // our best idea of when we sent the uplink (end of packet).
    ostime_t tLocal;
    // the network's best idea of when we sent the uplink.
    lmic_gpstime_t tNetwork;
};

enum lmic_request_time_state_e {
    lmic_RequestTimeState_idle = 0,     // we're not doing anything
    lmic_RequestTimeState_tx,           // we want to tx a time request on next uplink
    lmic_RequestTimeState_rx,           // we have tx'ed, next downlink completes.
    lmic_RequestTimeState_success       // we sucessfully got time.
};

typedef u1_t lmic_request_time_state_t;

struct lmic_t {
    // Radio settings TX/RX (also accessed by HAL)
    ostime_t    txend;
    ostime_t    rxtime;

    // LBT info
    ostime_t    lbt_ticks;      // ticks to listen
    s1_t        lbt_dbmax;      // max permissible dB on our channel (eg -80)

    u4_t        freq;
    s1_t        rssi;
    s1_t        snr;            // LMIC.snr is SNR times 4
    rps_t       rps;
    u1_t        rxsyms;
    u1_t        dndr;
    s1_t        txpow;     // dBm

    osjob_t     osjob;

    // Channel scheduling
#if CFG_LMIC_EU_like
    band_t      bands[MAX_BANDS];
    u4_t        channelFreq[MAX_CHANNELS];
    u2_t        channelDrMap[MAX_CHANNELS];
    u2_t        channelMap;
#elif CFG_LMIC_US_like
    u4_t        xchFreq[MAX_XCHANNELS];    // extra channel frequencies (if device is behind a repeater)
    u2_t        xchDrMap[MAX_XCHANNELS];   // extra channel datarate ranges  ---XXX: ditto
    u2_t        channelMap[(72+MAX_XCHANNELS+15)/16];  // enabled bits
    u2_t        activeChannels125khz;
    u2_t        activeChannels500khz;
#endif
    u1_t        txChnl;          // channel for next TX
    u1_t        globalDutyRate;  // max rate: 1/2^k
    ostime_t    globalDutyAvail; // time device can send again

    u4_t        netid;        // current network id (~0 - none)
    u2_t        opmode;
    u1_t        upRepeat;     // configured up repeat
    s1_t        adrTxPow;     // ADR adjusted TX power
    u1_t        datarate;     // current data rate
    u1_t        errcr;        // error coding rate (used for TX only)
    u1_t        rejoinCnt;    // adjustment for rejoin datarate
#if !defined(DISABLE_BEACONS)
    s2_t        drift;        // last measured drift
    s2_t        lastDriftDiff;
    s2_t        maxDriftDiff;
#endif

    u2_t        clockError; // Inaccuracy in the clock. CLOCK_ERROR_MAX
                            // represents +/-100% error

    u1_t        pendTxPort;
    u1_t        pendTxConf;   // confirmed data
    u1_t        pendTxLen;    // +0x80 = confirmed
    u1_t        pendTxData[MAX_LEN_PAYLOAD];

    u2_t        devNonce;     // last generated nonce
    u1_t        nwkKey[16];   // network session key
    u1_t        artKey[16];   // application router session key
    devaddr_t   devaddr;
    u4_t        seqnoDn;      // device level down stream seqno
    u4_t        seqnoUp;
#if LMIC_ENABLE_DeviceTimeReq
    // put here for alignment, to reduce RAM use.
    ostime_t    localDeviceTime;    // the LMIC.txend value for last DeviceTimeAns
    lmic_gpstime_t netDeviceTime;   // the netDeviceTime for lastDeviceTimeAns
                                    // zero ==> not valid.
    lmic_request_network_time_cb_t *pNetworkTimeCb;	// call-back routine
    void        *pNetworkTimeUserData; // call-back data
#endif // LMIC_ENABLE_DeviceTimeReq

    u1_t        dnConf;       // dn frame confirm pending: LORA::FCT_ACK or 0
    s1_t        adrAckReq;    // counter until we reset data rate (0=off)
    u1_t        adrChanged;

    u1_t        rxDelay;      // Rx delay after TX

    u1_t        margin;
    bit_t       ladrAns;      // link adr adapt answer pending
    bit_t       devsAns;      // device status answer pending
    s1_t        devAnsMargin; // SNR value between -32 and 31 (inclusive) for the last successfully received DevStatusReq command
    u1_t        adrEnabled;
    u1_t        moreData;     // NWK has more data pending
#if !defined(DISABLE_MCMD_DCAP_REQ)
    bit_t       dutyCapAns;   // have to ACK duty cycle settings
#endif
#if !defined(DISABLE_MCMD_SNCH_REQ)
    u1_t        snchAns;      // answer set new channel
#endif
#if LMIC_ENABLE_TxParamSetupReq
    bit_t       txParamSetupAns; // transmit setup answer pending.
    u1_t        txParam;        // the saved TX param byte.
#endif
#if LMIC_ENABLE_DeviceTimeReq
    lmic_request_time_state_t txDeviceTimeReqState;  // current state, initially idle.
    u1_t        netDeviceTimeFrac;     // updated on any DeviceTimeAns.
#endif

    // rx1DrOffset is the offset from uplink to downlink datarate
    u1_t        rx1DrOffset;  // captured from join. zero by default.

    // 2nd RX window (after up stream)
    u1_t        dn2Dr;
    u4_t        dn2Freq;
#if !defined(DISABLE_MCMD_DN2P_SET)
    u1_t        dn2Ans;       // 0=no answer pend, 0x80+ACKs
#endif

    // Class B state
#if !defined(DISABLE_BEACONS)
    u1_t        missedBcns;   // unable to track last N beacons
    u1_t        bcninfoTries; // how often to try (scan mode only)
#endif
#if !defined(DISABLE_MCMD_PING_SET) && !defined(DISABLE_PING)
    u1_t        pingSetAns;   // answer set cmd and ACK bits
#endif
#if !defined(DISABLE_PING)
    rxsched_t   ping;         // pingable setup
#endif

    // Public part of MAC state
    u1_t        txCnt;
    u1_t        txrxFlags;  // transaction flags (TX-RX combo)
    u1_t        dataBeg;    // 0 or start of data (dataBeg-1 is port)
    u1_t        dataLen;    // 0 no data or zero length data, >0 byte count of data
    u1_t        frame[MAX_LEN_FRAME];

#if !defined(DISABLE_BEACONS)
    u1_t        bcnChnl;
    u1_t        bcnRxsyms;    //
    ostime_t    bcnRxtime;
    bcninfo_t   bcninfo;      // Last received beacon info
#endif

    u1_t        noRXIQinversion;
};

//! \var struct lmic_t LMIC
//! The state of LMIC MAC layer is encapsulated in this variable.
DECLARE_LMIC; //!< \internal

//! Construct a bit map of allowed datarates from drlo to drhi (both included).
#define DR_RANGE_MAP(drlo,drhi) (((u2_t)0xFFFF<<(drlo)) & ((u2_t)0xFFFF>>(15-(drhi))))
bit_t LMIC_setupBand (u1_t bandidx, s1_t txpow, u2_t txcap);
bit_t LMIC_setupChannel (u1_t channel, u4_t freq, u2_t drmap, s1_t band);
void  LMIC_disableChannel (u1_t channel);
void  LMIC_enableSubBand(u1_t band);
void  LMIC_enableChannel(u1_t channel);
void  LMIC_disableSubBand(u1_t band);
void  LMIC_selectSubBand(u1_t band);

void  LMIC_setDrTxpow   (dr_t dr, s1_t txpow);  // set default/start DR/txpow
void  LMIC_setAdrMode   (bit_t enabled);        // set ADR mode (if mobile turn off)
#if !defined(DISABLE_JOIN)
bit_t LMIC_startJoining (void);
#endif

void  LMIC_shutdown     (void);
void  LMIC_init         (void);
void  LMIC_reset        (void);
void  LMIC_clrTxData    (void);
void  LMIC_setTxData    (void);
int   LMIC_setTxData2   (u1_t port, xref2u1_t data, u1_t dlen, u1_t confirmed);
void  LMIC_sendAlive    (void);

#if !defined(DISABLE_BEACONS)
bit_t LMIC_enableTracking  (u1_t tryBcnInfo);
void  LMIC_disableTracking (void);
#endif

#if !defined(DISABLE_PING)
void  LMIC_stopPingable  (void);
void  LMIC_setPingable   (u1_t intvExp);
#endif
#if !defined(DISABLE_JOIN)
void  LMIC_tryRejoin     (void);
#endif

void LMIC_setSession (u4_t netid, devaddr_t devaddr, xref2u1_t nwkKey, xref2u1_t artKey);
void LMIC_setLinkCheckMode (bit_t enabled);
void LMIC_setClockError(u2_t error);

u4_t LMIC_getSeqnoUp    (void);
u4_t LMIC_setSeqnoUp    (u4_t);
void LMIC_getSessionKeys (u4_t *netid, devaddr_t *devaddr, xref2u1_t nwkKey, xref2u1_t artKey);

void LMIC_requestNetworkTime(lmic_request_network_time_cb_t *pCallbackfn, void *pUserData);
int LMIC_getNetworkTimeReference(lmic_time_reference_t *pReference);

// Declare onEvent() function, to make sure any definition will have the
// C conventions, even when in a C++ file.
DECL_ON_LMIC_EVENT;



// Special APIs - for development or testing
// !!!See implementation for caveats!!!

#ifdef __cplusplus
} // extern "C"
#endif

#endif // _lmic_h_
