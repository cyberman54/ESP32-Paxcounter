/*

///--> IMPORTANT LICENSE NOTE for timesync option 1 in this file <--///

PLEASE NOTE: There is a patent filed for the time sync algorithm used in the
code of this file. The shown implementation example is covered by the
repository's licencse, but you may not be eligible to deploy the applied
algorithm in applications without granted license by the patent holder.

You may use timesync option 2 if you do not want or cannot accept this.

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
static uint16_t timestamp_msec;
static uint32_t timestamp_sec,
    timesync_timestamp[TIME_SYNC_SAMPLES][no_of_timestamps];
static TaskHandle_t timeSyncReqTask = NULL;

// create task for timeserver handshake processing, called from main.cpp
void timesync_init() {
  xTaskCreatePinnedToCore(timesync_processReq, // task function
                          "timesync_req",      // name of task
                          2048,                // stack size of task
                          (void *)1,           // task parameter
                          3,                   // priority of the task
                          &timeSyncReqTask,    // task handle
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
    xTaskNotifyGive(timeSyncReqTask);
  }
}

// task for processing time sync request
void IRAM_ATTR timesync_processReq(void *taskparameter) {

  uint32_t rcv_seq_no = TIMEREQUEST_FINISH, time_offset_ms;

  //  this task is an endless loop, waiting in blocked mode, until it is
  //  unblocked by timesync_sendReq(). It then waits to be notified from
  //  recv_timesync_ans(), which is called from RX callback in lorawan.cpp, each
  //  time a timestamp from timeserver arrived.

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
      // timesync option 1: use external timeserver (for LoRAWAN < 1.0.3)
      payload.reset();
      payload.addByte(time_sync_seqNo);
      SendPayload(TIMEPORT, prio_high);
#elif (TIME_SYNC_LORAWAN)
      // timesync option 2: use LoRAWAN network time (requires LoRAWAN >= 1.0.3)
      LMIC_requestNetworkTime(DevTimeAns_Cb, &time_sync_seqNo);
      // open a receive window to trigger DevTimeAns
      LMIC_sendAlive();
#endif

      // wait until a timestamp was received
      while (rcv_seq_no != time_sync_seqNo) {
        if (xTaskNotifyWait(0x00, ULONG_MAX, &rcv_seq_no,
                            pdMS_TO_TICKS(TIME_SYNC_TIMEOUT * 1000)) ==
            pdFALSE) {
          ESP_LOGW(TAG, "[%0.3f] Timesync handshake error: timeout",
                   millis() / 1000.0);
          goto finish; // no valid sequence received before timeout
        }
      }

      // calculate time diff from received timestamp
      time_offset_ms += timesync_timestamp[sample_idx][timesync_rx] -
                        timesync_timestamp[sample_idx][timesync_tx];

      // increment and maybe wrap around seqNo, keeping it in time port range
      WRAP(time_sync_seqNo, TIMEREQUEST_MAX_SEQNO);
      // increment index for timestamp array
      sample_idx++;

      // if last cycle, finish after, else pause until next cycle
      if (i < TIME_SYNC_SAMPLES - 1) { // wait for next cycle
        vTaskDelay(pdMS_TO_TICKS(TIME_SYNC_CYCLE * 1000));
      } else {
#if (TIME_SYNC_LORASERVER)
        // send finish char for closing timesync handshake
        payload.reset();
        payload.addByte(TIMEREQUEST_FINISH);
        SendPayload(RCMDPORT, prio_high);
        // open a receive window to get last time_sync_answer instantly
        LMIC_sendAlive();
#endif
      }

    } // end of for loop to collect timestamp samples

    // --- time critial part: evaluate timestamps and calculate time ---

    // mask application irq to ensure accurate timing
    mask_user_IRQ();

    // average time offset over the summed up difference
    // + add msec from recent gateway time, found with last sample_idx
    // + apply a compensation constant TIME_SYNC_FIXUP for processing time
    time_offset_ms /= TIME_SYNC_SAMPLES;
    time_offset_ms +=
        timesync_timestamp[sample_idx - 1][gwtime_msec] + TIME_SYNC_FIXUP;

    // calculate absolute time in UTC epoch: take latest time received from
    // gateway, convert to whole seconds, round to ceil, add fraction seconds
    setMyTime(timesync_timestamp[sample_idx - 1][gwtime_sec] +
                  time_offset_ms / 1000,
              time_offset_ms % 1000, _lora);

    // end of time critical section: release app irq lock
    unmask_user_IRQ();

  finish:
    timeSyncPending = false;

  } // infinite while(1)
}

// store incoming timestamps
void timesync_storeReq(uint32_t timestamp, timesync_t timestamp_type) {

  ESP_LOGD(TAG, "[%0.3f] seq#%d[%d]: timestamp(t%d)=%d", millis() / 1000.0,
           time_sync_seqNo, sample_idx, timestamp_type, timestamp);

  timesync_timestamp[sample_idx][timestamp_type] = timestamp;
}

#if (TIME_SYNC_LORASERVER)
// evaluate timerserver's timestamp answer, called by myRxCallback() in
// lorawan.cpp
int recv_timeserver_ans(const uint8_t buf[], const uint8_t buf_len) {

  /*
  parse 6 byte timesync_answer:

  byte    meaning
  1       sequence number (taken from node's time_sync_req)
  2..5    current second (from epoch time 1970)
  6       1/250ths fractions of current second
  */

  // if no timesync handshake is pending then exit
  if (!timeSyncPending)
    return 0; // failure

  // extract 1 byte timerequest sequence number from payload
  uint8_t seqNo = buf[0];
  buf++;

  // if no time is available or spurious buffer then exit
  if (buf_len != TIME_SYNC_FRAME_LENGTH) {
    if (seqNo == 0xff)
      ESP_LOGI(TAG, "[%0.3f] Timeserver error: no confident time available",
               millis() / 1000.0);
    else
      ESP_LOGW(TAG, "[%0.3f] Timeserver error: spurious data received",
               millis() / 1000.0);
    return 0; // failure
  }

  else { // we received a probably valid time frame

    // pointers to 4 bytes msb order
    uint32_t timestamp_sec, *timestamp_ptr;

    // extract 4 bytes containing gateway time in UTC seconds since unix
    // epoch and convert it to uint32_t, octet order is big endian
    timestamp_ptr = (uint32_t *)buf;
    // swap byte order from msb to lsb, note: this is a platform dependent hack
    timestamp_sec = __builtin_bswap32(*timestamp_ptr);
    buf += 4;

    // extract 1 byte containing fractional seconds in 2^-8 second steps
    // one step being 1/250th sec * 1000 = 4msec
    uint16_t timestamp_msec = buf[0] * 4;
    // calculate absolute time received from gateway
    time_t t = timestamp_sec + timestamp_msec / 1000;

    // we guess timepoint is recent if it is newer than code compile date
    if (timeIsValid(t)) {
      ESP_LOGD(TAG, "[%0.3f] Timesync request seq#%d rcvd at %0.3f",
               millis() / 1000.0, seqNo, osticks2ms(os_getTime()) / 1000.0);

      // store time received from gateway
      timesync_storeReq(timestamp_sec, gwtime_sec);
      timesync_storeReq(timestamp_msec, gwtime_msec);

      // inform processing task
      xTaskNotify(timeSyncReqTask, seqNo, eSetBits);

      return 1; // success
    } else {
      ESP_LOGW(TAG, "[%0.3f] Timeserver error: outdated time received",
               millis() / 1000.0);
      return 0; // failure
    }
  }
}

#elif (TIME_SYNC_LORAWAN)

void IRAM_ATTR DevTimeAns_Cb(void *pUserData, int flagSuccess) {
  // Explicit conversion from void* to uint8_t* to avoid compiler errors
  uint8_t *seqNo = (uint8_t *)pUserData;

  // mask application irq to ensure accurate timing
  mask_user_IRQ();

  // A struct that will be populated by LMIC_getNetworkTimeReference.
  // It contains the following fields:
  //  - tLocal: the value returned by os_GetTime() when the time
  //            request was sent to the gateway, and
  //  - tNetwork: the seconds between the GPS epoch and the time
  //              the gateway received the time request
  lmic_time_reference_t lmicTime;

  if (flagSuccess != 1) {
    ESP_LOGW(TAG, "Network did not answer time request");
    goto Finish;
  }

  if (time_sync_seqNo != *seqNo) {
    ESP_LOGW(TAG, "Network timesync handshake failed, seqNo#%u, *seqNo");
    goto Finish;
  }

  // Populate lmic_time_reference
  if ((LMIC_getNetworkTimeReference(&lmicTime)) != 1) {
    ESP_LOGW(TAG, "Network time request failed");
  goto Finish;
}

// Calculate UTCTime, considering the difference between GPS and UTC time
timestamp_sec = lmicTime.tNetwork + GPS_UTC_DIFF;
// Add delay between the instant the time was transmitted and the current time
timestamp_msec = osticks2ms(os_getTime() - lmicTime.tLocal);

// store time received from gateway
timesync_storeReq(timestamp_sec, gwtime_sec);
timesync_storeReq(timestamp_msec, gwtime_msec);

// inform processing task
xTaskNotify(timeSyncReqTask, *seqNo, eSetBits);

Finish :
    // end of time critical section: release app irq lock
    unmask_user_IRQ();
}

#endif

#endif // HAS_LORA