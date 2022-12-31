/*

///--> IMPORTANT LICENSE NOTE for timesync option 1 in this file <--///

PLEASE NOTE: There is a patent filed for the time sync algorithm used in the
code of this file for timesync option TIME_SYNC_LORASERVER. The shown
implementation example is covered by the repository's licencse, but you may not
be eligible to deploy the applied algorithm in applications without granted
license by the patent holder.

You may use timesync option TIME_SYNC_LORAWAN if you do not want or cannot
accept this.

*/

#if (HAS_LORA)

#if (TIME_SYNC_LORASERVER) && (TIME_SYNC_LORAWAN)
#error Duplicate timesync method selected. You must select either LORASERVER or LORAWAN timesync.
#endif

#include "timesync.h"


static bool timeSyncPending = false;
static uint8_t time_sync_seqNo = (uint8_t)random(TIME_SYNC_MAX_SEQNO),
               sample_idx;
static uint32_t timesync_timestamp[TIME_SYNC_SAMPLES][no_of_timestamps];
static TaskHandle_t timeSyncProcTask;

// create task for timeserver handshake processing, called from main.cpp
void timesync_init(void) {
  xTaskCreatePinnedToCore(timesync_processReq, // task function
                          "timesync_proc",     // name of task
                          4096,                // stack size of task
                          (void *)1,           // task parameter
                          7,                   // priority of the task
                          &timeSyncProcTask,   // task handle
                          1);                  // CPU core
}

// kickoff asnychronous timesync handshake
void timesync_request(void) {
  // exit if a timesync handshake is already running
  if (timeSyncPending)
    return;
  // start timesync handshake
  else {
    ESP_LOGI(TAG, "[%0.3f] Timeserver sync request started, seqNo#%d",
             _seconds(), time_sync_seqNo);
    xTaskNotifyGive(timeSyncProcTask); // unblock timesync task
  }
}

// task for processing time sync request
void timesync_processReq(void *taskparameter) {
  uint32_t rcv_seqNo = TIME_SYNC_END_FLAG;
  uint32_t time_offset_sec = 0, time_offset_ms = 0;

  //  this task is an endless loop, waiting in blocked mode, until it is
  //  unblocked by timesync_request(). It then waits to be notified from
  //  timesync_serverAnswer(), which is called from LMIC each time a timestamp
  //  from the timesource via LORAWAN arrived.

  // --- asnychronous part: generate and collect timestamps from gateway ---

  while (1) {
    // wait for kickoff
    ulTaskNotifyTake(pdFALSE, portMAX_DELAY);

    // initialize flag and counters
    timeSyncPending = true;
    time_offset_ms = sample_idx = 0;
    if (++time_sync_seqNo > TIME_SYNC_MAX_SEQNO)
      time_sync_seqNo = 0;

    // wait until we are joined if we are not
    while (!LMIC.devaddr) {
      vTaskDelay(pdMS_TO_TICKS(3000));
    }

    // collect timestamp samples in timestamp array
    for (int8_t i = 0; i < TIME_SYNC_SAMPLES; i++) {
// send timesync request
#if (TIME_SYNC_LORASERVER) // ask user's timeserver (for LoRAWAN < 1.0.3)
      payload.reset();
      payload.addByte(time_sync_seqNo);
      SendPayload(TIMEPORT);
#elif (TIME_SYNC_LORAWAN) // ask network (requires LoRAWAN >= 1.0.3)
      LMIC_requestNetworkTime(timesync_serverAnswer, &time_sync_seqNo);
      // trigger to immediately get DevTimeAns from class A device
      LMIC_sendAlive();
#endif
      // wait until a timestamp was received
      if (xTaskNotifyWait(0x00, ULONG_MAX, &rcv_seqNo,
                          pdMS_TO_TICKS(TIME_SYNC_TIMEOUT * 1000)) == pdFALSE) {
        ESP_LOGW(TAG, "[d%0.3f] Timesync aborted: timed out", _seconds());
        goto Fail; // no timestamp received before timeout
      }

      // check if we are in handshake with server
      if (rcv_seqNo != time_sync_seqNo) {
        ESP_LOGW(TAG, "[%0.3f] Timesync aborted: handshake out of sync",
                 _seconds());
        goto Fail;
      }

#if (TIME_SYNC_LORASERVER)
      // calculate time diff with received timestamp
      time_offset_ms += timesync_timestamp[sample_idx][timesync_rx] -
                        timesync_timestamp[sample_idx][timesync_tx];
#endif

      // increment sample index
      sample_idx++;

      // if we are not in last cycle, pause until next cycle
      if (i < TIME_SYNC_SAMPLES - 1)
        vTaskDelay(pdMS_TO_TICKS(TIME_SYNC_CYCLE * 1000));
    } // for i

    // --- time critial part: evaluate timestamps and calculate time ---

    // mask application irq to ensure accurate timing
    mask_user_IRQ();

    // calculate average time offset over the summed up difference
    time_offset_ms /= TIME_SYNC_SAMPLES;

    // add milliseconds from latest gateway time, and apply a compensation
    // constant for processing times on node and gateway, strip full seconds
    time_offset_ms += timesync_timestamp[sample_idx - 1][gwtime_msec];
    time_offset_ms -= TIME_SYNC_FIXUP;
    time_offset_ms %= 1000;

    // take latest timestamp received from gateway
    // and add time difference rounded to whole seconds
    time_offset_sec = timesync_timestamp[sample_idx - 1][gwtime_sec];
    time_offset_sec += time_offset_ms / 1000;

    setMyTime(time_offset_sec, time_offset_ms, _lora);

    // send timesync end char to show timesync was successful
    payload.reset();
    payload.addByte(TIME_SYNC_END_FLAG);
    SendPayload(TIMEPORT);
    goto Finish;

  Fail:
    // set retry timer
    timesyncer.attach(TIME_SYNC_INTERVAL_RETRY * 60, setTimeSyncIRQ);
    // intentionally fallthrough to Finish here

  Finish:
    // end of time critical section: release app irq lock
    timeSyncPending = false;
    unmask_user_IRQ();
  } // infinite while(1)
}

// store incoming timestamps
void timesync_store(uint32_t timestamp, timesync_t timestamp_type) {
  ESP_LOGD(TAG, "[%0.3f] seq#%d[%d]: t%d=%d", _seconds(), time_sync_seqNo - 1,
           sample_idx, timestamp_type, timestamp);
  timesync_timestamp[sample_idx][timestamp_type] = timestamp;
}

// callback function to receive time answer from network or answer
void timesync_serverAnswer(void *pUserData, int flag) {
#if (HAS_LORA_TIME)

  // if no timesync handshake is pending then exit
  if (!timeSyncPending)
    return;

  // mask application irq to ensure accurate timing
  mask_user_IRQ();

  // return code: 0 = failed / 1 = success
  int rc = 0;
  // cast back void parameter to a pointer
  uint8_t *p = (uint8_t *)pUserData, rcv_seqNo = *p;
  uint16_t timestamp_msec = 0;
  uint32_t timestamp_sec = 0;

#if (TIME_SYNC_LORASERVER)

  // pUserData: contains pointer (32bit) to payload buffer
  // flag: length of buffer

  // Store the instant the time request of the node was received on the gateway
  timesync_store(osticks2ms(os_getTime()), timesync_rx);

  //  parse pUserData:
  //  p       type      meaning
  //  +0      uint8_t   sequence number (taken from node's time_sync_req)
  //  +1      uint32_t  current second (from UTC epoch)
  //  +4      uint8_t   1/250ths fractions of current second

  // swap byte order from msb to lsb, note: this is a platform dependent hack
  timestamp_sec = __builtin_bswap32(*(uint32_t *)(++p));

  // one step being 1/250th sec * 1000 = 4msec
  timestamp_msec = *(p += 4) * 4;

  // if no time is available or spurious buffer then exit
  if (flag != TIME_SYNC_FRAME_LENGTH) {
    if (rcv_seqNo == TIME_SYNC_END_FLAG)
      ESP_LOGI(TAG, "[%0.3f] Timeserver error: no confident time available",
               _seconds());
    else
      ESP_LOGW(TAG, "[%0.3f] Timeserver error: spurious data received",
               _seconds());
    goto Exit; // failure
  }

  goto Finish;

#elif (TIME_SYNC_LORAWAN)

  // pUserData: contains pointer to SeqNo (not needed here)
  // flag: indicates if we got a recent time from the network

  int32_t delay_msec;
  lmic_time_reference_t lmicTime;

  if (flag != 1) {
    ESP_LOGW(TAG, "[%0.3f] Network did not answer time request", _seconds());
    goto Exit;
  }

  // Populate lmic_time_reference
  flag = LMIC_getNetworkTimeReference(&lmicTime);
  if (flag != 1) {
    ESP_LOGW(TAG, "[%0.3f] Network time request failed", _seconds());
    goto Exit;
  }

  // Calculate UTCTime, considering the difference between GPS and UTC time
  // epoch, and the leap seconds
  timestamp_sec = lmicTime.tNetwork + GPS_UTC_DIFF - LEAP_SECS_SINCE_GPSEPOCH;
  ESP_LOGD(TAG, "lmicTime.tNetwork = %d", timestamp_sec);

  // Add the delay between the instant the time was transmitted and
  // the current time
  delay_msec = osticks2ms(os_getTime() - lmicTime.tLocal);
  timestamp_sec += delay_msec / 1000;
  timestamp_msec += delay_msec % 1000;

  goto Finish;

#endif // (TIME_SYNC_LORAWAN)

Finish:
  // check if calculated time is recent
  if (timeIsValid(timestamp_sec)) {
    // store time received from gateway
    timesync_store(timestamp_sec, gwtime_sec);
    timesync_store(timestamp_msec, gwtime_msec);
    // success
    rc = 1;
  } else {
    ESP_LOGW(TAG, "[%0.3f] Timeserver error: outdated time received",
             _seconds());
  }

Exit:
  // end of time critical section: release app irq lock
  unmask_user_IRQ();
  // inform processing task
  xTaskNotify(timeSyncProcTask, (rc ? rcv_seqNo : TIME_SYNC_END_FLAG),
              eSetBits);

#endif // (HAS_LORA_TIME)
}

#endif // HAS_LORA