/*******************************************************************************
 * Copyright (c) 2015 Matthijs Kooijman
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

#if defined(ESP32) || defined(NRF51)
#define LMIC_SPI_PINS_IN_MAPPING
    struct lmic_pinmap {
        u1_t mosi;
        u1_t miso;
        u1_t sck;
        u1_t nss;
        u1_t rxtx;
        u1_t rst;
        u1_t dio[NUM_DIO];
    };
#else
    struct lmic_pinmap {
        u1_t nss;
        u1_t rxtx;
        u1_t rst;
        u1_t dio[NUM_DIO];
    };
#endif

// Use this for any unused pins.
const u1_t LMIC_UNUSED_PIN = 0xff;

// Declared here, to be defined an initialized by the application
extern const lmic_pinmap lmic_pins;

#endif // _hal_hal_h_
