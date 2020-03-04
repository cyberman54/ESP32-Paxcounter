/*

///--> IMPORTANT LICENSE NOTE for timesync option 1 in this file <--///

PLEASE NOTE: There is a patent filed for the time sync algorithm used in the
code of this file. The shown implementation example is covered by the
repository's licencse, but you may not be eligible to deploy the applied
algorithm in applications without granted license by the patent holder.

You may use timesync option 2 if you do not want or cannot accept this.

*/

#include "timesync.h"

#if (TIME_SYNC_LORASERVER) && (TIME_SYNC_LORAWAN) && (HAS_LORA)
#error Duplicate timesync method selected. You must select either LORASERVER or LORAWAN timesync.
#endif

// Local logging tag
static const char TAG[] = __FILE__;

// timesync option 1: use external timeserver (for LoRAWAN < 1.0.3)

#if (TIME_SYNC_LORASERVER) && (HAS_LORA)

static TaskHandle_t timeSyncReqTask = NULL;
static bool timeSyncPending = false;
static uint8_t time_sync_seqNo = (uint8_t)random(TIMEREQUEST_MAX_SEQNO);
static uint8_t sample_idx = 0;
static uint32_t timesync_timestamp[TIME_SYNC_SAMPLES][no_of_timestamps] = {0};

// send time request message
void send_timesync_req(void) {
  // if a timesync handshake is pending then exit
  if (timeSyncPending)
    return;
  // else unblock timesync task
  else {
    ESP_LOGI(TAG, "[%0.3f] Timeserver sync request started", millis() / 1000.0);
    xTaskNotifyGive(timeSyncReqTask);
  }
}

// task for sending time sync requests
void IRAM_ATTR process_timesync_req(void *taskparameter) {

  uint32_t rcv_seq_no = TIMEREQUEST_FINISH, time_offset_ms;

  //  this task is an endless loop, waiting in blocked mode, until it is
  //  unblocked by send_timesync_req(). It then waits to be notified from
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

    // trigger and collect timestamp samples
    for (uint8_t i = 0; i < TIME_SYNC_SAMPLES; i++) {
      // send timesync request to timeserver
      payload.reset();
      payload.addByte(time_sync_seqNo);
      SendPayload(TIMEPORT, prio_high);

      // wait until recv_timesync_ans() signals a timestamp was received
      while (rcv_seq_no != time_sync_seqNo) {
        if (xTaskNotifyWait(0x00, ULONG_MAX, &rcv_seq_no,
                            pdMS_TO_TICKS(TIME_SYNC_TIMEOUT * 1000)) ==
            pdFALSE) {
          ESP_LOGW(TAG, "[%0.3f] Timesync handshake error: timeout",
                   millis() / 1000.0);
          goto finish; // no valid sequence received before timeout
        }
      }

      // calculate time diff from collected timestamps
      time_offset_ms += timesync_timestamp[sample_idx][timesync_rx] -
                        timesync_timestamp[sample_idx][timesync_tx];

      // increment and maybe wrap around seqNo, keeping it in time port range
      time_sync_seqNo++;
      if (time_sync_seqNo > TIMEREQUEST_MAX_SEQNO) {
        time_sync_seqNo = 0;
      }

      // increment index for timestamp array
      sample_idx++;

      // if last cycle, send finish char for closing timesync handshake,
      // else wait until time has come for next cycle
      if (i < TIME_SYNC_SAMPLES - 1) { // wait for next cycle
        vTaskDelay(pdMS_TO_TICKS(TIME_SYNC_CYCLE * 1000));
      } else { // finish timesync handshake
        payload.reset();
        payload.addByte(TIMEREQUEST_FINISH);
        SendPayload(RCMDPORT, prio_high);
        // open a receive window to get last time_sync_answer instantly
        LMIC_sendAlive();
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

// called from lorawan.cpp
void store_timestamp(uint32_t timestamp, timesync_t timestamp_type) {

  ESP_LOGD(TAG, "[%0.3f] seq#%d[%d]: timestamp(t%d)=%d", millis() / 1000.0,
           time_sync_seqNo, sample_idx, timestamp_type, timestamp);

  timesync_timestamp[sample_idx][timestamp_type] = timestamp;
}

// process timeserver timestamp answer, called by myRxCallback() in lorawan.cpp
int recv_timesync_ans(const uint8_t buf[], const uint8_t buf_len) {

  /*
  parse 7 byte timesync_answer:

  byte    meaning
  1       sequence number (taken from node's time_sync_req)
  2       timezone in 15 minutes steps
  3..6    current second (from epoch time 1970)
  7       1/250ths fractions of current second
  */

  // if no timesync handshake is pending then exit
  if (!timeSyncPending)
    return 0; // failure

  // extract 1 byte timerequest sequence number from payload
  uint8_t seq_no = buf[0];
  buf++;

  // if no time is available or spurious buffer then exit
  if (buf_len != TIME_SYNC_FRAME_LENGTH) {
    if (seq_no == 0xff)
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

    // extract 1 byte containing timezone offset
    // one step being 15min * 60sec = 900sec
    uint32_t timestamp_tzsec = buf[0] * 900; // timezone offset in secs
    buf++;

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
               millis() / 1000.0, seq_no, osticks2ms(os_getTime()) / 1000.0);

      // store time received from gateway
      store_timestamp(timestamp_sec, gwtime_sec);
      store_timestamp(timestamp_msec, gwtime_msec);
      store_timestamp(timestamp_tzsec, gwtime_tzsec);

      // inform processing task
      xTaskNotify(timeSyncReqTask, seq_no, eSetBits);

      return 1; // success
    } else {
      ESP_LOGW(TAG, "[%0.3f] Timeserver error: outdated time received",
               millis() / 1000.0);
      return 0; // failure
    }
  }
}

// create task for timeserver handshake processing, called from main.cpp
void timesync_init() {
  xTaskCreatePinnedToCore(process_timesync_req, // task function
                          "timesync_req",       // name of task
                          2048,                 // stack size of task
                          (void *)1,            // task parameter
                          3,                    // priority of the task
                          &timeSyncReqTask,     // task handle
                          1);                   // CPU core
}

#endif

// timesync option 2: use LoRAWAN network time (requires LoRAWAN >= 1.0.3)

#if (TIME_SYNC_LORAWAN) && (HAS_LORA)

static time_t networkUTCTime;

// send time request message
void send_timesync_req(void) {
  LMIC_requestNetworkTime(process_timesync_req, &networkUTCTime);
}

void IRAM_ATTR process_timesync_req(void *pVoidUserUTCTime, int flagSuccess) {
  // Explicit conversion from void* to uint32_t* to avoid compiler errors
  time_t *pUserUTCTime = (time_t *)pVoidUserUTCTime;

  // A struct that will be populated by LMIC_getNetworkTimeReference.
  // It contains the following fields:
  //  - tLocal: the value returned by os_GetTime() when the time
  //            request was sent to the gateway, and
  //  - tNetwork: the seconds between the GPS epoch and the time
  //              the gateway received the time request
  lmic_time_reference_t lmicTimeReference;

  if (flagSuccess != 1) {
    ESP_LOGW(TAG, "LoRaWAN network did not answer time request");
    return;
  }

  // Populate lmic_time_reference
  flagSuccess = LMIC_getNetworkTimeReference(&lmicTimeReference);
  if (flagSuccess != 1) {
    ESP_LOGW(TAG, "LoRaWAN time request failed");
    return;
  }

  // mask application irq to ensure accurate timing
  mask_user_IRQ();

  // Update networkUTCTime, considering the difference between GPS and UTC time
  *pUserUTCTime = lmicTimeReference.tNetwork + GPS_UTC_DIFF;
  // Add delay between the instant the time was transmitted and the current time
  uint16_t requestDelaymSec =
      osticks2ms(os_getTime() - lmicTimeReference.tLocal);

  // Update system time with time read from the network
  setMyTime(*pUserUTCTime, requestDelaymSec, _lora);

  // end of time critical section: release app irq lock
  unmask_user_IRQ();

} // user_request_network_time_callback
#endif // TIME_SYNC_LORAWAN