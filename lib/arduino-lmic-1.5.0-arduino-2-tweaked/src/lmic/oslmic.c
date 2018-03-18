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

#include "lmic.h"
#include <stdbool.h>

// RUNTIME STATE
static struct {
    osjob_t* scheduledjobs;
    osjob_t* runnablejobs;
} OS;

void os_init () {
    memset(&OS, 0x00, sizeof(OS));
    hal_init();
    radio_init();
    LMIC_init();
}

ostime_t os_getTime () {
    return hal_ticks();
}

static u1_t unlinkjob (osjob_t** pnext, osjob_t* job) {
    for( ; *pnext; pnext = &((*pnext)->next)) {
        if(*pnext == job) { // unlink
            *pnext = job->next;
            return 1;
        }
    }
    return 0;
}

// clear scheduled job
void os_clearCallback (osjob_t* job) {
    hal_disableIRQs();
    u1_t res = unlinkjob(&OS.scheduledjobs, job) || unlinkjob(&OS.runnablejobs, job);
    hal_enableIRQs();
    #if LMIC_DEBUG_LEVEL > 1
        if (res)
            lmic_printf("%lu: Cleared job %p\n", os_getTime(), job);
    #endif
}

// schedule immediately runnable job
void os_setCallback (osjob_t* job, osjobcb_t cb) {
    osjob_t** pnext;
    hal_disableIRQs();
    // remove if job was already queued
    os_clearCallback(job);
    // fill-in job
    job->func = cb;
    job->next = NULL;
    // add to end of run queue
    for(pnext=&OS.runnablejobs; *pnext; pnext=&((*pnext)->next));
    *pnext = job;
    hal_enableIRQs();
    #if LMIC_DEBUG_LEVEL > 1
        lmic_printf("%lu: Scheduled job %p, cb %p ASAP\n", os_getTime(), job, cb);
    #endif
}

// schedule timed job
void os_setTimedCallback (osjob_t* job, ostime_t time, osjobcb_t cb) {
    osjob_t** pnext;
    hal_disableIRQs();
    // remove if job was already queued
    os_clearCallback(job);
    // fill-in job
    job->deadline = time;
    job->func = cb;
    job->next = NULL;
    // insert into schedule
    for(pnext=&OS.scheduledjobs; *pnext; pnext=&((*pnext)->next)) {
        if((*pnext)->deadline - time > 0) { // (cmp diff, not abs!)
            // enqueue before next element and stop
            job->next = *pnext;
            break;
        }
    }
    *pnext = job;
    hal_enableIRQs();
    #if LMIC_DEBUG_LEVEL > 1
        lmic_printf("%lu: Scheduled job %p, cb %p at %lu\n", os_getTime(), job, cb, time);
    #endif
}

// execute jobs from timer and from run queue
void os_runloop () {
    while(1) {
        os_runloop_once();
    }
}

void os_runloop_once() {
    #if LMIC_DEBUG_LEVEL > 1
        bool has_deadline = false;
    #endif
    osjob_t* j = NULL;
    hal_disableIRQs();
    // check for runnable jobs
    if(OS.runnablejobs) {
        j = OS.runnablejobs;
        OS.runnablejobs = j->next;
    } else if(OS.scheduledjobs && hal_checkTimer(OS.scheduledjobs->deadline)) { // check for expired timed jobs
        j = OS.scheduledjobs;
        OS.scheduledjobs = j->next;
        #if LMIC_DEBUG_LEVEL > 1
            has_deadline = true;
        #endif
    } else { // nothing pending
        hal_sleep(); // wake by irq (timer already restarted)
    }
    hal_enableIRQs();
    if(j) { // run job callback
        #if LMIC_DEBUG_LEVEL > 1
            lmic_printf("%lu: Running job %p, cb %p, deadline %lu\n", os_getTime(), j, j->func, has_deadline ? j->deadline : 0);
        #endif
        j->func(j);
    }
}
