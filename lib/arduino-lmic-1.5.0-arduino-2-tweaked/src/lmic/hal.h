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

#ifndef _hal_hpp_
#define _hal_hpp_

#ifdef __cplusplus
extern "C"{
#endif

/*
 * initialize hardware (IO, SPI, TIMER, IRQ).
 */
void hal_init (void);

/*
 * drive radio NSS pin (0=low, 1=high).
 */
void hal_pin_nss (u1_t val);

/*
 * drive radio RX/TX pins (0=rx, 1=tx).
 */
void hal_pin_rxtx (u1_t val);

/*
 * control radio RST pin (0=low, 1=high, 2=floating)
 */
void hal_pin_rst (u1_t val);

/*
 * perform 8-bit SPI transaction with radio.
 *   - write given byte 'outval'
 *   - read byte and return value
 */
u1_t hal_spi (u1_t outval);

/*
 * disable all CPU interrupts.
 *   - might be invoked nested
 *   - will be followed by matching call to hal_enableIRQs()
 */
void hal_disableIRQs (void);

/*
 * enable CPU interrupts.
 */
void hal_enableIRQs (void);

/*
 * put system and CPU in low-power mode, sleep until interrupt.
 */
void hal_sleep (void);

/*
 * return 32-bit system time in ticks.
 */
u4_t hal_ticks (void);

/*
 * busy-wait until specified timestamp (in ticks) is reached.
 */
void hal_waitUntil (u4_t time);

/*
 * check and rewind timer for target time.
 *   - return 1 if target time is close
 *   - otherwise rewind timer for target time or full period and return 0
 */
u1_t hal_checkTimer (u4_t targettime);

/*
 * perform fatal failure action.
 *   - called by assertions
 *   - action could be HALT or reboot
 */
void hal_failed (const char *file, u2_t line);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // _hal_hpp_
