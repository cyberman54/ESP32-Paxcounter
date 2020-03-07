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

#define WRAP(v, top) (v++ > top ? 0 : v)

// Local logging tag
static const char TAG[] = __FILE__;

static bool timeSyncPending = false;
static uint8_t time_sync_seqNo = (uint8_t)random(TIMEREQUEST_MAX_SEQNO),
               sample_idx;
static uint32_t timesync_timestamp[TIME_SYNC_SAMPLES][no_of_timestamps];
static TaskHandle_t timeSyncProcTask = NULL;

// create task for timeserver handshake processing, called from main.cpp
void timesync_init() {
  xTaskCreatePinnedToCore(timesync_processReq, // task function
                          "timesync_proc",     // name of task
                          2048,                // stack size of task
                          (void *)1,           // task parameter
                          3,                   // priority of the task
                          &timeSyncProcTask,   // task handle
                          1);                  // CPU core
}

// kickoff asnychronous timesync handshake
void timesync_sendReq(void) {
  // if a timesync handshake is pending then exit
  if (timeSyncPending)
    return;
  // else clear array and unblock timesync task
  else {
    ESP_LOGI(TAG, "[%0.3f] Timeserver sync request seqNo#%d started",
             millis() / 1000.0, time_sync_seqNo);
    sample_idx = 0;
    xTaskNotifyGive(timeSyncProcTask);
  }
}

// task for processing time sync request
void IRAM_ATTR timesync_processReq(void *taskparameter) {

  uint32_t seqNo = TIMEREQUEST_END, time_offset_ms;

  //  this task is an endless loop, waiting in blocked mode, until it is
  //  unblocked by timesync_sendReq(). It then waits to be notified from
  //  timesync_serverAnswer(), which is called from LMIC each time a timestamp
  //  from the timesource via LORAWAN arrived.

  // --- asnychronous part: generate and collect timestamps from gateway ---

  while (1) {

    // wait for kickoff
    ulTaskNotifyTake(pdFALSE, portMAX_DELAY);
    timeSyncPending = true;
    time_offset_ms = 0;

    // wait until we are joined if we are not
    while (!LMIC.devaddr) {
      vTaskDelay(pdMS_TO_TICKS(5000));
    }

    // clear timestamp array
    timesync_timestamp[TIME_SYNC_SAMPLES][no_of_timestamps] = {0};

    // trigger and collect samples in timestamp array
    for (uint8_t i = 0; i < TIME_SYNC_SAMPLES; i++) {

// send timesync request to timeserver or networkserver
#if (TIME_SYNC_LORASERVER)
      // ask user's timeserver (for LoRAWAN < 1.0.3)
      payload.reset();
      payload.addByte(time_sync_seqNo);
      SendPayload(TIMEPORT, prio_high);
#elif (TIME_SYNC_LORAWAN)
      // ask network (requires LoRAWAN >= 1.0.3)
      LMIC_requestNetworkTime(timesync_serverAnswer, &time_sync_seqNo);
      // open a receive window to immediately get DevTimeAns
      LMIC_sendAlive();
#endif
      // wait until a timestamp was received
      if (xTaskNotifyWait(0x00, ULONG_MAX, &seqNo,
                          pdMS_TO_TICKS(TIME_SYNC_TIMEOUT * 1000)) == pdFALSE) {
        ESP_LOGW(TAG, "[%0.3f] Timesync aborted: timed out", millis() / 1000.0);
        goto Fail; // no timestamp received before timeout
      }

      // check if we are in handshake with server
      if (seqNo != time_sync_seqNo) {
        ESP_LOGW(TAG, "[%0.3f] Timesync aborted: handshake out of sync",
                 millis() / 1000.0);
        goto Fail;
      }

      // calculate time diff with received timestamp
      time_offset_ms += timesync_timestamp[sample_idx][timesync_rx] -
                        timesync_timestamp[sample_idx][timesync_tx];

      // increment and wrap around seqNo, keeping it in time port range
      // increment index for timestamp array
      WRAP(time_sync_seqNo, TIMEREQUEST_MAX_SEQNO);
      sample_idx++;

      // if we are not in last cycle, pause until next cycle
      if (i < TIME_SYNC_SAMPLES - 1)
        vTaskDelay(pdMS_TO_TICKS(TIME_SYNC_CYCLE * 1000));

    } // end of for loop to collect timestamp samples

    // --- time critial part: evaluate timestamps and calculate time ---

    // mask application irq to ensure accurate timing
    mask_user_IRQ();

    // calculate average time offset over the summed up difference
    // + add msec from recent gateway time, found with last sample_idx
    // + apply a compensation constant for processing times node + gateway
    time_offset_ms /= TIME_SYNC_SAMPLES;
    time_offset_ms +=
        timesync_timestamp[sample_idx - 1][gwtime_msec] + TIME_SYNC_FIXUP;

    // calculate absolute UTC time: take latest timestamp received from
    // gateway, convert to whole seconds, round to ceil, add fraction seconds
    setMyTime(timesync_timestamp[sample_idx - 1][gwtime_sec] +
                  time_offset_ms / 1000,
              time_offset_ms % 1000, _lora);

    // send timerequest end char to show timesync was successful
    payload.reset();
    payload.addByte(TIMEREQUEST_END);
    SendPayload(RCMDPORT, prio_high);
    goto Finish;

  Fail:
    // set retry timer
    timesyncer.attach(TIME_SYNC_INTERVAL_RETRY * 60, timeSync);

  Finish:
    // end of time critical section: release app irq lock
    unmask_user_IRQ();

  } // infinite while(1)
}

// store incoming timestamps
void timesync_storeReq(uint32_t timestamp, timesync_t timestamp_type) {

  ESP_LOGD(TAG, "[%0.3f] seq#%d[%d]: timestamp(t%d)=%d", millis() / 1000.0,
           time_sync_seqNo, sample_idx, timestamp_type, timestamp);

  timesync_timestamp[sample_idx][timestamp_type] = timestamp;
}

// callback function to receive time answer from network or answer
void IRAM_ATTR timesync_serverAnswer(void *pUserData, int flag) {

  // if no timesync handshake is pending then exit
  if (!timeSyncPending)
    return;

  // mask application irq to ensure accurate timing
  mask_user_IRQ();

  int rc = 0;
  uint8_t seqNo = *(uint8_t *)pUserData;
  uint16_t timestamp_msec;
  uint32_t timestamp_sec;

#if (TIME_SYNC_LORASERVER)

  // pUserData: contains pointer to payload buffer
  // flag: length of buffer

  // store LMIC time when we received the timesync answer
  timesync_storeReq(osticks2ms(os_getTime()), timesync_rx);

  //  parse timesync_answer:
  //  byte    meaning
  //  0       sequence number (taken from node's time_sync_req)
  //  1..4    current second (from UTC epoch)
  //  5       1/250ths fractions of current second

  // swap byte order from msb to lsb, note: this is a platform dependent hack
  timestamp_sec = __builtin_bswap32(*(uint32_t *)(pUserData + 1));

  // one step being 1/250th sec * 1000 = 4msec
  timestamp_msec = *(uint8_t *)(pUserData + 5);
  timestamp_msec *= 4;

  // if no time is available or spurious buffer then exit
  if (flag != TIME_SYNC_FRAME_LENGTH) {
    if (seqNo == TIMEREQUEST_END)
      ESP_LOGI(TAG, "[%0.3f] Timeserver error: no confident time available",
               millis() / 1000.0);
    else
      ESP_LOGW(TAG, "[%0.3f] Timeserver error: spurious data received",
               millis() / 1000.0);
    goto Exit; // failure
  }

  goto Finish;

#elif (TIME_SYNC_LORAWAN)

  // pUserData: contains pointer to SeqNo
  // flag: indicates if we got a recent time from the network

  if (flag != 1) {
    ESP_LOGW(TAG, "[%0.3f] Network did not answer time request",
             millis() / 1000.0);
    goto Exit;
  }

  // A struct that will be populated by LMIC_getNetworkTimeReference.
  // It contains the following fields:
  //  - tLocal: the value returned by os_GetTime() when the time
  //            request was sent to the gateway, and
  //  - tNetwork: the seconds between the GPS epoch and the time
  //              the gateway received the time request
  lmic_time_reference_t lmicTime;

  // Populate lmic_time_reference
  if ((LMIC_getNetworkTimeReference(&lmicTime)) != 1) {
    ESP_LOGW(TAG, "[%0.3f] Network time request failed", millis() / 1000.0);
    goto Exit;
  }

  // Calculate UTCTime, considering the difference between GPS and UTC time
  timestamp_sec = lmicTime.tNetwork + GPS_UTC_DIFF;
  // Add delay between the instant the time was transmitted and the current time
  timestamp_msec = osticks2ms(os_getTime() - lmicTime.tLocal);
  goto Finish;

#endif // (TIME_SYNC_LORAWAN)

Finish:
  // check if calculated time is recent
  if (timeIsValid(timestamp_sec)) {
    // store time received from gateway
    timesync_storeReq(timestamp_sec, gwtime_sec);
    timesync_storeReq(timestamp_msec, gwtime_msec);
    // success
    rc = 1;
  } else {
    ESP_LOGW(TAG, "[%0.3f] Timeserver error: outdated time received",
             millis() / 1000.0);
  }

Exit:
  // end of time critical section: release app irq lock
  unmask_user_IRQ();
  // inform processing task
  xTaskNotify(timeSyncProcTask, rc ? seqNo : TIMEREQUEST_END, eSetBits);
}

#endif // HAS_LORA