/*******************************************************************************
 * Copyright (c) 2014-2015 IBM Corporation.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *    IBM Zurich Research Lab - initial API, implementation and documentation
 *******************************************************************************/

//! \file
#include "lmic.h"

#if defined(DISABLE_BEACONS) && !defined(DISABLE_PING)
#error Ping needs beacon tracking
#endif

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
#if defined(CFG_eu868)
#define DNW2_SAFETY_ZONE       ms2osticks(3000)
#endif
#if defined(CFG_us915)
#define DNW2_SAFETY_ZONE       ms2osticks(750)
#endif

// Special APIs - for development or testing
#define isTESTMODE() 0

DEFINE_LMIC;


// Fwd decls.
static void engineUpdate(void);
static void startScan (void);


// ================================================================================
// BEG OS - default implementations for certain OS suport functions

#if !defined(HAS_os_calls)

#if !defined(os_rlsbf2)
u2_t os_rlsbf2 (xref2cu1_t buf) {
    return (u2_t)((u2_t)buf[0] | ((u2_t)buf[1]<<8));
}
#endif

#if !defined(os_rlsbf4)
u4_t os_rlsbf4 (xref2cu1_t buf) {
    return (u4_t)((u4_t)buf[0] | ((u4_t)buf[1]<<8) | ((u4_t)buf[2]<<16) | ((u4_t)buf[3]<<24));
}
#endif


#if !defined(os_rmsbf4)
u4_t os_rmsbf4 (xref2cu1_t buf) {
    return (u4_t)((u4_t)buf[3] | ((u4_t)buf[2]<<8) | ((u4_t)buf[1]<<16) | ((u4_t)buf[0]<<24));
}
#endif


#if !defined(os_wlsbf2)
void os_wlsbf2 (xref2u1_t buf, u2_t v) {
    buf[0] = v;
    buf[1] = v>>8;
}
#endif

#if !defined(os_wlsbf4)
void os_wlsbf4 (xref2u1_t buf, u4_t v) {
    buf[0] = v;
    buf[1] = v>>8;
    buf[2] = v>>16;
    buf[3] = v>>24;
}
#endif

#if !defined(os_wmsbf4)
void os_wmsbf4 (xref2u1_t buf, u4_t v) {
    buf[3] = v;
    buf[2] = v>>8;
    buf[1] = v>>16;
    buf[0] = v>>24;
}
#endif

#if !defined(os_getBattLevel)
u1_t os_getBattLevel (void) {
    return MCMD_DEVS_BATT_NOINFO;
}
#endif

#if !defined(os_crc16)
// New CRC-16 CCITT(XMODEM) checksum for beacons:
u2_t os_crc16 (xref2u1_t data, uint len) {
    u2_t remainder = 0;
    u2_t polynomial = 0x1021;
    for( uint i = 0; i < len; i++ ) {
        remainder ^= data[i] << 8;
        for( u1_t bit = 8; bit > 0; bit--) {
            if( (remainder & 0x8000) )
                remainder = (remainder << 1) ^ polynomial;
            else
                remainder <<= 1;
        }
    }
    return remainder;
}
#endif

#endif // !HAS_os_calls

// END OS - default implementations for certain OS suport functions
// ================================================================================

// ================================================================================
// BEG AES

static void micB0 (u4_t devaddr, u4_t seqno, int dndir, int len) {
    os_clearMem(AESaux,16);
    AESaux[0]  = 0x49;
    AESaux[5]  = dndir?1:0;
    AESaux[15] = len;
    os_wlsbf4(AESaux+ 6,devaddr);
    os_wlsbf4(AESaux+10,seqno);
}


static int aes_verifyMic (xref2cu1_t key, u4_t devaddr, u4_t seqno, int dndir, xref2u1_t pdu, int len) {
    micB0(devaddr, seqno, dndir, len);
    os_copyMem(AESkey,key,16);
    return os_aes(AES_MIC, pdu, len) == os_rmsbf4(pdu+len);
}


static void aes_appendMic (xref2cu1_t key, u4_t devaddr, u4_t seqno, int dndir, xref2u1_t pdu, int len) {
    micB0(devaddr, seqno, dndir, len);
    os_copyMem(AESkey,key,16);
    // MSB because of internal structure of AES
    os_wmsbf4(pdu+len, os_aes(AES_MIC, pdu, len));
}


static void aes_appendMic0 (xref2u1_t pdu, int len) {
    os_getDevKey(AESkey);
    os_wmsbf4(pdu+len, os_aes(AES_MIC|AES_MICNOAUX, pdu, len));  // MSB because of internal structure of AES
}


static int aes_verifyMic0 (xref2u1_t pdu, int len) {
    os_getDevKey(AESkey);
    return os_aes(AES_MIC|AES_MICNOAUX, pdu, len) == os_rmsbf4(pdu+len);
}


static void aes_encrypt (xref2u1_t pdu, int len) {
    os_getDevKey(AESkey);
    os_aes(AES_ENC, pdu, len);
}


static void aes_cipher (xref2cu1_t key, u4_t devaddr, u4_t seqno, int dndir, xref2u1_t payload, int len) {
    if( len <= 0 )
        return;
    os_clearMem(AESaux, 16);
    AESaux[0] = AESaux[15] = 1; // mode=cipher / dir=down / block counter=1
    AESaux[5] = dndir?1:0;
    os_wlsbf4(AESaux+ 6,devaddr);
    os_wlsbf4(AESaux+10,seqno);
    os_copyMem(AESkey,key,16);
    os_aes(AES_CTR, payload, len);
}


static void aes_sessKeys (u2_t devnonce, xref2cu1_t artnonce, xref2u1_t nwkkey, xref2u1_t artkey) {
    os_clearMem(nwkkey, 16);
    nwkkey[0] = 0x01;
    os_copyMem(nwkkey+1, artnonce, LEN_ARTNONCE+LEN_NETID);
    os_wlsbf2(nwkkey+1+LEN_ARTNONCE+LEN_NETID, devnonce);
    os_copyMem(artkey, nwkkey, 16);
    artkey[0] = 0x02;

    os_getDevKey(AESkey);
    os_aes(AES_ENC, nwkkey, 16);
    os_getDevKey(AESkey);
    os_aes(AES_ENC, artkey, 16);
}

// END AES
// ================================================================================


// ================================================================================
// BEG LORA

#if defined(CFG_eu868) // ========================================

#define maxFrameLen(dr) ((dr)<=DR_SF9 ? TABLE_GET_U1(maxFrameLens, (dr)) : 0xFF)
CONST_TABLE(u1_t, maxFrameLens) [] = { 64,64,64,123 };

CONST_TABLE(u1_t, _DR2RPS_CRC)[] = {
    ILLEGAL_RPS,
    (u1_t)MAKERPS(SF12, BW125, CR_4_5, 0, 0),
    (u1_t)MAKERPS(SF11, BW125, CR_4_5, 0, 0),
    (u1_t)MAKERPS(SF10, BW125, CR_4_5, 0, 0),
    (u1_t)MAKERPS(SF9,  BW125, CR_4_5, 0, 0),
    (u1_t)MAKERPS(SF8,  BW125, CR_4_5, 0, 0),
    (u1_t)MAKERPS(SF7,  BW125, CR_4_5, 0, 0),
    (u1_t)MAKERPS(SF7,  BW250, CR_4_5, 0, 0),
    (u1_t)MAKERPS(FSK,  BW125, CR_4_5, 0, 0),
    ILLEGAL_RPS
};

static CONST_TABLE(s1_t, TXPOWLEVELS)[] = {
    20, 14, 11, 8, 5, 2, 0,0, 0,0,0,0, 0,0,0,0
};
#define pow2dBm(mcmd_ladr_p1) (TABLE_GET_S1(TXPOWLEVELS, (mcmd_ladr_p1&MCMD_LADR_POW_MASK)>>MCMD_LADR_POW_SHIFT))

#elif defined(CFG_us915) // ========================================

#define maxFrameLen(dr) ((dr)<=DR_SF11CR ? TABLE_GET_U1(maxFrameLens, (dr)) : 0xFF)
CONST_TABLE(u1_t, maxFrameLens) [] = { 24,66,142,255,255,255,255,255,  66,142 };

CONST_TABLE(u1_t, _DR2RPS_CRC)[] = {
    ILLEGAL_RPS,
    MAKERPS(SF10, BW125, CR_4_5, 0, 0),
    MAKERPS(SF9 , BW125, CR_4_5, 0, 0),
    MAKERPS(SF8 , BW125, CR_4_5, 0, 0),
    MAKERPS(SF7 , BW125, CR_4_5, 0, 0),
    MAKERPS(SF8 , BW500, CR_4_5, 0, 0),
    ILLEGAL_RPS ,
    ILLEGAL_RPS ,
    ILLEGAL_RPS ,
    MAKERPS(SF12, BW500, CR_4_5, 0, 0),
    MAKERPS(SF11, BW500, CR_4_5, 0, 0),
    MAKERPS(SF10, BW500, CR_4_5, 0, 0),
    MAKERPS(SF9 , BW500, CR_4_5, 0, 0),
    MAKERPS(SF8 , BW500, CR_4_5, 0, 0),
    MAKERPS(SF7 , BW500, CR_4_5, 0, 0),
    ILLEGAL_RPS
};

#define pow2dBm(mcmd_ladr_p1) ((s1_t)(30 - (((mcmd_ladr_p1)&MCMD_LADR_POW_MASK)<<1)))

#endif // ================================================

static CONST_TABLE(u1_t, SENSITIVITY)[7][3] = {
    // ------------bw----------
    // 125kHz    250kHz    500kHz
    { 141-109,  141-109, 141-109 },  // FSK
    { 141-127,  141-124, 141-121 },  // SF7
    { 141-129,  141-126, 141-123 },  // SF8
    { 141-132,  141-129, 141-126 },  // SF9
    { 141-135,  141-132, 141-129 },  // SF10
    { 141-138,  141-135, 141-132 },  // SF11
    { 141-141,  141-138, 141-135 }   // SF12
};

int getSensitivity (rps_t rps) {
    return -141 + TABLE_GET_U1_TWODIM(SENSITIVITY, getSf(rps), getBw(rps));
}

ostime_t calcAirTime (rps_t rps, u1_t plen) {
    u1_t bw = getBw(rps);  // 0,1,2 = 125,250,500kHz
    u1_t sf = getSf(rps);  // 0=FSK, 1..6 = SF7..12
    if( sf == FSK ) {
        return (plen+/*preamble*/5+/*syncword*/3+/*len*/1+/*crc*/2) * /*bits/byte*/8
            * (s4_t)OSTICKS_PER_SEC / /*kbit/s*/50000;
    }
    u1_t sfx = 4*(sf+(7-SF7));
    u1_t q = sfx - (sf >= SF11 ? 8 : 0);
    int tmp = 8*plen - sfx + 28 + (getNocrc(rps)?0:16) - (getIh(rps)?20:0);
    if( tmp > 0 ) {
        tmp = (tmp + q - 1) / q;
        tmp *= getCr(rps)+5;
        tmp += 8;
    } else {
        tmp = 8;
    }
    tmp = (tmp<<2) + /*preamble*/49 /* 4 * (8 + 4.25) */;
    // bw = 125000 = 15625 * 2^3
    //      250000 = 15625 * 2^4
    //      500000 = 15625 * 2^5
    // sf = 7..12
    //
    // osticks =  tmp * OSTICKS_PER_SEC * 1<<sf / bw
    //
    // 3 => counter reduced divisor 125000/8 => 15625
    // 2 => counter 2 shift on tmp
    sfx = sf+(7-SF7) - (3+2) - bw;
    int div = 15625;
    if( sfx > 4 ) {
        // prevent 32bit signed int overflow in last step
        div >>= sfx-4;
        sfx = 4;
    }
    // Need 32bit arithmetic for this last step
    return (((ostime_t)tmp << sfx) * OSTICKS_PER_SEC + div/2) / div;
}

extern inline rps_t updr2rps (dr_t dr);
extern inline rps_t dndr2rps (dr_t dr);
extern inline int isFasterDR (dr_t dr1, dr_t dr2);
extern inline int isSlowerDR (dr_t dr1, dr_t dr2);
extern inline dr_t  incDR    (dr_t dr);
extern inline dr_t  decDR    (dr_t dr);
extern inline dr_t  assertDR (dr_t dr);
extern inline dr_t  validDR  (dr_t dr);
extern inline dr_t  lowerDR  (dr_t dr, u1_t n);

extern inline sf_t  getSf    (rps_t params);
extern inline rps_t setSf    (rps_t params, sf_t sf);
extern inline bw_t  getBw    (rps_t params);
extern inline rps_t setBw    (rps_t params, bw_t cr);
extern inline cr_t  getCr    (rps_t params);
extern inline rps_t setCr    (rps_t params, cr_t cr);
extern inline int   getNocrc (rps_t params);
extern inline rps_t setNocrc (rps_t params, int nocrc);
extern inline int   getIh    (rps_t params);
extern inline rps_t setIh    (rps_t params, int ih);
extern inline rps_t makeRps  (sf_t sf, bw_t bw, cr_t cr, int ih, int nocrc);
extern inline int   sameSfBw (rps_t r1, rps_t r2);

// END LORA
// ================================================================================


// Adjust DR for TX retries
//  - indexed by retry count
//  - return steps to lower DR
static CONST_TABLE(u1_t, DRADJUST)[2+TXCONF_ATTEMPTS] = {
    // normal frames - 1st try / no retry
    0,
    // confirmed frames
    0,0,1,0,1,0,1,0,0
};


// Table below defines the size of one symbol as
//   symtime = 256us * 2^T(sf,bw)
// 256us is called one symunit.
//                 SF:
//      BW:      |__7___8___9__10__11__12
//      125kHz   |  2   3   4   5   6   7
//      250kHz   |  1   2   3   4   5   6
//      500kHz   |  0   1   2   3   4   5
//
// Times for half symbol per DR
// Per DR table to minimize rounding errors
static CONST_TABLE(ostime_t, DR2HSYM_osticks)[] = {
#if defined(CFG_eu868)
#define dr2hsym(dr) (TABLE_GET_OSTIME(DR2HSYM_osticks, (dr)))
    us2osticksRound(128<<7),  // DR_SF12
    us2osticksRound(128<<6),  // DR_SF11
    us2osticksRound(128<<5),  // DR_SF10
    us2osticksRound(128<<4),  // DR_SF9
    us2osticksRound(128<<3),  // DR_SF8
    us2osticksRound(128<<2),  // DR_SF7
    us2osticksRound(128<<1),  // DR_SF7B
    us2osticksRound(80)       // FSK -- not used (time for 1/2 byte)
#elif defined(CFG_us915)
#define dr2hsym(dr) (TABLE_GET_OSTIME(DR2HSYM_osticks, (dr)&7))  // map DR_SFnCR -> 0-6
    us2osticksRound(128<<5),  // DR_SF10   DR_SF12CR
    us2osticksRound(128<<4),  // DR_SF9    DR_SF11CR
    us2osticksRound(128<<3),  // DR_SF8    DR_SF10CR
    us2osticksRound(128<<2),  // DR_SF7    DR_SF9CR
    us2osticksRound(128<<1),  // DR_SF8C   DR_SF8CR
    us2osticksRound(128<<0)   // ------    DR_SF7CR
#endif
};


#if !defined(DISABLE_BEACONS)
static ostime_t calcRxWindow (u1_t secs, dr_t dr) {
    ostime_t rxoff, err;
    if( secs==0 ) {
        // aka 128 secs (next becaon)
        rxoff = LMIC.drift;
        err = LMIC.lastDriftDiff;
    } else {
        // scheduled RX window within secs into current beacon period
        rxoff = (LMIC.drift * (ostime_t)secs) >> BCN_INTV_exp;
        err = (LMIC.lastDriftDiff * (ostime_t)secs) >> BCN_INTV_exp;
    }
    u1_t rxsyms = MINRX_SYMS;
    err += (ostime_t)LMIC.maxDriftDiff * LMIC.missedBcns;
    LMIC.rxsyms = MINRX_SYMS + (err / dr2hsym(dr));

    return (rxsyms-PAMBL_SYMS) * dr2hsym(dr) + rxoff;
}


// Setup beacon RX parameters assuming we have an error of ms (aka +/-(ms/2))
static void calcBcnRxWindowFromMillis (u1_t ms, bit_t ini) {
    if( ini ) {
        LMIC.drift = 0;
        LMIC.maxDriftDiff = 0;
        LMIC.missedBcns = 0;
        LMIC.bcninfo.flags |= BCN_NODRIFT|BCN_NODDIFF;
    }
    ostime_t hsym = dr2hsym(DR_BCN);
    LMIC.bcnRxsyms = MINRX_SYMS + ms2osticksCeil(ms) / hsym;
    LMIC.bcnRxtime = LMIC.bcninfo.txtime + BCN_INTV_osticks - (LMIC.bcnRxsyms-PAMBL_SYMS) * hsym;
}
#endif // !DISABLE_BEACONS


#if !defined(DISABLE_PING)
// Setup scheduled RX window (ping/multicast slot)
static void rxschedInit (xref2rxsched_t rxsched) {
    os_clearMem(AESkey,16);
    os_clearMem(LMIC.frame+8,8);
    os_wlsbf4(LMIC.frame, LMIC.bcninfo.time);
    os_wlsbf4(LMIC.frame+4, LMIC.devaddr);
    os_aes(AES_ENC,LMIC.frame,16);
    u1_t intvExp = rxsched->intvExp;
    ostime_t off = os_rlsbf2(LMIC.frame) & (0x0FFF >> (7 - intvExp)); // random offset (slot units)
    rxsched->rxbase = (LMIC.bcninfo.txtime +
                       BCN_RESERVE_osticks +
                       ms2osticks(BCN_SLOT_SPAN_ms * off)); // random offset osticks
    rxsched->slot   = 0;
    rxsched->rxtime = rxsched->rxbase - calcRxWindow(/*secs BCN_RESERVE*/2+(1<<intvExp),rxsched->dr);
    rxsched->rxsyms = LMIC.rxsyms;
}


static bit_t rxschedNext (xref2rxsched_t rxsched, ostime_t cando) {
  again:
    if( rxsched->rxtime - cando >= 0 )
        return 1;
    u1_t slot;
    if( (slot=rxsched->slot) >= 128 )
        return 0;
    u1_t intv = 1<<rxsched->intvExp;
    if( (rxsched->slot = (slot += (intv))) >= 128 )
        return 0;
    rxsched->rxtime = rxsched->rxbase
        + ((BCN_WINDOW_osticks * (ostime_t)slot) >> BCN_INTV_exp)
        - calcRxWindow(/*secs BCN_RESERVE*/2+slot+intv,rxsched->dr);
    rxsched->rxsyms = LMIC.rxsyms;
    goto again;
}
#endif // !DISABLE_PING)


static ostime_t rndDelay (u1_t secSpan) {
    u2_t r = os_getRndU2();
    ostime_t delay = r;
    if( delay > OSTICKS_PER_SEC )
        delay = r % (u2_t)OSTICKS_PER_SEC;
    if( secSpan > 0 )
        delay += ((u1_t)r % secSpan) * OSTICKS_PER_SEC;
    return delay;
}


static void txDelay (ostime_t reftime, u1_t secSpan) {
    reftime += rndDelay(secSpan);
    if( LMIC.globalDutyRate == 0  ||  (reftime - LMIC.globalDutyAvail) > 0 ) {
        LMIC.globalDutyAvail = reftime;
        LMIC.opmode |= OP_RNDTX;
    }
}


static void setDrJoin (u1_t reason, u1_t dr) {
    EV(drChange, INFO, (e_.reason    = reason,
                        e_.deveui    = MAIN::CDEV->getEui(),
                        e_.dr        = dr|DR_PAGE,
                        e_.txpow     = LMIC.adrTxPow,
                        e_.prevdr    = LMIC.datarate|DR_PAGE,
                        e_.prevtxpow = LMIC.adrTxPow));
    LMIC.datarate = dr;
    DO_DEVDB(LMIC.datarate,datarate);
}


static void setDrTxpow (u1_t reason, u1_t dr, s1_t pow) {
    EV(drChange, INFO, (e_.reason    = reason,
                        e_.deveui    = MAIN::CDEV->getEui(),
                        e_.dr        = dr|DR_PAGE,
                        e_.txpow     = pow,
                        e_.prevdr    = LMIC.datarate|DR_PAGE,
                        e_.prevtxpow = LMIC.adrTxPow));

    if( pow != KEEP_TXPOW )
        LMIC.adrTxPow = pow;
    if( LMIC.datarate != dr ) {
        LMIC.datarate = dr;
        DO_DEVDB(LMIC.datarate,datarate);
        LMIC.opmode |= OP_NEXTCHNL;
    }
}


#if !defined(DISABLE_PING)
void LMIC_stopPingable (void) {
    LMIC.opmode &= ~(OP_PINGABLE|OP_PINGINI);
}


void LMIC_setPingable (u1_t intvExp) {
    // Change setting
    LMIC.ping.intvExp = (intvExp & 0x7);
    LMIC.opmode |= OP_PINGABLE;
    // App may call LMIC_enableTracking() explicitely before
    // Otherwise tracking is implicitly enabled here
    if( (LMIC.opmode & (OP_TRACK|OP_SCAN)) == 0  &&  LMIC.bcninfoTries == 0 )
        LMIC_enableTracking(0);
}

#endif // !DISABLE_PING

#if defined(CFG_eu868)
// ================================================================================
//
// BEG: EU868 related stuff
//
enum { NUM_DEFAULT_CHANNELS=3 };
static CONST_TABLE(u4_t, iniChannelFreq)[6] = {
    // Join frequencies and duty cycle limit (0.1%)
    EU868_F1|BAND_MILLI, EU868_F2|BAND_MILLI, EU868_F3|BAND_MILLI,
    // Default operational frequencies
    EU868_F1|BAND_CENTI, EU868_F2|BAND_CENTI, EU868_F3|BAND_CENTI,
};

static void initDefaultChannels (bit_t join) {
    os_clearMem(&LMIC.channelFreq, sizeof(LMIC.channelFreq));
    os_clearMem(&LMIC.channelDrMap, sizeof(LMIC.channelDrMap));
    os_clearMem(&LMIC.bands, sizeof(LMIC.bands));

    LMIC.channelMap = 0x07;
    u1_t su = join ? 0 : 3;
    for( u1_t fu=0; fu<3; fu++,su++ ) {
        LMIC.channelFreq[fu]  = TABLE_GET_U4(iniChannelFreq, su);
        LMIC.channelDrMap[fu] = DR_RANGE_MAP(DR_SF12,DR_SF7);
    }

    LMIC.bands[BAND_MILLI].txcap    = 1000;  // 0.1%
    LMIC.bands[BAND_MILLI].txpow    = 14;
    LMIC.bands[BAND_MILLI].lastchnl = os_getRndU1() % MAX_CHANNELS;
    LMIC.bands[BAND_CENTI].txcap    = 100;   // 1%
    LMIC.bands[BAND_CENTI].txpow    = 14;
    LMIC.bands[BAND_CENTI].lastchnl = os_getRndU1() % MAX_CHANNELS;
    LMIC.bands[BAND_DECI ].txcap    = 10;    // 10%
    LMIC.bands[BAND_DECI ].txpow    = 27;
    LMIC.bands[BAND_DECI ].lastchnl = os_getRndU1() % MAX_CHANNELS;
    LMIC.bands[BAND_MILLI].avail =
    LMIC.bands[BAND_CENTI].avail =
    LMIC.bands[BAND_DECI ].avail = os_getTime();
}

bit_t LMIC_setupBand (u1_t bandidx, s1_t txpow, u2_t txcap) {
    if( bandidx > BAND_AUX ) return 0;
    band_t* b = &LMIC.bands[bandidx];
    b->txpow = txpow;
    b->txcap = txcap;
    b->avail = os_getTime();
    b->lastchnl = os_getRndU1() % MAX_CHANNELS;
    return 1;
}

bit_t LMIC_setupChannel (u1_t chidx, u4_t freq, u2_t drmap, s1_t band) {
    if( chidx >= MAX_CHANNELS )
        return 0;
    if( band == -1 ) {
        if( freq >= 869400000 && freq <= 869650000 )
            freq |= BAND_DECI;   // 10% 27dBm
        else if( (freq >= 868000000 && freq <= 868600000) ||
                 (freq >= 869700000 && freq <= 870000000)  )
            freq |= BAND_CENTI;  // 1% 14dBm
        else
            freq |= BAND_MILLI;  // 0.1% 14dBm
    } else {
        if( band > BAND_AUX ) return 0;
        freq = (freq&~3) | band;
    }
    LMIC.channelFreq [chidx] = freq;
    LMIC.channelDrMap[chidx] = drmap==0 ? DR_RANGE_MAP(DR_SF12,DR_SF7) : drmap;
    LMIC.channelMap |= 1<<chidx;  // enabled right away
    return 1;
}

void LMIC_disableChannel (u1_t channel) {
    LMIC.channelFreq[channel] = 0;
    LMIC.channelDrMap[channel] = 0;
    LMIC.channelMap &= ~(1<<channel);
}

static u4_t convFreq (xref2u1_t ptr) {
    u4_t freq = (os_rlsbf4(ptr-1) >> 8) * 100;
    if( freq < EU868_FREQ_MIN || freq > EU868_FREQ_MAX )
        freq = 0;
    return freq;
}

static u1_t mapChannels (u1_t chpage, u2_t chmap) {
    // Bad page, disable all channel, enable non-existent
    if( chpage != 0 || chmap==0 || (chmap & ~LMIC.channelMap) != 0 )
        return 0;  // illegal input
    for( u1_t chnl=0; chnl<MAX_CHANNELS; chnl++ ) {
        if( (chmap & (1<<chnl)) != 0 && LMIC.channelFreq[chnl] == 0 )
            chmap &= ~(1<<chnl); // ignore - channel is not defined
    }
    LMIC.channelMap = chmap;
    return 1;
}


static void updateTx (ostime_t txbeg) {
    u4_t freq = LMIC.channelFreq[LMIC.txChnl];
    // Update global/band specific duty cycle stats
    ostime_t airtime = calcAirTime(LMIC.rps, LMIC.dataLen);
    // Update channel/global duty cycle stats
    xref2band_t band = &LMIC.bands[freq & 0x3];
    LMIC.freq  = freq & ~(u4_t)3;
    LMIC.txpow = band->txpow;
    band->avail = txbeg + airtime * band->txcap;
    if( LMIC.globalDutyRate != 0 )
        LMIC.globalDutyAvail = txbeg + (airtime<<LMIC.globalDutyRate);
    #if LMIC_DEBUG_LEVEL > 1
        lmic_printf("%lu: Updating info for TX at %lu, airtime will be %lu. Setting available time for band %d to %lu\n", os_getTime(), txbeg, airtime, freq & 0x3, band->avail);
        if( LMIC.globalDutyRate != 0 )
            lmic_printf("%lu: Updating global duty avail to %lu\n", os_getTime(), LMIC.globalDutyAvail);
    #endif
}

static ostime_t nextTx (ostime_t now) {
    u1_t bmap=0xF;
    do {
        ostime_t mintime = now + /*8h*/sec2osticks(28800);
        u1_t band=0;
        for( u1_t bi=0; bi<4; bi++ ) {
            if( (bmap & (1<<bi)) && mintime - LMIC.bands[bi].avail > 0 ) {
                #if LMIC_DEBUG_LEVEL > 1
                    lmic_printf("%lu: Considering band %d, which is available at %lu\n", os_getTime(), bi, LMIC.bands[bi].avail);
                #endif
                mintime = LMIC.bands[band = bi].avail;
            }
        }
        // Find next channel in given band
        u1_t chnl = LMIC.bands[band].lastchnl;
        for( u1_t ci=0; ci<MAX_CHANNELS; ci++ ) {
            if( (chnl = (chnl+1)) >= MAX_CHANNELS )
                chnl -=  MAX_CHANNELS;
            if( (LMIC.channelMap & (1<<chnl)) != 0  &&  // channel enabled
                (LMIC.channelDrMap[chnl] & (1<<(LMIC.datarate&0xF))) != 0  &&
                band == (LMIC.channelFreq[chnl] & 0x3) ) { // in selected band
                LMIC.txChnl = LMIC.bands[band].lastchnl = chnl;
                return mintime;
            }
        }
        #if LMIC_DEBUG_LEVEL > 1
            lmic_printf("%lu: No channel found in band %d\n", os_getTime(), band);
        #endif
        if( (bmap &= ~(1<<band)) == 0 ) {
            // No feasible channel  found!
            return mintime;
        }
    } while(1);
}


#if !defined(DISABLE_BEACONS)
static void setBcnRxParams (void) {
    LMIC.dataLen = 0;
    LMIC.freq = LMIC.channelFreq[LMIC.bcnChnl] & ~(u4_t)3;
    LMIC.rps  = setIh(setNocrc(dndr2rps((dr_t)DR_BCN),1),LEN_BCN);
}
#endif // !DISABLE_BEACONS

#define setRx1Params() /*LMIC.freq/rps remain unchanged*/

#if !defined(DISABLE_JOIN)
static void initJoinLoop (void) {
    LMIC.txChnl = os_getRndU1() % 3;
    LMIC.adrTxPow = 14;
    setDrJoin(DRCHG_SET, DR_SF7);
    initDefaultChannels(1);
    ASSERT((LMIC.opmode & OP_NEXTCHNL)==0);
    LMIC.txend = LMIC.bands[BAND_MILLI].avail + rndDelay(8);
}


static ostime_t nextJoinState (void) {
    u1_t failed = 0;

    // Try 869.x and then 864.x with same DR
    // If both fail try next lower datarate
    if( ++LMIC.txChnl == 3 )
        LMIC.txChnl = 0;
    if( (++LMIC.txCnt & 1) == 0 ) {
        // Lower DR every 2nd try (having tried 868.x and 864.x with the same DR)
        if( LMIC.datarate == DR_SF12 )
            failed = 1; // we have tried all DR - signal EV_JOIN_FAILED
        else
            setDrJoin(DRCHG_NOJACC, decDR((dr_t)LMIC.datarate));
    }
    // Clear NEXTCHNL because join state engine controls channel hopping
    LMIC.opmode &= ~OP_NEXTCHNL;
    // Move txend to randomize synchronized concurrent joins.
    // Duty cycle is based on txend.
    ostime_t time = os_getTime();
    if( time - LMIC.bands[BAND_MILLI].avail < 0 )
        time = LMIC.bands[BAND_MILLI].avail;
    LMIC.txend = time +
        (isTESTMODE()
         // Avoid collision with JOIN ACCEPT @ SF12 being sent by GW (but we missed it)
         ? DNW2_SAFETY_ZONE
         // Otherwise: randomize join (street lamp case):
         // SF12:255, SF11:127, .., SF7:8secs
         : DNW2_SAFETY_ZONE+rndDelay(255>>LMIC.datarate));
    #if LMIC_DEBUG_LEVEL > 1
        if (failed)
            lmic_printf("%lu: Join failed\n", os_getTime());
        else
            lmic_printf("%lu: Scheduling next join at %lu\n", os_getTime(), LMIC.txend);
    #endif
    // 1 - triggers EV_JOIN_FAILED event
    return failed;
}
#endif // !DISABLE_JOIN

//
// END: EU868 related stuff
//
// ================================================================================
#elif defined(CFG_us915)
// ================================================================================
//
// BEG: US915 related stuff
//


static void initDefaultChannels (void) {
    for( u1_t i=0; i<4; i++ )
        LMIC.channelMap[i] = 0xFFFF;
    LMIC.channelMap[4] = 0x00FF;
}

static u4_t convFreq (xref2u1_t ptr) {
    u4_t freq = (os_rlsbf4(ptr-1) >> 8) * 100;
    if( freq < US915_FREQ_MIN || freq > US915_FREQ_MAX )
        freq = 0;
    return freq;
}

bit_t LMIC_setupChannel (u1_t chidx, u4_t freq, u2_t drmap, s1_t band) {
    if( chidx < 72 || chidx >= 72+MAX_XCHANNELS )
        return 0; // channels 0..71 are hardwired
    chidx -= 72;
    LMIC.xchFreq[chidx] = freq;
    LMIC.xchDrMap[chidx] = drmap==0 ? DR_RANGE_MAP(DR_SF10,DR_SF8C) : drmap;
    LMIC.channelMap[chidx>>4] |= (1<<(chidx&0xF));
    return 1;
}

void LMIC_disableChannel (u1_t channel) {
    if( channel < 72+MAX_XCHANNELS )
        LMIC.channelMap[channel>>4] &= ~(1<<(channel&0xF));
}

void LMIC_enableChannel (u1_t channel) {
    if( channel < 72+MAX_XCHANNELS )
        LMIC.channelMap[channel>>4] |= (1<<(channel&0xF));
}

void  LMIC_enableSubBand (u1_t band) {
  ASSERT(band < 8);
  u1_t start = band * 8;
  u1_t end = start + 8;
  for (int channel=start; channel < end; ++channel )
      LMIC_enableChannel(channel);
}
void  LMIC_disableSubBand (u1_t band) {
  ASSERT(band < 8);
  u1_t start = band * 8;
  u1_t end = start + 8;
  for (int channel=start; channel < end; ++channel )
      LMIC_disableChannel(channel);
}
void  LMIC_selectSubBand (u1_t band) {
  ASSERT(band < 8);
  for (int b=0; b<8; ++b) {
    if (band==b)
      LMIC_enableSubBand(b);
    else
      LMIC_disableSubBand(b);
  }
}

static u1_t mapChannels (u1_t chpage, u2_t chmap) {
    if( chpage == MCMD_LADR_CHP_125ON || chpage == MCMD_LADR_CHP_125OFF ) {
        u2_t en125 = chpage == MCMD_LADR_CHP_125ON ? 0xFFFF : 0x0000;
        for( u1_t u=0; u<4; u++ )
            LMIC.channelMap[u] = en125;
        LMIC.channelMap[64/16] = chmap;
    } else {
        if( chpage >= (72+MAX_XCHANNELS+15)/16 )
            return 0;
        LMIC.channelMap[chpage] = chmap;
    }
    return 1;
}

static void updateTx (ostime_t txbeg) {
    u1_t chnl = LMIC.txChnl;
    if( chnl < 64 ) {
        LMIC.freq = US915_125kHz_UPFBASE + chnl*US915_125kHz_UPFSTEP;
        LMIC.txpow = 30;
        return;
    }
    LMIC.txpow = 26;
    if( chnl < 64+8 ) {
        LMIC.freq = US915_500kHz_UPFBASE + (chnl-64)*US915_500kHz_UPFSTEP;
    } else {
        ASSERT(chnl < 64+8+MAX_XCHANNELS);
        LMIC.freq = LMIC.xchFreq[chnl-72];
    }

    // Update global duty cycle stats
    if( LMIC.globalDutyRate != 0 ) {
        ostime_t airtime = calcAirTime(LMIC.rps, LMIC.dataLen);
        LMIC.globalDutyAvail = txbeg + (airtime<<LMIC.globalDutyRate);
    }
}

// US does not have duty cycling - return now as earliest TX time
#define nextTx(now) (_nextTx(),(now))
static void _nextTx (void) {
    if( LMIC.chRnd==0 )
        LMIC.chRnd = os_getRndU1() & 0x3F;
    if( LMIC.datarate >= DR_SF8C ) { // 500kHz
        u1_t map = LMIC.channelMap[64/16]&0xFF;
        for( u1_t i=0; i<8; i++ ) {
            if( (map & (1<<(++LMIC.chRnd & 7))) != 0 ) {
                LMIC.txChnl = 64 + (LMIC.chRnd & 7);
                return;
            }
        }
    } else { // 125kHz
        for( u1_t i=0; i<64; i++ ) {
            u1_t chnl = ++LMIC.chRnd & 0x3F;
            if( (LMIC.channelMap[(chnl >> 4)] & (1<<(chnl & 0xF))) != 0 ) {
                LMIC.txChnl = chnl;
                return;
            }
        }
    }
    // No feasible channel  found! Keep old one.
}

#if !defined(DISABLE_BEACONS)
static void setBcnRxParams (void) {
    LMIC.dataLen = 0;
    LMIC.freq = US915_500kHz_DNFBASE + LMIC.bcnChnl * US915_500kHz_DNFSTEP;
    LMIC.rps  = setIh(setNocrc(dndr2rps((dr_t)DR_BCN),1),LEN_BCN);
}
#endif // !DISABLE_BEACONS

#define setRx1Params() {                                                \
    LMIC.freq = US915_500kHz_DNFBASE + (LMIC.txChnl & 0x7) * US915_500kHz_DNFSTEP; \
    if( /* TX datarate */LMIC.dndr < DR_SF8C )                          \
        LMIC.dndr += DR_SF10CR - DR_SF10;                               \
    else if( LMIC.dndr == DR_SF8C )                                     \
        LMIC.dndr = DR_SF7CR;                                           \
    LMIC.rps = dndr2rps(LMIC.dndr);                                     \
}

#if !defined(DISABLE_JOIN)
static void initJoinLoop (void) {
    LMIC.chRnd = 0;
    LMIC.txChnl = 0;
    LMIC.adrTxPow = 20;
    ASSERT((LMIC.opmode & OP_NEXTCHNL)==0);
    LMIC.txend = os_getTime();
    setDrJoin(DRCHG_SET, DR_SF7);
}

static ostime_t nextJoinState (void) {
    // Try the following:
    //   SF7/8/9/10  on a random channel 0..63
    //   SF8C        on a random channel 64..71
    //
    u1_t failed = 0;
    if( LMIC.datarate != DR_SF8C ) {
        LMIC.txChnl = 64+(LMIC.txChnl&7);
        setDrJoin(DRCHG_SET, DR_SF8C);
    } else {
        LMIC.txChnl = os_getRndU1() & 0x3F;
        s1_t dr = DR_SF7 - ++LMIC.txCnt;
        if( dr < DR_SF10 ) {
            dr = DR_SF10;
            failed = 1; // All DR exhausted - signal failed
        }
        setDrJoin(DRCHG_SET, dr);
    }
    LMIC.opmode &= ~OP_NEXTCHNL;
    LMIC.txend = os_getTime() +
        (isTESTMODE()
         // Avoid collision with JOIN ACCEPT being sent by GW (but we missed it - GW is still busy)
         ? DNW2_SAFETY_ZONE
         // Otherwise: randomize join (street lamp case):
         // SF10:16, SF9=8,..SF8C:1secs
         : rndDelay(16>>LMIC.datarate));
    // 1 - triggers EV_JOIN_FAILED event
    return failed;
}
#endif // !DISABLE_JOIN

//
// END: US915 related stuff
//
// ================================================================================
#else
#error Unsupported frequency band!
#endif


static void runEngineUpdate (xref2osjob_t osjob) {
    engineUpdate();
}


static void reportEvent (ev_t ev) {
    EV(devCond, INFO, (e_.reason = EV::devCond_t::LMIC_EV,
                       e_.eui    = MAIN::CDEV->getEui(),
                       e_.info   = ev));
    ON_LMIC_EVENT(ev);
    engineUpdate();
}


static void runReset (xref2osjob_t osjob) {
    // Disable session
    LMIC_reset();
#if !defined(DISABLE_JOIN)
    LMIC_startJoining();
#endif // !DISABLE_JOIN
    reportEvent(EV_RESET);
}

static void stateJustJoined (void) {
    LMIC.seqnoDn     = LMIC.seqnoUp = 0;
    LMIC.rejoinCnt   = 0;
    LMIC.dnConf      = LMIC.adrChanged = LMIC.ladrAns = LMIC.devsAns = 0;
#if !defined(DISABLE_MCMD_SNCH_REQ)
    LMIC.snchAns     = 0;
#endif
#if !defined(DISABLE_MCMD_DN2P_SET)
    LMIC.dn2Ans      = 0;
#endif
    LMIC.moreData    = 0;
#if !defined(DISABLE_MCMD_DCAP_REQ)
    LMIC.dutyCapAns  = 0;
#endif
#if !defined(DISABLE_MCMD_PING_SET) && !defined(DISABLE_PING)
    LMIC.pingSetAns  = 0;
#endif
    LMIC.upRepeat    = 0;
    LMIC.adrAckReq   = LINK_CHECK_INIT;
    LMIC.dn2Dr       = DR_DNW2;
    LMIC.dn2Freq     = FREQ_DNW2;
#if !defined(DISABLE_BEACONS)
    LMIC.bcnChnl     = CHNL_BCN;
#endif
#if !defined(DISABLE_PING)
    LMIC.ping.freq   = FREQ_PING;
    LMIC.ping.dr     = DR_PING;
#endif
}


// ================================================================================
// Decoding frames


#if !defined(DISABLE_BEACONS)
// Decode beacon  - do not overwrite bcninfo unless we have a match!
static int decodeBeacon (void) {
    ASSERT(LMIC.dataLen == LEN_BCN); // implicit header RX guarantees this
    xref2u1_t d = LMIC.frame;
    if(
#if CFG_eu868
        d[OFF_BCN_CRC1] != (u1_t)os_crc16(d,OFF_BCN_CRC1)
#elif CFG_us915
        os_rlsbf2(&d[OFF_BCN_CRC1]) != os_crc16(d,OFF_BCN_CRC1)
#endif
        )
        return 0;   // first (common) part fails CRC check
    // First set of fields is ok
    u4_t bcnnetid = os_rlsbf4(&d[OFF_BCN_NETID]) & 0xFFFFFF;
    if( bcnnetid != LMIC.netid )
        return -1;  // not the beacon we're looking for

    LMIC.bcninfo.flags &= ~(BCN_PARTIAL|BCN_FULL);
    // Match - update bcninfo structure
    LMIC.bcninfo.snr    = LMIC.snr;
    LMIC.bcninfo.rssi   = LMIC.rssi;
    LMIC.bcninfo.txtime = LMIC.rxtime - AIRTIME_BCN_osticks;
    LMIC.bcninfo.time   = os_rlsbf4(&d[OFF_BCN_TIME]);
    LMIC.bcninfo.flags |= BCN_PARTIAL;

    // Check 2nd set
    if( os_rlsbf2(&d[OFF_BCN_CRC2]) != os_crc16(d,OFF_BCN_CRC2) )
        return 1;
    // Second set of fields is ok
    LMIC.bcninfo.lat    = (s4_t)os_rlsbf4(&d[OFF_BCN_LAT-1]) >> 8; // read as signed 24-bit
    LMIC.bcninfo.lon    = (s4_t)os_rlsbf4(&d[OFF_BCN_LON-1]) >> 8; // ditto
    LMIC.bcninfo.info   = d[OFF_BCN_INFO];
    LMIC.bcninfo.flags |= BCN_FULL;
    return 2;
}
#endif // !DISABLE_BEACONS


static bit_t decodeFrame (void) {
    xref2u1_t d = LMIC.frame;
    u1_t hdr    = d[0];
    u1_t ftype  = hdr & HDR_FTYPE;
    int  dlen   = LMIC.dataLen;
    const char *window = (LMIC.txrxFlags & TXRX_DNW1) ? "RX1" : ((LMIC.txrxFlags & TXRX_DNW2) ? "RX2" : "Other");
    if( dlen < OFF_DAT_OPTS+4 ||
        (hdr & HDR_MAJOR) != HDR_MAJOR_V1 ||
        (ftype != HDR_FTYPE_DADN  &&  ftype != HDR_FTYPE_DCDN) ) {
        // Basic sanity checks failed
        EV(specCond, WARN, (e_.reason = EV::specCond_t::UNEXPECTED_FRAME,
                            e_.eui    = MAIN::CDEV->getEui(),
                            e_.info   = dlen < 4 ? 0 : os_rlsbf4(&d[dlen-4]),
                            e_.info2  = hdr + (dlen<<8)));
      norx:
#if LMIC_DEBUG_LEVEL > 0
        lmic_printf("%lu: Invalid downlink, window=%s\n", os_getTime(), window);
#endif
        LMIC.dataLen = 0;
        return 0;
    }
    // Validate exact frame length
    // Note: device address was already read+evaluated in order to arrive here.
    int  fct   = d[OFF_DAT_FCT];
    u4_t addr  = os_rlsbf4(&d[OFF_DAT_ADDR]);
    u4_t seqno = os_rlsbf2(&d[OFF_DAT_SEQNO]);
    int  olen  = fct & FCT_OPTLEN;
    int  ackup = (fct & FCT_ACK) != 0 ? 1 : 0;   // ACK last up frame
    int  poff  = OFF_DAT_OPTS+olen;
    int  pend  = dlen-4;  // MIC

    if( addr != LMIC.devaddr ) {
        EV(specCond, WARN, (e_.reason = EV::specCond_t::ALIEN_ADDRESS,
                            e_.eui    = MAIN::CDEV->getEui(),
                            e_.info   = addr,
                            e_.info2  = LMIC.devaddr));
        goto norx;
    }
    if( poff > pend ) {
        EV(specCond, ERR, (e_.reason = EV::specCond_t::CORRUPTED_FRAME,
                           e_.eui    = MAIN::CDEV->getEui(),
                           e_.info   = 0x1000000 + (poff-pend) + (fct<<8) + (dlen<<16)));
        goto norx;
    }

    int port = -1;
    int replayConf = 0;

    if( pend > poff )
        port = d[poff++];

    seqno = LMIC.seqnoDn + (u2_t)(seqno - LMIC.seqnoDn);

    if( !aes_verifyMic(LMIC.nwkKey, LMIC.devaddr, seqno, /*dn*/1, d, pend) ) {
        EV(spe3Cond, ERR, (e_.reason = EV::spe3Cond_t::CORRUPTED_MIC,
                           e_.eui1   = MAIN::CDEV->getEui(),
                           e_.info1  = Base::lsbf4(&d[pend]),
                           e_.info2  = seqno,
                           e_.info3  = LMIC.devaddr));
        goto norx;
    }
    if( seqno < LMIC.seqnoDn ) {
        if( (s4_t)seqno > (s4_t)LMIC.seqnoDn ) {
            EV(specCond, INFO, (e_.reason = EV::specCond_t::DNSEQNO_ROLL_OVER,
                                e_.eui    = MAIN::CDEV->getEui(),
                                e_.info   = LMIC.seqnoDn,
                                e_.info2  = seqno));
            goto norx;
        }
        if( seqno != LMIC.seqnoDn-1 || !LMIC.dnConf || ftype != HDR_FTYPE_DCDN ) {
            EV(specCond, INFO, (e_.reason = EV::specCond_t::DNSEQNO_OBSOLETE,
                                e_.eui    = MAIN::CDEV->getEui(),
                                e_.info   = LMIC.seqnoDn,
                                e_.info2  = seqno));
            goto norx;
        }
        // Replay of previous sequence number allowed only if
        // previous frame and repeated both requested confirmation
        replayConf = 1;
    }
    else {
        if( seqno > LMIC.seqnoDn ) {
            EV(specCond, INFO, (e_.reason = EV::specCond_t::DNSEQNO_SKIP,
                                e_.eui    = MAIN::CDEV->getEui(),
                                e_.info   = LMIC.seqnoDn,
                                e_.info2  = seqno));
        }
        LMIC.seqnoDn = seqno+1;  // next number to be expected
        DO_DEVDB(LMIC.seqnoDn,seqnoDn);
        // DN frame requested confirmation - provide ACK once with next UP frame
        LMIC.dnConf = (ftype == HDR_FTYPE_DCDN ? FCT_ACK : 0);
    }

    if( LMIC.dnConf || (fct & FCT_MORE) )
        LMIC.opmode |= OP_POLL;

    // We heard from network
    LMIC.adrChanged = LMIC.rejoinCnt = 0;
    if( LMIC.adrAckReq != LINK_CHECK_OFF )
        LMIC.adrAckReq = LINK_CHECK_INIT;

    // Process OPTS
    int m = LMIC.rssi - RSSI_OFF - getSensitivity(LMIC.rps);
    LMIC.margin = m < 0 ? 0 : m > 254 ? 254 : m;

    xref2u1_t opts = &d[OFF_DAT_OPTS];
    int oidx = 0;
    while( oidx < olen ) {
        switch( opts[oidx] ) {
        case MCMD_LCHK_ANS: {
            //int gwmargin = opts[oidx+1];
            //int ngws = opts[oidx+2];
            oidx += 3;
            continue;
        }
        case MCMD_LADR_REQ: {
            u1_t p1     = opts[oidx+1];            // txpow + DR
            u2_t chmap  = os_rlsbf2(&opts[oidx+2]);// list of enabled channels
            u1_t chpage = opts[oidx+4] & MCMD_LADR_CHPAGE_MASK;     // channel page
            u1_t uprpt  = opts[oidx+4] & MCMD_LADR_REPEAT_MASK;     // up repeat count
            oidx += 5;

            LMIC.ladrAns = 0x80 |     // Include an answer into next frame up
                MCMD_LADR_ANS_POWACK | MCMD_LADR_ANS_CHACK | MCMD_LADR_ANS_DRACK;
            if( !mapChannels(chpage, chmap) )
                LMIC.ladrAns &= ~MCMD_LADR_ANS_CHACK;
            dr_t dr = (dr_t)(p1>>MCMD_LADR_DR_SHIFT);
            if( !validDR(dr) ) {
                LMIC.ladrAns &= ~MCMD_LADR_ANS_DRACK;
                EV(specCond, ERR, (e_.reason = EV::specCond_t::BAD_MAC_CMD,
                                   e_.eui    = MAIN::CDEV->getEui(),
                                   e_.info   = Base::lsbf4(&d[pend]),
                                   e_.info2  = Base::msbf4(&opts[oidx-4])));
            }
            if( (LMIC.ladrAns & 0x7F) == (MCMD_LADR_ANS_POWACK | MCMD_LADR_ANS_CHACK | MCMD_LADR_ANS_DRACK) ) {
                // Nothing went wrong - use settings
                LMIC.upRepeat = uprpt;
                setDrTxpow(DRCHG_NWKCMD, dr, pow2dBm(p1));
            }
            LMIC.adrChanged = 1;  // Trigger an ACK to NWK
            continue;
        }
        case MCMD_DEVS_REQ: {
            LMIC.devsAns = 1;
            oidx += 1;
            continue;
        }
        case MCMD_DN2P_SET: {
#if !defined(DISABLE_MCMD_DN2P_SET)
            dr_t dr = (dr_t)(opts[oidx+1] & 0x0F);
            u4_t freq = convFreq(&opts[oidx+2]);
            LMIC.dn2Ans = 0x80;   // answer pending
            if( validDR(dr) )
                LMIC.dn2Ans |= MCMD_DN2P_ANS_DRACK;
            if( freq != 0 )
                LMIC.dn2Ans |= MCMD_DN2P_ANS_CHACK;
            if( LMIC.dn2Ans == (0x80|MCMD_DN2P_ANS_DRACK|MCMD_DN2P_ANS_CHACK) ) {
                LMIC.dn2Dr = dr;
                LMIC.dn2Freq = freq;
                DO_DEVDB(LMIC.dn2Dr,dn2Dr);
                DO_DEVDB(LMIC.dn2Freq,dn2Freq);
            }
#endif // !DISABLE_MCMD_DN2P_SET
            oidx += 5;
            continue;
        }
        case MCMD_DCAP_REQ: {
#if !defined(DISABLE_MCMD_DCAP_REQ)
            u1_t cap = opts[oidx+1];
            // A value cap=0xFF means device is OFF unless enabled again manually.
            if( cap==0xFF )
                LMIC.opmode |= OP_SHUTDOWN;  // stop any sending
            LMIC.globalDutyRate  = cap & 0xF;
            LMIC.globalDutyAvail = os_getTime();
            DO_DEVDB(cap,dutyCap);
            LMIC.dutyCapAns = 1;
            oidx += 2;
#endif // !DISABLE_MCMD_DCAP_REQ
            continue;
        }
        case MCMD_SNCH_REQ: {
#if !defined(DISABLE_MCMD_SNCH_REQ)
            u1_t chidx = opts[oidx+1];  // channel
            u4_t freq  = convFreq(&opts[oidx+2]); // freq
            u1_t drs   = opts[oidx+5];  // datarate span
            LMIC.snchAns = 0x80;
            if( freq != 0 && LMIC_setupChannel(chidx, freq, DR_RANGE_MAP(drs&0xF,drs>>4), -1) )
                LMIC.snchAns |= MCMD_SNCH_ANS_DRACK|MCMD_SNCH_ANS_FQACK;
#endif // !DISABLE_MCMD_SNCH_REQ
            oidx += 6;
            continue;
        }
        case MCMD_PING_SET: {
#if !defined(DISABLE_MCMD_PING_SET) && !defined(DISABLE_PING)
            u4_t freq = convFreq(&opts[oidx+1]);
            u1_t flags = 0x80;
            if( freq != 0 ) {
                flags |= MCMD_PING_ANS_FQACK;
                LMIC.ping.freq = freq;
                DO_DEVDB(LMIC.ping.intvExp, pingIntvExp);
                DO_DEVDB(LMIC.ping.freq, pingFreq);
                DO_DEVDB(LMIC.ping.dr, pingDr);
            }
            LMIC.pingSetAns = flags;
#endif // !DISABLE_MCMD_PING_SET && !DISABLE_PING
            oidx += 4;
            continue;
        }
        case MCMD_BCNI_ANS: {
#if !defined(DISABLE_MCMD_BCNI_ANS) && !defined(DISABLE_BEACONS)
            // Ignore if tracking already enabled
            if( (LMIC.opmode & OP_TRACK) == 0 ) {
                LMIC.bcnChnl = opts[oidx+3];
                // Enable tracking - bcninfoTries
                LMIC.opmode |= OP_TRACK;
                // Cleared later in txComplete handling - triggers EV_BEACON_FOUND
                ASSERT(LMIC.bcninfoTries!=0);
                // Setup RX parameters
                LMIC.bcninfo.txtime = (LMIC.rxtime
                                       + ms2osticks(os_rlsbf2(&opts[oidx+1]) * MCMD_BCNI_TUNIT)
                                       + ms2osticksCeil(MCMD_BCNI_TUNIT/2)
                                       - BCN_INTV_osticks);
                LMIC.bcninfo.flags = 0;  // txtime above cannot be used as reference (BCN_PARTIAL|BCN_FULL cleared)
                calcBcnRxWindowFromMillis(MCMD_BCNI_TUNIT,1);  // error of +/-N ms

                EV(lostFrame, INFO, (e_.reason  = EV::lostFrame_t::MCMD_BCNI_ANS,
                                     e_.eui     = MAIN::CDEV->getEui(),
                                     e_.lostmic = Base::lsbf4(&d[pend]),
                                     e_.info    = (LMIC.missedBcns |
                                                   (osticks2us(LMIC.bcninfo.txtime + BCN_INTV_osticks
                                                               - LMIC.bcnRxtime) << 8)),
                                     e_.time    = MAIN::CDEV->ostime2ustime(LMIC.bcninfo.txtime + BCN_INTV_osticks)));
            }
#endif // !DISABLE_MCMD_BCNI_ANS && !DISABLE_BEACONS
            oidx += 4;
            continue;
        }
        }
        EV(specCond, ERR, (e_.reason = EV::specCond_t::BAD_MAC_CMD,
                           e_.eui    = MAIN::CDEV->getEui(),
                           e_.info   = Base::lsbf4(&d[pend]),
                           e_.info2  = Base::msbf4(&opts[oidx])));
        break;
    }
    if( oidx != olen ) {
        EV(specCond, ERR, (e_.reason = EV::specCond_t::CORRUPTED_FRAME,
                           e_.eui    = MAIN::CDEV->getEui(),
                           e_.info   = 0x1000000 + (oidx) + (olen<<8)));
    }

    if( !replayConf ) {
        // Handle payload only if not a replay
        // Decrypt payload - if any
        if( port >= 0  &&  pend-poff > 0 )
            aes_cipher(port <= 0 ? LMIC.nwkKey : LMIC.artKey, LMIC.devaddr, seqno, /*dn*/1, d+poff, pend-poff);

        EV(dfinfo, DEBUG, (e_.deveui  = MAIN::CDEV->getEui(),
                           e_.devaddr = LMIC.devaddr,
                           e_.seqno   = seqno,
                           e_.flags   = (port < 0 ? EV::dfinfo_t::NOPORT : 0) | EV::dfinfo_t::DN,
                           e_.mic     = Base::lsbf4(&d[pend]),
                           e_.hdr     = d[LORA::OFF_DAT_HDR],
                           e_.fct     = d[LORA::OFF_DAT_FCT],
                           e_.port    = port,
                           e_.plen    = dlen,
                           e_.opts.length = olen,
                           memcpy(&e_.opts[0], opts, olen)));
    } else {
        EV(specCond, INFO, (e_.reason = EV::specCond_t::DNSEQNO_REPLAY,
                            e_.eui    = MAIN::CDEV->getEui(),
                            e_.info   = Base::lsbf4(&d[pend]),
                            e_.info2  = seqno));
    }

    if( // NWK acks but we don't have a frame pending
        (ackup && LMIC.txCnt == 0) ||
        // We sent up confirmed and we got a response in DNW1/DNW2
        // BUT it did not carry an ACK - this should never happen
        // Do not resend and assume frame was not ACKed.
        (!ackup && LMIC.txCnt != 0) ) {
        EV(specCond, ERR, (e_.reason = EV::specCond_t::SPURIOUS_ACK,
                           e_.eui    = MAIN::CDEV->getEui(),
                           e_.info   = seqno,
                           e_.info2  = ackup));
    }

    if( LMIC.txCnt != 0 ) // we requested an ACK
        LMIC.txrxFlags |= ackup ? TXRX_ACK : TXRX_NACK;

    if( port < 0 ) {
        LMIC.txrxFlags |= TXRX_NOPORT;
        LMIC.dataBeg = poff;
        LMIC.dataLen = 0;
    } else {
        LMIC.txrxFlags |= TXRX_PORT;
        LMIC.dataBeg = poff;
        LMIC.dataLen = pend-poff;
    }
#if LMIC_DEBUG_LEVEL > 0
    lmic_printf("%lu: Received downlink, window=%s, port=%d, ack=%d\n", os_getTime(), window, port, ackup);
#endif
    return 1;
}


// ================================================================================
// TX/RX transaction support


static void setupRx2 (void) {
    LMIC.txrxFlags = TXRX_DNW2;
    LMIC.rps = dndr2rps(LMIC.dn2Dr);
    LMIC.freq = LMIC.dn2Freq;
    LMIC.dataLen = 0;
    os_radio(RADIO_RX);
}


static void schedRx12 (ostime_t delay, osjobcb_t func, u1_t dr) {
    ostime_t hsym = dr2hsym(dr);

    LMIC.rxsyms = MINRX_SYMS;

    // If a clock error is specified, compensate for it by extending the
    // receive window
    if (LMIC.clockError != 0) {
        // Calculate how much the clock will drift maximally after delay has
        // passed. This indicates the amount of time we can be early
        // _or_ late.
        ostime_t drift = (int64_t)delay * LMIC.clockError / MAX_CLOCK_ERROR;

        // Increase the receive window by twice the maximum drift (to
        // compensate for a slow or a fast clock).
        // decrease the rxtime to compensate for. Note that hsym is a
        // *half* symbol time, so the factor 2 is hidden. First check if
        // this would overflow (which can happen if the drift is very
        // high, or the symbol time is low at high datarates).
        if ((255 - LMIC.rxsyms) * hsym < drift)
            LMIC.rxsyms = 255;
        else
            LMIC.rxsyms += drift / hsym;

    }

    // Center the receive window on the center of the expected preamble
    // (again note that hsym is half a sumbol time, so no /2 needed)
    LMIC.rxtime = LMIC.txend + delay + PAMBL_SYMS * hsym - LMIC.rxsyms * hsym;

    os_setTimedCallback(&LMIC.osjob, LMIC.rxtime - RX_RAMPUP, func);
}

static void setupRx1 (osjobcb_t func) {
    LMIC.txrxFlags = TXRX_DNW1;
    // Turn LMIC.rps from TX over to RX
    LMIC.rps = setNocrc(LMIC.rps,1);
    LMIC.dataLen = 0;
    LMIC.osjob.func = func;
    os_radio(RADIO_RX);
}


// Called by HAL once TX complete and delivers exact end of TX time stamp in LMIC.rxtime
static void txDone (ostime_t delay, osjobcb_t func) {
#if !defined(DISABLE_PING)
    if( (LMIC.opmode & (OP_TRACK|OP_PINGABLE|OP_PINGINI)) == (OP_TRACK|OP_PINGABLE) ) {
        rxschedInit(&LMIC.ping);    // note: reuses LMIC.frame buffer!
        LMIC.opmode |= OP_PINGINI;
    }
#endif // !DISABLE_PING

    // Change RX frequency / rps (US only) before we increment txChnl
    setRx1Params();
    // LMIC.rxsyms carries the TX datarate (can be != LMIC.datarate [confirm retries etc.])
    // Setup receive - LMIC.rxtime is preloaded with 1.5 symbols offset to tune
    // into the middle of the 8 symbols preamble.
#if defined(CFG_eu868)
    if( /* TX datarate */LMIC.rxsyms == DR_FSK ) {
        LMIC.rxtime = LMIC.txend + delay - PRERX_FSK*us2osticksRound(160);
        LMIC.rxsyms = RXLEN_FSK;
        os_setTimedCallback(&LMIC.osjob, LMIC.rxtime - RX_RAMPUP, func);
    }
    else
#endif
    {
        schedRx12(delay, func, LMIC.dndr);
    }
}


// ======================================== Join frames


#if !defined(DISABLE_JOIN)
static void onJoinFailed (xref2osjob_t osjob) {
    // Notify app - must call LMIC_reset() to stop joining
    // otherwise join procedure continues.
    reportEvent(EV_JOIN_FAILED);
}


static bit_t processJoinAccept (void) {
    ASSERT(LMIC.txrxFlags != TXRX_DNW1 || LMIC.dataLen != 0);
    ASSERT((LMIC.opmode & OP_TXRXPEND)!=0);

    if( LMIC.dataLen == 0 ) {
      nojoinframe:
        if( (LMIC.opmode & OP_JOINING) == 0 ) {
            ASSERT((LMIC.opmode & OP_REJOIN) != 0);
            // REJOIN attempt for roaming
            LMIC.opmode &= ~(OP_REJOIN|OP_TXRXPEND);
            if( LMIC.rejoinCnt < 10 )
                LMIC.rejoinCnt++;
            reportEvent(EV_REJOIN_FAILED);
            return 1;
        }
        LMIC.opmode &= ~OP_TXRXPEND;
        ostime_t delay = nextJoinState();
        EV(devCond, DEBUG, (e_.reason = EV::devCond_t::NO_JACC,
                            e_.eui    = MAIN::CDEV->getEui(),
                            e_.info   = LMIC.datarate|DR_PAGE,
                            e_.info2  = osticks2ms(delay)));
        // Build next JOIN REQUEST with next engineUpdate call
        // Optionally, report join failed.
        // Both after a random/chosen amount of ticks.
        os_setTimedCallback(&LMIC.osjob, os_getTime()+delay,
                            (delay&1) != 0
                            ? FUNC_ADDR(onJoinFailed)      // one JOIN iteration done and failed
                            : FUNC_ADDR(runEngineUpdate)); // next step to be delayed
        return 1;
    }
    u1_t hdr  = LMIC.frame[0];
    u1_t dlen = LMIC.dataLen;
    u4_t mic  = os_rlsbf4(&LMIC.frame[dlen-4]); // safe before modified by encrypt!
    if( (dlen != LEN_JA && dlen != LEN_JAEXT)
        || (hdr & (HDR_FTYPE|HDR_MAJOR)) != (HDR_FTYPE_JACC|HDR_MAJOR_V1) ) {
        EV(specCond, ERR, (e_.reason = EV::specCond_t::UNEXPECTED_FRAME,
                           e_.eui    = MAIN::CDEV->getEui(),
                           e_.info   = dlen < 4 ? 0 : mic,
                           e_.info2  = hdr + (dlen<<8)));
      badframe:
        if( (LMIC.txrxFlags & TXRX_DNW1) != 0 )
            return 0;
        goto nojoinframe;
    }
    aes_encrypt(LMIC.frame+1, dlen-1);
    if( !aes_verifyMic0(LMIC.frame, dlen-4) ) {
        EV(specCond, ERR, (e_.reason = EV::specCond_t::JOIN_BAD_MIC,
                           e_.info   = mic));
        goto badframe;
    }

    u4_t addr = os_rlsbf4(LMIC.frame+OFF_JA_DEVADDR);
    LMIC.devaddr = addr;
    LMIC.netid = os_rlsbf4(&LMIC.frame[OFF_JA_NETID]) & 0xFFFFFF;

#if defined(CFG_eu868)
    initDefaultChannels(0);
#endif
    if( dlen > LEN_JA ) {
#if defined(CFG_us915)
        goto badframe;
#endif
        dlen = OFF_CFLIST;
        for( u1_t chidx=3; chidx<8; chidx++, dlen+=3 ) {
            u4_t freq = convFreq(&LMIC.frame[dlen]);
            if( freq ) {
                LMIC_setupChannel(chidx, freq, 0, -1);
#if LMIC_DEBUG_LEVEL > 1
                lmic_printf("%lu: Setup channel, idx=%d, freq=%lu\n", os_getTime(), chidx, (unsigned long)freq);
#endif
            }
        }
    }

    // already incremented when JOIN REQ got sent off
    aes_sessKeys(LMIC.devNonce-1, &LMIC.frame[OFF_JA_ARTNONCE], LMIC.nwkKey, LMIC.artKey);
    DO_DEVDB(LMIC.netid,   netid);
    DO_DEVDB(LMIC.devaddr, devaddr);
    DO_DEVDB(LMIC.nwkKey,  nwkkey);
    DO_DEVDB(LMIC.artKey,  artkey);

    EV(joininfo, INFO, (e_.arteui  = MAIN::CDEV->getArtEui(),
                        e_.deveui  = MAIN::CDEV->getEui(),
                        e_.devaddr = LMIC.devaddr,
                        e_.oldaddr = oldaddr,
                        e_.nonce   = LMIC.devNonce-1,
                        e_.mic     = mic,
                        e_.reason  = ((LMIC.opmode & OP_REJOIN) != 0
                                      ? EV::joininfo_t::REJOIN_ACCEPT
                                      : EV::joininfo_t::ACCEPT)));

    ASSERT((LMIC.opmode & (OP_JOINING|OP_REJOIN))!=0);
    if( (LMIC.opmode & OP_REJOIN) != 0 ) {
        // Lower DR every try below current UP DR
        LMIC.datarate = lowerDR(LMIC.datarate, LMIC.rejoinCnt);
    }
    LMIC.opmode &= ~(OP_JOINING|OP_TRACK|OP_REJOIN|OP_TXRXPEND|OP_PINGINI) | OP_NEXTCHNL;
    LMIC.txCnt = 0;
    stateJustJoined();
    LMIC.dn2Dr = LMIC.frame[OFF_JA_DLSET] & 0x0F;
    LMIC.rxDelay = LMIC.frame[OFF_JA_RXDLY];
    if (LMIC.rxDelay == 0) LMIC.rxDelay = 1;
    reportEvent(EV_JOINED);
    return 1;
}


static void processRx2Jacc (xref2osjob_t osjob) {
    if( LMIC.dataLen == 0 )
        LMIC.txrxFlags = 0;  // nothing in 1st/2nd DN slot
    processJoinAccept();
}


static void setupRx2Jacc (xref2osjob_t osjob) {
    LMIC.osjob.func = FUNC_ADDR(processRx2Jacc);
    setupRx2();
}


static void processRx1Jacc (xref2osjob_t osjob) {
    if( LMIC.dataLen == 0 || !processJoinAccept() )
        schedRx12(DELAY_JACC2_osticks, FUNC_ADDR(setupRx2Jacc), LMIC.dn2Dr);
}


static void setupRx1Jacc (xref2osjob_t osjob) {
    setupRx1(FUNC_ADDR(processRx1Jacc));
}


static void jreqDone (xref2osjob_t osjob) {
    txDone(DELAY_JACC1_osticks, FUNC_ADDR(setupRx1Jacc));
}

#endif // !DISABLE_JOIN

// ======================================== Data frames

// Fwd decl.
static bit_t processDnData(void);

static void processRx2DnData (xref2osjob_t osjob) {
    if( LMIC.dataLen == 0 ) {
        LMIC.txrxFlags = 0;  // nothing in 1st/2nd DN slot
        // It could be that the gateway *is* sending a reply, but we
        // just didn't pick it up. To avoid TX'ing again while the
        // gateay is not listening anyway, delay the next transmission
        // until DNW2_SAFETY_ZONE from now, and add up to 2 seconds of
        // extra randomization.
        txDelay(os_getTime() + DNW2_SAFETY_ZONE, 2);
    }
    processDnData();
}


static void setupRx2DnData (xref2osjob_t osjob) {
    LMIC.osjob.func = FUNC_ADDR(processRx2DnData);
    setupRx2();
}


static void processRx1DnData (xref2osjob_t osjob) {
    if( LMIC.dataLen == 0 || !processDnData() )
        schedRx12(sec2osticks(LMIC.rxDelay +(int)DELAY_EXTDNW2), FUNC_ADDR(setupRx2DnData), LMIC.dn2Dr);
}


static void setupRx1DnData (xref2osjob_t osjob) {
    setupRx1(FUNC_ADDR(processRx1DnData));
}


static void updataDone (xref2osjob_t osjob) {
    txDone(sec2osticks(LMIC.rxDelay), FUNC_ADDR(setupRx1DnData));
}

// ========================================


static void buildDataFrame (void) {
    bit_t txdata = ((LMIC.opmode & (OP_TXDATA|OP_POLL)) != OP_POLL);
    u1_t dlen = txdata ? LMIC.pendTxLen : 0;

    // Piggyback MAC options
    // Prioritize by importance
    int  end = OFF_DAT_OPTS;
#if !defined(DISABLE_PING)
    if( (LMIC.opmode & (OP_TRACK|OP_PINGABLE)) == (OP_TRACK|OP_PINGABLE) ) {
        // Indicate pingability in every UP frame
        LMIC.frame[end] = MCMD_PING_IND;
        LMIC.frame[end+1] = LMIC.ping.dr | (LMIC.ping.intvExp<<4);
        end += 2;
    }
#endif // !DISABLE_PING
#if !defined(DISABLE_MCMD_DCAP_REQ)
    if( LMIC.dutyCapAns ) {
        LMIC.frame[end] = MCMD_DCAP_ANS;
        end += 1;
        LMIC.dutyCapAns = 0;
    }
#endif // !DISABLE_MCMD_DCAP_REQ
#if !defined(DISABLE_MCMD_DN2P_SET)
    if( LMIC.dn2Ans ) {
        LMIC.frame[end+0] = MCMD_DN2P_ANS;
        LMIC.frame[end+1] = LMIC.dn2Ans & ~MCMD_DN2P_ANS_RFU;
        end += 2;
        LMIC.dn2Ans = 0;
    }
#endif // !DISABLE_MCMD_DN2P_SET
    if( LMIC.devsAns ) {  // answer to device status
        LMIC.frame[end+0] = MCMD_DEVS_ANS;
        LMIC.frame[end+1] = os_getBattLevel();
        LMIC.frame[end+2] = LMIC.margin;
        end += 3;
        LMIC.devsAns = 0;
    }
    if( LMIC.ladrAns ) {  // answer to ADR change
        LMIC.frame[end+0] = MCMD_LADR_ANS;
        LMIC.frame[end+1] = LMIC.ladrAns & ~MCMD_LADR_ANS_RFU;
        end += 2;
        LMIC.ladrAns = 0;
    }
#if !defined(DISABLE_BEACONS)
    if( LMIC.bcninfoTries > 0 ) {
        LMIC.frame[end] = MCMD_BCNI_REQ;
        end += 1;
    }
#endif // !DISABLE_BEACONS
    if( LMIC.adrChanged ) {
        if( LMIC.adrAckReq < 0 )
            LMIC.adrAckReq = 0;
        LMIC.adrChanged = 0;
    }
#if !defined(DISABLE_MCMD_PING_SET) && !defined(DISABLE_PING)
    if( LMIC.pingSetAns != 0 ) {
        LMIC.frame[end+0] = MCMD_PING_ANS;
        LMIC.frame[end+1] = LMIC.pingSetAns & ~MCMD_PING_ANS_RFU;
        end += 2;
        LMIC.pingSetAns = 0;
    }
#endif // !DISABLE_MCMD_PING_SET && !DISABLE_PING
#if !defined(DISABLE_MCMD_SNCH_REQ)
    if( LMIC.snchAns ) {
        LMIC.frame[end+0] = MCMD_SNCH_ANS;
        LMIC.frame[end+1] = LMIC.snchAns & ~MCMD_SNCH_ANS_RFU;
        end += 2;
        LMIC.snchAns = 0;
    }
#endif // !DISABLE_MCMD_SNCH_REQ
    ASSERT(end <= OFF_DAT_OPTS+16);

    u1_t flen = end + (txdata ? 5+dlen : 4);
    if( flen > MAX_LEN_FRAME ) {
        // Options and payload too big - delay payload
        txdata = 0;
        flen = end+4;
    }
    LMIC.frame[OFF_DAT_HDR] = HDR_FTYPE_DAUP | HDR_MAJOR_V1;
    LMIC.frame[OFF_DAT_FCT] = (LMIC.dnConf | LMIC.adrEnabled
                              | (LMIC.adrAckReq >= 0 ? FCT_ADRARQ : 0)
                              | (end-OFF_DAT_OPTS));
    os_wlsbf4(LMIC.frame+OFF_DAT_ADDR,  LMIC.devaddr);

    if( LMIC.txCnt == 0 ) {
        LMIC.seqnoUp += 1;
        DO_DEVDB(LMIC.seqnoUp,seqnoUp);
    } else {
        EV(devCond, INFO, (e_.reason = EV::devCond_t::RE_TX,
                           e_.eui    = MAIN::CDEV->getEui(),
                           e_.info   = LMIC.seqnoUp-1,
                           e_.info2  = ((LMIC.txCnt+1) |
                                        (TABLE_GET_U1(DRADJUST, LMIC.txCnt+1) << 8) |
                                        ((LMIC.datarate|DR_PAGE)<<16))));
    }
    os_wlsbf2(LMIC.frame+OFF_DAT_SEQNO, LMIC.seqnoUp-1);

    // Clear pending DN confirmation
    LMIC.dnConf = 0;

    if( txdata ) {
        if( LMIC.pendTxConf ) {
            // Confirmed only makes sense if we have a payload (or at least a port)
            LMIC.frame[OFF_DAT_HDR] = HDR_FTYPE_DCUP | HDR_MAJOR_V1;
            if( LMIC.txCnt == 0 ) LMIC.txCnt = 1;
        }
        LMIC.frame[end] = LMIC.pendTxPort;
        os_copyMem(LMIC.frame+end+1, LMIC.pendTxData, dlen);
        aes_cipher(LMIC.pendTxPort==0 ? LMIC.nwkKey : LMIC.artKey,
                   LMIC.devaddr, LMIC.seqnoUp-1,
                   /*up*/0, LMIC.frame+end+1, dlen);
    }
    aes_appendMic(LMIC.nwkKey, LMIC.devaddr, LMIC.seqnoUp-1, /*up*/0, LMIC.frame, flen-4);

    EV(dfinfo, DEBUG, (e_.deveui  = MAIN::CDEV->getEui(),
                       e_.devaddr = LMIC.devaddr,
                       e_.seqno   = LMIC.seqnoUp-1,
                       e_.flags   = (LMIC.pendTxPort < 0 ? EV::dfinfo_t::NOPORT : EV::dfinfo_t::NOP),
                       e_.mic     = Base::lsbf4(&LMIC.frame[flen-4]),
                       e_.hdr     = LMIC.frame[LORA::OFF_DAT_HDR],
                       e_.fct     = LMIC.frame[LORA::OFF_DAT_FCT],
                       e_.port    = LMIC.pendTxPort,
                       e_.plen    = txdata ? dlen : 0,
                       e_.opts.length = end-LORA::OFF_DAT_OPTS,
                       memcpy(&e_.opts[0], LMIC.frame+LORA::OFF_DAT_OPTS, end-LORA::OFF_DAT_OPTS)));
    LMIC.dataLen = flen;
}


#if !defined(DISABLE_BEACONS)
// Callback from HAL during scan mode or when job timer expires.
static void onBcnRx (xref2osjob_t job) {
    // If we arrive via job timer make sure to put radio to rest.
    os_radio(RADIO_RST);
    os_clearCallback(&LMIC.osjob);
    if( LMIC.dataLen == 0 ) {
        // Nothing received - timeout
        LMIC.opmode &= ~(OP_SCAN | OP_TRACK);
        reportEvent(EV_SCAN_TIMEOUT);
        return;
    }
    if( decodeBeacon() <= 0 ) {
        // Something is wrong with the beacon - continue scan
        LMIC.dataLen = 0;
        os_radio(RADIO_RXON);
        os_setTimedCallback(&LMIC.osjob, LMIC.bcninfo.txtime, FUNC_ADDR(onBcnRx));
        return;
    }
    // Found our 1st beacon
    // We don't have a previous beacon to calc some drift - assume
    // an max error of 13ms = 128sec*100ppm which is roughly +/-100ppm
    calcBcnRxWindowFromMillis(13,1);
    LMIC.opmode &= ~OP_SCAN;          // turn SCAN off
    LMIC.opmode |=  OP_TRACK;         // auto enable tracking
    reportEvent(EV_BEACON_FOUND);    // can be disabled in callback
}


// Enable receiver to listen to incoming beacons
// netid defines when scan stops (any or specific beacon)
// This mode ends with events: EV_SCAN_TIMEOUT/EV_SCAN_BEACON
// Implicitely cancels any pending TX/RX transaction.
// Also cancels an onpoing joining procedure.
static void startScan (void) {
    ASSERT(LMIC.devaddr!=0 && (LMIC.opmode & OP_JOINING)==0);
    if( (LMIC.opmode & OP_SHUTDOWN) != 0 )
        return;
    // Cancel onging TX/RX transaction
    LMIC.txCnt = LMIC.dnConf = LMIC.bcninfo.flags = 0;
    LMIC.opmode = (LMIC.opmode | OP_SCAN) & ~(OP_TXRXPEND);
    setBcnRxParams();
    LMIC.rxtime = LMIC.bcninfo.txtime = os_getTime() + sec2osticks(BCN_INTV_sec+1);
    os_setTimedCallback(&LMIC.osjob, LMIC.rxtime, FUNC_ADDR(onBcnRx));
    os_radio(RADIO_RXON);
}


bit_t LMIC_enableTracking (u1_t tryBcnInfo) {
    if( (LMIC.opmode & (OP_SCAN|OP_TRACK|OP_SHUTDOWN)) != 0 )
        return 0;  // already in progress or failed to enable
    // If BCN info requested from NWK then app has to take are
    // of sending data up so that MCMD_BCNI_REQ can be attached.
    if( (LMIC.bcninfoTries = tryBcnInfo) == 0 )
        startScan();
    return 1;  // enabled
}


void LMIC_disableTracking (void) {
    LMIC.opmode &= ~(OP_SCAN|OP_TRACK);
    LMIC.bcninfoTries = 0;
    engineUpdate();
}
#endif // !DISABLE_BEACONS


// ================================================================================
//
// Join stuff
//
// ================================================================================

#if !defined(DISABLE_JOIN)
static void buildJoinRequest (u1_t ftype) {
    // Do not use pendTxData since we might have a pending
    // user level frame in there. Use RX holding area instead.
    xref2u1_t d = LMIC.frame;
    d[OFF_JR_HDR] = ftype;
    os_getArtEui(d + OFF_JR_ARTEUI);
    os_getDevEui(d + OFF_JR_DEVEUI);
    os_wlsbf2(d + OFF_JR_DEVNONCE, LMIC.devNonce);
    aes_appendMic0(d, OFF_JR_MIC);

    EV(joininfo,INFO,(e_.deveui  = MAIN::CDEV->getEui(),
                      e_.arteui  = MAIN::CDEV->getArtEui(),
                      e_.nonce   = LMIC.devNonce,
                      e_.oldaddr = LMIC.devaddr,
                      e_.mic     = Base::lsbf4(&d[LORA::OFF_JR_MIC]),
                      e_.reason  = ((LMIC.opmode & OP_REJOIN) != 0
                                    ? EV::joininfo_t::REJOIN_REQUEST
                                    : EV::joininfo_t::REQUEST)));
    LMIC.dataLen = LEN_JR;
    LMIC.devNonce++;
    DO_DEVDB(LMIC.devNonce,devNonce);
}

static void startJoining (xref2osjob_t osjob) {
    reportEvent(EV_JOINING);
}

// Start join procedure if not already joined.
bit_t LMIC_startJoining (void) {
    if( LMIC.devaddr == 0 ) {
        // There should be no TX/RX going on
        ASSERT((LMIC.opmode & (OP_POLL|OP_TXRXPEND)) == 0);
        // Lift any previous duty limitation
        LMIC.globalDutyRate = 0;
        // Cancel scanning
        LMIC.opmode &= ~(OP_SCAN|OP_REJOIN|OP_LINKDEAD|OP_NEXTCHNL);
        // Setup state
        LMIC.rejoinCnt = LMIC.txCnt = 0;
        initJoinLoop();
        LMIC.opmode |= OP_JOINING;
        // reportEvent will call engineUpdate which then starts sending JOIN REQUESTS
        os_setCallback(&LMIC.osjob, FUNC_ADDR(startJoining));
        return 1;
    }
    return 0; // already joined
}
#endif // !DISABLE_JOIN


// ================================================================================
//
//
//
// ================================================================================

#if !defined(DISABLE_PING)
static void processPingRx (xref2osjob_t osjob) {
    if( LMIC.dataLen != 0 ) {
        LMIC.txrxFlags = TXRX_PING;
        if( decodeFrame() ) {
            reportEvent(EV_RXCOMPLETE);
            return;
        }
    }
    // Pick next ping slot
    engineUpdate();
}
#endif // !DISABLE_PING


static bit_t processDnData (void) {
    ASSERT((LMIC.opmode & OP_TXRXPEND)!=0);

    if( LMIC.dataLen == 0 ) {
      norx:
        if( LMIC.txCnt != 0 ) {
            if( LMIC.txCnt < TXCONF_ATTEMPTS ) {
                LMIC.txCnt += 1;
                setDrTxpow(DRCHG_NOACK, lowerDR(LMIC.datarate, TABLE_GET_U1(DRADJUST, LMIC.txCnt)), KEEP_TXPOW);
                // Schedule another retransmission
                txDelay(LMIC.rxtime, RETRY_PERIOD_secs);
                LMIC.opmode &= ~OP_TXRXPEND;
                engineUpdate();
                return 1;
            }
            LMIC.txrxFlags = TXRX_NACK | TXRX_NOPORT;
        } else {
            // Nothing received - implies no port
            LMIC.txrxFlags = TXRX_NOPORT;
        }
        if( LMIC.adrAckReq != LINK_CHECK_OFF )
            LMIC.adrAckReq += 1;
        LMIC.dataBeg = LMIC.dataLen = 0;
      txcomplete:
        LMIC.opmode &= ~(OP_TXDATA|OP_TXRXPEND);
        if( (LMIC.txrxFlags & (TXRX_DNW1|TXRX_DNW2|TXRX_PING)) != 0  &&  (LMIC.opmode & OP_LINKDEAD) != 0 ) {
            LMIC.opmode &= ~OP_LINKDEAD;
            reportEvent(EV_LINK_ALIVE);
        }
        reportEvent(EV_TXCOMPLETE);
        // If we haven't heard from NWK in a while although we asked for a sign
        // assume link is dead - notify application and keep going
        if( LMIC.adrAckReq > LINK_CHECK_DEAD ) {
            // We haven't heard from NWK for some time although we
            // asked for a response for some time - assume we're disconnected. Lower DR one notch.
            EV(devCond, ERR, (e_.reason = EV::devCond_t::LINK_DEAD,
                              e_.eui    = MAIN::CDEV->getEui(),
                              e_.info   = LMIC.adrAckReq));
            setDrTxpow(DRCHG_NOADRACK, decDR((dr_t)LMIC.datarate), KEEP_TXPOW);
            LMIC.adrAckReq = LINK_CHECK_CONT;
            LMIC.opmode |= OP_REJOIN|OP_LINKDEAD;
            reportEvent(EV_LINK_DEAD);
        }
#if !defined(DISABLE_BEACONS)
        // If this falls to zero the NWK did not answer our MCMD_BCNI_REQ commands - try full scan
        if( LMIC.bcninfoTries > 0 ) {
            if( (LMIC.opmode & OP_TRACK) != 0 ) {
                reportEvent(EV_BEACON_FOUND);
                LMIC.bcninfoTries = 0;
            }
            else if( --LMIC.bcninfoTries == 0 ) {
                startScan();   // NWK did not answer - try scan
            }
        }
#endif // !DISABLE_BEACONS
        return 1;
    }
    if( !decodeFrame() ) {
        if( (LMIC.txrxFlags & TXRX_DNW1) != 0 )
            return 0;
        goto norx;
    }
    goto txcomplete;
}


#if !defined(DISABLE_BEACONS)
static void processBeacon (xref2osjob_t osjob) {
    ostime_t lasttx = LMIC.bcninfo.txtime;   // save here - decodeBeacon might overwrite
    u1_t flags = LMIC.bcninfo.flags;
    ev_t ev;

    if( LMIC.dataLen != 0 && decodeBeacon() >= 1 ) {
        ev = EV_BEACON_TRACKED;
        if( (flags & (BCN_PARTIAL|BCN_FULL)) == 0 ) {
            // We don't have a previous beacon to calc some drift - assume
            // an max error of 13ms = 128sec*100ppm which is roughly +/-100ppm
            calcBcnRxWindowFromMillis(13,0);
            goto rev;
        }
        // We have a previous BEACON to calculate some drift
        s2_t drift = BCN_INTV_osticks - (LMIC.bcninfo.txtime - lasttx);
        if( LMIC.missedBcns > 0 ) {
            drift = LMIC.drift + (drift - LMIC.drift) / (LMIC.missedBcns+1);
        }
        if( (LMIC.bcninfo.flags & BCN_NODRIFT) == 0 ) {
            s2_t diff = LMIC.drift - drift;
            if( diff < 0 ) diff = -diff;
            LMIC.lastDriftDiff = diff;
            if( LMIC.maxDriftDiff < diff )
                LMIC.maxDriftDiff = diff;
            LMIC.bcninfo.flags &= ~BCN_NODDIFF;
        }
        LMIC.drift = drift;
        LMIC.missedBcns = LMIC.rejoinCnt = 0;
        LMIC.bcninfo.flags &= ~BCN_NODRIFT;
        EV(devCond,INFO,(e_.reason = EV::devCond_t::CLOCK_DRIFT,
                         e_.eui    = MAIN::CDEV->getEui(),
                         e_.info   = drift,
                         e_.info2  = /*occasion BEACON*/0));
        ASSERT((LMIC.bcninfo.flags & (BCN_PARTIAL|BCN_FULL)) != 0);
    } else {
        ev = EV_BEACON_MISSED;
        LMIC.bcninfo.txtime += BCN_INTV_osticks - LMIC.drift;
        LMIC.bcninfo.time   += BCN_INTV_sec;
        LMIC.missedBcns++;
        // Delay any possible TX after surmised beacon - it's there although we missed it
        txDelay(LMIC.bcninfo.txtime + BCN_RESERVE_osticks, 4);
        if( LMIC.missedBcns > MAX_MISSED_BCNS )
            LMIC.opmode |= OP_REJOIN;  // try if we can roam to another network
        if( LMIC.bcnRxsyms > MAX_RXSYMS ) {
            LMIC.opmode &= ~(OP_TRACK|OP_PINGABLE|OP_PINGINI|OP_REJOIN);
            reportEvent(EV_LOST_TSYNC);
            return;
        }
    }
    LMIC.bcnRxtime = LMIC.bcninfo.txtime + BCN_INTV_osticks - calcRxWindow(0,DR_BCN);
    LMIC.bcnRxsyms = LMIC.rxsyms;
  rev:
#if CFG_us915
    LMIC.bcnChnl = (LMIC.bcnChnl+1) & 7;
#endif
#if !defined(DISABLE_PING)
    if( (LMIC.opmode & OP_PINGINI) != 0 )
        rxschedInit(&LMIC.ping);  // note: reuses LMIC.frame buffer!
#endif // !DISABLE_PING
    reportEvent(ev);
}


static void startRxBcn (xref2osjob_t osjob) {
    LMIC.osjob.func = FUNC_ADDR(processBeacon);
    os_radio(RADIO_RX);
}
#endif // !DISABLE_BEACONS


#if !defined(DISABLE_PING)
static void startRxPing (xref2osjob_t osjob) {
    LMIC.osjob.func = FUNC_ADDR(processPingRx);
    os_radio(RADIO_RX);
}
#endif // !DISABLE_PING


// Decide what to do next for the MAC layer of a device
static void engineUpdate (void) {
#if LMIC_DEBUG_LEVEL > 0
    lmic_printf("%lu: engineUpdate, opmode=0x%x\n", os_getTime(), LMIC.opmode);
#endif
    // Check for ongoing state: scan or TX/RX transaction
    if( (LMIC.opmode & (OP_SCAN|OP_TXRXPEND|OP_SHUTDOWN)) != 0 )
        return;

#if !defined(DISABLE_JOIN)
    if( LMIC.devaddr == 0 && (LMIC.opmode & OP_JOINING) == 0 ) {
        LMIC_startJoining();
        return;
    }
#endif // !DISABLE_JOIN

    ostime_t now    = os_getTime();
    ostime_t rxtime = 0;
    ostime_t txbeg  = 0;

#if !defined(DISABLE_BEACONS)
    if( (LMIC.opmode & OP_TRACK) != 0 ) {
        // We are tracking a beacon
        ASSERT( now + RX_RAMPUP - LMIC.bcnRxtime <= 0 );
        rxtime = LMIC.bcnRxtime - RX_RAMPUP;
    }
#endif // !DISABLE_BEACONS

    if( (LMIC.opmode & (OP_JOINING|OP_REJOIN|OP_TXDATA|OP_POLL)) != 0 ) {
        // Need to TX some data...
        // Assuming txChnl points to channel which first becomes available again.
        bit_t jacc = ((LMIC.opmode & (OP_JOINING|OP_REJOIN)) != 0 ? 1 : 0);
        #if LMIC_DEBUG_LEVEL > 1
            if (jacc)
                lmic_printf("%lu: Uplink join pending\n", os_getTime());
            else
                lmic_printf("%lu: Uplink data pending\n", os_getTime());
        #endif
        // Find next suitable channel and return availability time
        if( (LMIC.opmode & OP_NEXTCHNL) != 0 ) {
            txbeg = LMIC.txend = nextTx(now);
            LMIC.opmode &= ~OP_NEXTCHNL;
            #if LMIC_DEBUG_LEVEL > 1
                lmic_printf("%lu: Airtime available at %lu (channel duty limit)\n", os_getTime(), txbeg);
            #endif
        } else {
            txbeg = LMIC.txend;
            #if LMIC_DEBUG_LEVEL > 1
                lmic_printf("%lu: Airtime available at %lu (previously determined)\n", os_getTime(), txbeg);
            #endif
        }
        // Delayed TX or waiting for duty cycle?
        if( (LMIC.globalDutyRate != 0 || (LMIC.opmode & OP_RNDTX) != 0)  &&  (txbeg - LMIC.globalDutyAvail) < 0 ) {
            txbeg = LMIC.globalDutyAvail;
            #if LMIC_DEBUG_LEVEL > 1
                lmic_printf("%lu: Airtime available at %lu (global duty limit)\n", os_getTime(), txbeg);
            #endif
        }
#if !defined(DISABLE_BEACONS)
        // If we're tracking a beacon...
        // then make sure TX-RX transaction is complete before beacon
        if( (LMIC.opmode & OP_TRACK) != 0 &&
            txbeg + (jacc ? JOIN_GUARD_osticks : TXRX_GUARD_osticks) - rxtime > 0 ) {

            #if LMIC_DEBUG_LEVEL > 1
                lmic_printf("%lu: Awaiting beacon before uplink\n", os_getTime());
            #endif

            // Not enough time to complete TX-RX before beacon - postpone after beacon.
            // In order to avoid clustering of postponed TX right after beacon randomize start!
            txDelay(rxtime + BCN_RESERVE_osticks, 16);
            txbeg = 0;
            goto checkrx;
        }
#endif // !DISABLE_BEACONS
        // Earliest possible time vs overhead to setup radio
        if( txbeg - (now + TX_RAMPUP) < 0 ) {
            #if LMIC_DEBUG_LEVEL > 1
                lmic_printf("%lu: Ready for uplink\n", os_getTime());
            #endif
            // We could send right now!
        txbeg = now;
            dr_t txdr = (dr_t)LMIC.datarate;
#if !defined(DISABLE_JOIN)
            if( jacc ) {
                u1_t ftype;
                if( (LMIC.opmode & OP_REJOIN) != 0 ) {
                    txdr = lowerDR(txdr, LMIC.rejoinCnt);
                    ftype = HDR_FTYPE_REJOIN;
                } else {
                    ftype = HDR_FTYPE_JREQ;
                }
                buildJoinRequest(ftype);
                LMIC.osjob.func = FUNC_ADDR(jreqDone);
            } else
#endif // !DISABLE_JOIN
            {
                if( LMIC.seqnoDn >= 0xFFFFFF80 ) {
                    // Imminent roll over - proactively reset MAC
                    EV(specCond, INFO, (e_.reason = EV::specCond_t::DNSEQNO_ROLL_OVER,
                                        e_.eui    = MAIN::CDEV->getEui(),
                                        e_.info   = LMIC.seqnoDn,
                                        e_.info2  = 0));
                    // Device has to react! NWK will not roll over and just stop sending.
                    // Thus, we have N frames to detect a possible lock up.
                  reset:
                    os_setCallback(&LMIC.osjob, FUNC_ADDR(runReset));
                    return;
                }
                if( (LMIC.txCnt==0 && LMIC.seqnoUp == 0xFFFFFFFF) ) {
                    // Roll over of up seq counter
                    EV(specCond, ERR, (e_.reason = EV::specCond_t::UPSEQNO_ROLL_OVER,
                                       e_.eui    = MAIN::CDEV->getEui(),
                                       e_.info2  = LMIC.seqnoUp));
                    // Do not run RESET event callback from here!
                    // App code might do some stuff after send unaware of RESET.
                    goto reset;
                }
                buildDataFrame();
                LMIC.osjob.func = FUNC_ADDR(updataDone);
            }
            LMIC.rps    = setCr(updr2rps(txdr), (cr_t)LMIC.errcr);
            LMIC.dndr   = txdr;  // carry TX datarate (can be != LMIC.datarate) over to txDone/setupRx1
            LMIC.opmode = (LMIC.opmode & ~(OP_POLL|OP_RNDTX)) | OP_TXRXPEND | OP_NEXTCHNL;
            updateTx(txbeg);
            os_radio(RADIO_TX);
            return;
        }
        #if LMIC_DEBUG_LEVEL > 1
            lmic_printf("%lu: Uplink delayed until %lu\n", os_getTime(), txbeg);
        #endif
        // Cannot yet TX
        if( (LMIC.opmode & OP_TRACK) == 0 )
            goto txdelay; // We don't track the beacon - nothing else to do - so wait for the time to TX
        // Consider RX tasks
        if( txbeg == 0 ) // zero indicates no TX pending
            txbeg += 1;  // TX delayed by one tick (insignificant amount of time)
    } else {
        // No TX pending - no scheduled RX
        if( (LMIC.opmode & OP_TRACK) == 0 )
            return;
    }

#if !defined(DISABLE_BEACONS)
    // Are we pingable?
  checkrx:
#if !defined(DISABLE_PING)
    if( (LMIC.opmode & OP_PINGINI) != 0 ) {
        // One more RX slot in this beacon period?
        if( rxschedNext(&LMIC.ping, now+RX_RAMPUP) ) {
            if( txbeg != 0  &&  (txbeg - LMIC.ping.rxtime) < 0 )
                goto txdelay;
            LMIC.rxsyms  = LMIC.ping.rxsyms;
            LMIC.rxtime  = LMIC.ping.rxtime;
            LMIC.freq    = LMIC.ping.freq;
            LMIC.rps     = dndr2rps(LMIC.ping.dr);
            LMIC.dataLen = 0;
            ASSERT(LMIC.rxtime - now+RX_RAMPUP >= 0 );
            os_setTimedCallback(&LMIC.osjob, LMIC.rxtime - RX_RAMPUP, FUNC_ADDR(startRxPing));
            return;
        }
        // no - just wait for the beacon
    }
#endif // !DISABLE_PING

    if( txbeg != 0  &&  (txbeg - rxtime) < 0 )
        goto txdelay;

    setBcnRxParams();
    LMIC.rxsyms = LMIC.bcnRxsyms;
    LMIC.rxtime = LMIC.bcnRxtime;
    if( now - rxtime >= 0 ) {
        LMIC.osjob.func = FUNC_ADDR(processBeacon);
        os_radio(RADIO_RX);
        return;
    }
    os_setTimedCallback(&LMIC.osjob, rxtime, FUNC_ADDR(startRxBcn));
    return;
#endif // !DISABLE_BEACONS

  txdelay:
    EV(devCond, INFO, (e_.reason = EV::devCond_t::TX_DELAY,
                       e_.eui    = MAIN::CDEV->getEui(),
                       e_.info   = osticks2ms(txbeg-now),
                       e_.info2  = LMIC.seqnoUp-1));
    os_setTimedCallback(&LMIC.osjob, txbeg-TX_RAMPUP, FUNC_ADDR(runEngineUpdate));
}


void LMIC_setAdrMode (bit_t enabled) {
    LMIC.adrEnabled = enabled ? FCT_ADREN : 0;
}


//  Should we have/need an ext. API like this?
void LMIC_setDrTxpow (dr_t dr, s1_t txpow) {
    setDrTxpow(DRCHG_SET, dr, txpow);
}


void LMIC_shutdown (void) {
    os_clearCallback(&LMIC.osjob);
    os_radio(RADIO_RST);
    LMIC.opmode |= OP_SHUTDOWN;
}


void LMIC_reset (void) {
    EV(devCond, INFO, (e_.reason = EV::devCond_t::LMIC_EV,
                       e_.eui    = MAIN::CDEV->getEui(),
                       e_.info   = EV_RESET));
    os_radio(RADIO_RST);
    os_clearCallback(&LMIC.osjob);

    os_clearMem((xref2u1_t)&LMIC,SIZEOFEXPR(LMIC));
    LMIC.devaddr      =  0;
    LMIC.devNonce     =  os_getRndU2();
    LMIC.opmode       =  OP_NONE;
    LMIC.errcr        =  CR_4_5;
    LMIC.adrEnabled   =  FCT_ADREN;
    LMIC.dn2Dr        =  DR_DNW2;   // we need this for 2nd DN window of join accept
    LMIC.dn2Freq      =  FREQ_DNW2; // ditto
    LMIC.rxDelay      =  DELAY_DNW1;
#if !defined(DISABLE_PING)
    LMIC.ping.freq    =  FREQ_PING; // defaults for ping
    LMIC.ping.dr      =  DR_PING;   // ditto
    LMIC.ping.intvExp =  0xFF;
#endif // !DISABLE_PING
#if defined(CFG_us915)
    initDefaultChannels();
#endif
    DO_DEVDB(LMIC.devaddr,      devaddr);
    DO_DEVDB(LMIC.devNonce,     devNonce);
    DO_DEVDB(LMIC.dn2Dr,        dn2Dr);
    DO_DEVDB(LMIC.dn2Freq,      dn2Freq);
#if !defined(DISABLE_PING)
    DO_DEVDB(LMIC.ping.freq,    pingFreq);
    DO_DEVDB(LMIC.ping.dr,      pingDr);
    DO_DEVDB(LMIC.ping.intvExp, pingIntvExp);
#endif // !DISABLE_PING
}


void LMIC_init (void) {
    LMIC.opmode = OP_SHUTDOWN;
}


void LMIC_clrTxData (void) {
    LMIC.opmode &= ~(OP_TXDATA|OP_TXRXPEND|OP_POLL);
    LMIC.pendTxLen = 0;
    if( (LMIC.opmode & (OP_JOINING|OP_SCAN)) != 0 ) // do not interfere with JOINING
        return;
    os_clearCallback(&LMIC.osjob);
    os_radio(RADIO_RST);
    engineUpdate();
}


void LMIC_setTxData (void) {
    LMIC.opmode |= OP_TXDATA;
    if( (LMIC.opmode & OP_JOINING) == 0 )
        LMIC.txCnt = 0;             // cancel any ongoing TX/RX retries
    engineUpdate();
}


//
int LMIC_setTxData2 (u1_t port, xref2u1_t data, u1_t dlen, u1_t confirmed) {
    if( dlen > SIZEOFEXPR(LMIC.pendTxData) )
        return -2;
    if( data != (xref2u1_t)0 )
        os_copyMem(LMIC.pendTxData, data, dlen);
    LMIC.pendTxConf = confirmed;
    LMIC.pendTxPort = port;
    LMIC.pendTxLen  = dlen;
    LMIC_setTxData();
    return 0;
}


// Send a payload-less message to signal device is alive
void LMIC_sendAlive (void) {
    LMIC.opmode |= OP_POLL;
    engineUpdate();
}


// Check if other networks are around.
void LMIC_tryRejoin (void) {
    LMIC.opmode |= OP_REJOIN;
    engineUpdate();
}

//! \brief Setup given session keys
//! and put the MAC in a state as if
//! a join request/accept would have negotiated just these keys.
//! It is crucial that the combinations `devaddr/nwkkey` and `devaddr/artkey`
//! are unique within the network identified by `netid`.
//! NOTE: on Harvard architectures when session keys are in flash:
//!  Caller has to fill in LMIC.{nwk,art}Key  before and pass {nwk,art}Key are NULL
//! \param netid a 24 bit number describing the network id this device is using
//! \param devaddr the 32 bit session address of the device. It is strongly recommended
//!    to ensure that different devices use different numbers with high probability.
//! \param nwkKey  the 16 byte network session key used for message integrity.
//!     If NULL the caller has copied the key into `LMIC.nwkKey` before.
//! \param artKey  the 16 byte application router session key used for message confidentiality.
//!     If NULL the caller has copied the key into `LMIC.artKey` before.
void LMIC_setSession (u4_t netid, devaddr_t devaddr, xref2u1_t nwkKey, xref2u1_t artKey) {
    LMIC.netid = netid;
    LMIC.devaddr = devaddr;
    if( nwkKey != (xref2u1_t)0 )
        os_copyMem(LMIC.nwkKey, nwkKey, 16);
    if( artKey != (xref2u1_t)0 )
        os_copyMem(LMIC.artKey, artKey, 16);

#if defined(CFG_eu868)
    initDefaultChannels(0);
#endif

    LMIC.opmode &= ~(OP_JOINING|OP_TRACK|OP_REJOIN|OP_TXRXPEND|OP_PINGINI);
    LMIC.opmode |= OP_NEXTCHNL;
    stateJustJoined();
    DO_DEVDB(LMIC.netid,   netid);
    DO_DEVDB(LMIC.devaddr, devaddr);
    DO_DEVDB(LMIC.nwkKey,  nwkkey);
    DO_DEVDB(LMIC.artKey,  artkey);
    DO_DEVDB(LMIC.seqnoUp, seqnoUp);
    DO_DEVDB(LMIC.seqnoDn, seqnoDn);
}

// Enable/disable link check validation.
// LMIC sets the ADRACKREQ bit in UP frames if there were no DN frames
// for a while. It expects the network to provide a DN message to prove
// connectivity with a span of UP frames. If this no such prove is coming
// then the datarate is lowered and a LINK_DEAD event is generated.
// This mode can be disabled and no connectivity prove (ADRACKREQ) is requested
// nor is the datarate changed.
// This must be called only if a session is established (e.g. after EV_JOINED)
void LMIC_setLinkCheckMode (bit_t enabled) {
    LMIC.adrChanged = 0;
    LMIC.adrAckReq = enabled ? LINK_CHECK_INIT : LINK_CHECK_OFF;
}

// Sets the max clock error to compensate for (defaults to 0, which
// allows for +/- 640 at SF7BW250). MAX_CLOCK_ERROR represents +/-100%,
// so e.g. for a +/-1% error you would pass MAX_CLOCK_ERROR * 1 / 100.
void LMIC_setClockError(u2_t error) {
    LMIC.clockError = error;
}
