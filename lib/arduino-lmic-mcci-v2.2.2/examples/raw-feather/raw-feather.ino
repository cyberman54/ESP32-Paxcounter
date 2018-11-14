/*

Module:  raw-feather.ino

Function:
        Slightly improved Raw test example, for Adafruit Feather M0 LoRa

Copyright notice and License:
        See LICENSE file accompanying this project.

Author:
        Matthijs Kooijman  2015
        Terry Moore, MCCI Corporation	April 2017

*/

/*******************************************************************************
 * Copyright (c) 2015 Matthijs Kooijman
 *
 * Permission is hereby granted, free of charge, to anyone
 * obtaining a copy of this document and accompanying files,
 * to do whatever they want with them without any restriction,
 * including, but not limited to, copying, modification and redistribution.
 * NO WARRANTY OF ANY KIND IS PROVIDED.
 *
 * This example transmits data on hardcoded channel and receives data
 * when not transmitting. Running this sketch on two nodes should allow
 * them to communicate.
 *******************************************************************************/

#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>

#include <stdarg.h>
#include <stdio.h>

// we formerly would check this configuration; but now there is a flag,
// in the LMIC, LMIC.noRXIQinversion;
// if we set that during init, we get the same effect.  If
// DISABLE_INVERT_IQ_ON_RX is defined, it means that LMIC.noRXIQinversion is
// treated as always set.
//
// #if !defined(DISABLE_INVERT_IQ_ON_RX)
// #error This example requires DISABLE_INVERT_IQ_ON_RX to be set. Update \
//        lmic_project_config.h in arduino-lmic/project_config to set it.
// #endif

// How often to send a packet. Note that this sketch bypasses the normal
// LMIC duty cycle limiting, so when you change anything in this sketch
// (payload length, frequency, spreading factor), be sure to check if
// this interval should not also be increased.
// See this spreadsheet for an easy airtime and duty cycle calculator:
// https://docs.google.com/spreadsheets/d/1voGAtQAjC1qBmaVuP1ApNKs1ekgUjavHuVQIXyYSvNc

#define TX_INTERVAL 2000        // milliseconds
#define RX_RSSI_INTERVAL 100    // milliseconds

// Pin mapping for Adafruit Feather M0 LoRa, etc.
#if defined(ARDUINO_SAMD_FEATHER_M0)
const lmic_pinmap lmic_pins = {
    .nss = 8,
    .rxtx = LMIC_UNUSED_PIN,
    .rst = 4,
    .dio = {3, 6, LMIC_UNUSED_PIN},
    .rxtx_rx_active = 0,
    .rssi_cal = 8,              // LBT cal for the Adafruit Feather M0 LoRa, in dB
    .spi_freq = 8000000,
};
#elif defined(ARDUINO_AVR_FEATHER32U4)
// Pin mapping for Adafruit Feather 32u4 LoRa, etc.
// Just like Feather M0 LoRa, but uses SPI at 1MHz; and that's only
// because MCCI doesn't have a test board; probably higher frequencies
// will work.
const lmic_pinmap lmic_pins = {
    .nss = 8,
    .rxtx = LMIC_UNUSED_PIN,
    .rst = 4,
    .dio = {3, 6, LMIC_UNUSED_PIN},
    .rxtx_rx_active = 0,
    .rssi_cal = 8,              // LBT cal for the Adafruit Feather M0 LoRa, in dB
    .spi_freq = 1000000,
};
#elif defined(ARDUINO_CATENA_4551)
const lmic_pinmap lmic_pins = {
        .nss = 7,
        .rxtx = 29,
        .rst = 8,
        .dio = { 25,    // DIO0 (IRQ) is D25
                 26,    // DIO1 is D26
                 27,    // DIO2 is D27
               },
        .rxtx_rx_active = 1,
        .rssi_cal = 10,
        .spi_freq = 8000000     // 8MHz
};
#else
# error "Unknown target"
#endif

// These callbacks are only used in over-the-air activation, so they are
// left empty here (we cannot leave them out completely unless
// DISABLE_JOIN is set in arduino-lmoc/project_config/lmic_project_config.h,
// otherwise the linker will complain).
void os_getArtEui (u1_t* buf) { }
void os_getDevEui (u1_t* buf) { }
void os_getDevKey (u1_t* buf) { }

// this gets callled by the library but we choose not to display any info;
// and no action is required.
void onEvent (ev_t ev) {
}

extern "C" {
void lmic_printf(const char *fmt, ...);
};

void lmic_printf(const char *fmt, ...) {
        if (! Serial.dtr())
                return;

        char buf[256];
        va_list ap;

        va_start(ap, fmt);
        (void) vsnprintf(buf, sizeof(buf) - 1, fmt, ap);
        va_end(ap);

        // in case we overflowed:
        buf[sizeof(buf) - 1] = '\0';
        if (Serial.dtr()) Serial.print(buf);
}

osjob_t txjob;
osjob_t timeoutjob;
static void tx_func (osjob_t* job);

// Transmit the given string and call the given function afterwards
void tx(const char *str, osjobcb_t func) {
  // the radio is probably in RX mode; stop it.
  os_radio(RADIO_RST);
  // wait a bit so the radio can come out of RX mode
  delay(1);

  // prepare data
  LMIC.dataLen = 0;
  while (*str)
    LMIC.frame[LMIC.dataLen++] = *str++;

  // set completion function.
  LMIC.osjob.func = func;

  // start the transmission
  os_radio(RADIO_TX);
  Serial.println("TX");
}

// Enable rx mode and call func when a packet is received
void rx(osjobcb_t func) {
  LMIC.osjob.func = func;
  LMIC.rxtime = os_getTime(); // RX _now_
  // Enable "continuous" RX (e.g. without a timeout, still stops after
  // receiving a packet)
  os_radio(RADIO_RXON);
  Serial.println("RX");
}

static void rxtimeout_func(osjob_t *job) {
  digitalWrite(LED_BUILTIN, LOW); // off
}

static void rx_func (osjob_t* job) {
  // Blink once to confirm reception and then keep the led on
  digitalWrite(LED_BUILTIN, LOW); // off
  delay(10);
  digitalWrite(LED_BUILTIN, HIGH); // on

  // Timeout RX (i.e. update led status) after 3 periods without RX
  os_setTimedCallback(&timeoutjob, os_getTime() + ms2osticks(3*TX_INTERVAL), rxtimeout_func);

  // Reschedule TX so that it should not collide with the other side's
  // next TX
  os_setTimedCallback(&txjob, os_getTime() + ms2osticks(TX_INTERVAL/2), tx_func);

  Serial.print("Got ");
  Serial.print(LMIC.dataLen);
  Serial.println(" bytes");
  Serial.write(LMIC.frame, LMIC.dataLen);
  Serial.println();

  // Restart RX
  rx(rx_func);
}

static void txdone_func (osjob_t* job) {
  rx(rx_func);
}

// log text to USART and toggle LED
static void tx_func (osjob_t* job) {
  // say hello
  tx("Hello, world!", txdone_func);
  // reschedule job every TX_INTERVAL (plus a bit of random to prevent
  // systematic collisions), unless packets are received, then rx_func
  // will reschedule at half this time.
  os_setTimedCallback(job, os_getTime() + ms2osticks(TX_INTERVAL + random(500)), tx_func);
}

// application entry point
void setup() {
  // delay(3000) makes recovery from botched images much easier, as it
  // gives the host time to break in to start a download. Without it,
  // you get to the crash before the host can break in.
  delay(3000);

  // even after the delay, we wait for the host to open the port. operator
  // bool(Serial) just checks dtr(), and it tosses in a 10ms delay.
  while(! Serial.dtr())
        /* wait for the PC */;

  Serial.begin(115200);
  Serial.println("Starting");

  #ifdef VCC_ENABLE
  // For Pinoccio Scout boards
  pinMode(VCC_ENABLE, OUTPUT);
  digitalWrite(VCC_ENABLE, HIGH);
  delay(1000);
  #endif

  pinMode(LED_BUILTIN, OUTPUT);

  // initialize runtime env
  os_init();

  // Set up these settings once, and use them for both TX and RX
#ifdef ARDUINO_ARCH_STM32
  LMIC_setClockError(10*65536/100);
#endif

#if defined(CFG_eu868)
  // Use a frequency in the g3 which allows 10% duty cycling.
  LMIC.freq = 869525000;
  // Use a medium spread factor. This can be increased up to SF12 for
  // better range, but then, the interval should be (significantly)
  // raised to comply with duty cycle limits as well.
  LMIC.datarate = DR_SF9;
  // Maximum TX power
  LMIC.txpow = 27;
#elif defined(CFG_us915)
  // make it easier for test, by pull the parameters up to the top of the
  // block. Ideally, we'd use the serial port to drive this; or have
  // a voting protocol where one side is elected the controller and
  // guides the responder through all the channels, powers, ramps
  // the transmit power from min to max, and measures the RSSI and SNR.
  // Even more amazing would be a scheme where the controller could
  // handle multiple nodes; in that case we'd have a way to do
  // production test and qualification. However, using an RWC5020A
  // is a much better use of development time.

  // set fDownlink true to use a downlink channel; false
  // to use an uplink channel. Generally speaking, uplink
  // is more interesting, because you can prove that gateways
  // *should* be able to hear you.
  const static bool fDownlink = false;

  // the downlink channel to be used.
  const static uint8_t kDownlinkChannel = 3;

  // the uplink channel to be used.
  const static uint8_t kUplinkChannel = 8 + 3;

  // this is automatically set to the proper bandwidth in kHz,
  // based on the selected channel.
  uint32_t uBandwidth;

  if (! fDownlink)
        {
        if (kUplinkChannel < 64)
                {
                LMIC.freq = US915_125kHz_UPFBASE +
                            kUplinkChannel * US915_125kHz_UPFSTEP;
                uBandwidth = 125;
                }
        else
                {
                LMIC.freq = US915_500kHz_UPFBASE +
                            (kUplinkChannel - 64) * US915_500kHz_UPFSTEP;
                uBandwidth = 500;
                }
        }
  else
        {
        // downlink channel
        LMIC.freq = US915_500kHz_DNFBASE +
                    kDownlinkChannel * US915_500kHz_DNFSTEP;
        uBandwidth = 500;
        }

  // Use a suitable spreading factor
  if (uBandwidth < 500)
        LMIC.datarate = US915_DR_SF7;         // DR4
  else
        LMIC.datarate = US915_DR_SF12CR;      // DR8

  // default tx power for US: 21 dBm
  LMIC.txpow = 21;
#elif defined(CFG_au921)
  // make it easier for test, by pull the parameters up to the top of the
  // block. Ideally, we'd use the serial port to drive this; or have
  // a voting protocol where one side is elected the controller and
  // guides the responder through all the channels, powers, ramps
  // the transmit power from min to max, and measures the RSSI and SNR.
  // Even more amazing would be a scheme where the controller could
  // handle multiple nodes; in that case we'd have a way to do
  // production test and qualification. However, using an RWC5020A
  // is a much better use of development time.

  // set fDownlink true to use a downlink channel; false
  // to use an uplink channel. Generally speaking, uplink
  // is more interesting, because you can prove that gateways
  // *should* be able to hear you.
  const static bool fDownlink = false;

  // the downlink channel to be used.
  const static uint8_t kDownlinkChannel = 3;

  // the uplink channel to be used.
  const static uint8_t kUplinkChannel = 8 + 3;

  // this is automatically set to the proper bandwidth in kHz,
  // based on the selected channel.
  uint32_t uBandwidth;

  if (! fDownlink)
        {
        if (kUplinkChannel < 64)
                {
                LMIC.freq = AU921_125kHz_UPFBASE +
                            kUplinkChannel * AU921_125kHz_UPFSTEP;
                uBandwidth = 125;
                }
        else
                {
                LMIC.freq = AU921_500kHz_UPFBASE +
                            (kUplinkChannel - 64) * AU921_500kHz_UPFSTEP;
                uBandwidth = 500;
                }
        }
  else
        {
        // downlink channel
        LMIC.freq = AU921_500kHz_DNFBASE +
                    kDownlinkChannel * AU921_500kHz_DNFSTEP;
        uBandwidth = 500;
        }

  // Use a suitable spreading factor
  if (uBandwidth < 500)
        LMIC.datarate = AU921_DR_SF7;         // DR4
  else
        LMIC.datarate = AU921_DR_SF12CR;      // DR8

  // default tx power for AU: 30 dBm
  LMIC.txpow = 30;
#elif defined(CFG_as923)
// make it easier for test, by pull the parameters up to the top of the
// block. Ideally, we'd use the serial port to drive this; or have
// a voting protocol where one side is elected the controller and
// guides the responder through all the channels, powers, ramps
// the transmit power from min to max, and measures the RSSI and SNR.
// Even more amazing would be a scheme where the controller could
// handle multiple nodes; in that case we'd have a way to do
// production test and qualification. However, using an RWC5020A
// is a much better use of development time.
        const static uint8_t kChannel = 0;
        uint32_t uBandwidth;

        LMIC.freq = AS923_F1 + kChannel * 200000;
        uBandwidth = 125;

        // Use a suitable spreading factor
        if (uBandwidth == 125)
                LMIC.datarate = AS923_DR_SF7;         // DR7
        else
                LMIC.datarate = AS923_DR_SF7B;        // DR8

        // default tx power for AS: 21 dBm
        LMIC.txpow = 16;

        if (LMIC_COUNTRY_CODE == LMIC_COUNTRY_CODE_JP)
                {
                LMIC.lbt_ticks = us2osticks(AS923JP_LBT_US);
                LMIC.lbt_dbmax = AS923JP_LBT_DB_MAX;
                }
#elif defined(CFG_in866)
// make it easier for test, by pull the parameters up to the top of the
// block. Ideally, we'd use the serial port to drive this; or have
// a voting protocol where one side is elected the controller and
// guides the responder through all the channels, powers, ramps
// the transmit power from min to max, and measures the RSSI and SNR.
// Even more amazing would be a scheme where the controller could
// handle multiple nodes; in that case we'd have a way to do
// production test and qualification. However, using an RWC5020A
// is a much better use of development time.
        const static uint8_t kChannel = 0;
        uint32_t uBandwidth;

        LMIC.freq = IN866_F1 + kChannel * 200000;
        uBandwidth = 125;

        LMIC.datarate = IN866_DR_SF7;         // DR7
        // default tx power for IN: 30 dBm
        LMIC.txpow = IN866_TX_EIRP_MAX_DBM;
#else
# error Unsupported LMIC regional configuration.
#endif


  // disable RX IQ inversion
  LMIC.noRXIQinversion = true;

  // This sets CR 4/5, BW125 (except for EU/AS923 DR_SF7B, which uses BW250)
  LMIC.rps = updr2rps(LMIC.datarate);

  Serial.print("Frequency: "); Serial.print(LMIC.freq / 1000000);
            Serial.print("."); Serial.print((LMIC.freq / 100000) % 10);
            Serial.print("MHz");
  Serial.print("  LMIC.datarate: "); Serial.print(LMIC.datarate);
  Serial.print("  LMIC.txpow: "); Serial.println(LMIC.txpow);
  Serial.println("Started");
  Serial.flush();

  // setup initial job
  os_setCallback(&txjob, tx_func);
}

void loop() {
  // execute scheduled jobs and events
  os_runloop_once();
}
