/*******************************************************************************
 * Copyright (c) 2015-2016 Matthijs Kooijman
 * Copyright (c) 2016-2018 MCCI Corporation
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * This the HAL to run LMIC on top of the Arduino environment.
 *******************************************************************************/
#ifndef _hal_hal_h_
#define _hal_hal_h_

static const int NUM_DIO = 3;

// be careful of alignment below.
struct lmic_pinmap {
    u1_t nss;                   // byte 0: pin for select
    u1_t rxtx;                  // byte 1: pin for rx/tx control
    u1_t rst;                   // byte 2: pin for reset
    u1_t dio[NUM_DIO];          // bytes 3..5: pins for DIO0, DOI1, DIO2
    u1_t mosi;                  // byte 9: pin for master out / slave in (write to LORA chip)
    u1_t miso;                  // byte 10: pin for master in / slave out (read from LORA chip)
    u1_t sck;                   // byte 11: pin for serial clock by master
    // true if we must set rxtx for rx_active, false for tx_active
    u1_t rxtx_rx_active;        // byte 6: polarity of rxtx active
    s1_t rssi_cal;              // byte 7: cal in dB -- added to RSSI
                                //   measured prior to decision.
                                //   Must include noise guardband!
    u4_t spi_freq;              // bytes 8..11: SPI freq in Hz.
};

// Use this for any unused pins.
const u1_t LMIC_UNUSED_PIN = 0xff;

// Declared here, to be defined and initialized by the application
// use os_init_ex() if you want not to use a const table.
extern const lmic_pinmap lmic_pins;

#endif // _hal_hal_h_
