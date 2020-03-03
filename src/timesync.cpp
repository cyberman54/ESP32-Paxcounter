/*

///--> IMPORTANT LICENSE NOTE for this file <--///

PLEASE NOTE: There is a patent filed for the time sync algorithm used in the
code of this file. The shown implementation example is covered by the
repository's licencse, but you may not be eligible to deploy the applied
algorithm in applications without granted license by the patent holder.

*/

#if (TIME_SYNC_LORASERVER) && (HAS_LORA)

#include "timesync.h"

// Local logging tag
static const char TAG[] = __FILE__;

TaskHandle_t timeSyncReqTask = NULL;

static uint8_t time_sync_seqNo = (uint8_t)random(TIMEREQUEST_MAX_SEQNO);
static bool timeSyncPending = false;
static uint32_t timesync_timestamp[TIME_SYNC_SAMPLES][no_of_timestamps] = {0};

// send time request message
void send_timesync_req() {

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
void process_timesync_req(void *taskparameter) {

  uint8_t i;
  uint32_t seq_no, time_offset_ms;

  // --- asnychronous part: generate and collect timestamps from gateway ---

  while (1) {

    // wait for kickoff
    ulTaskNotifyTake(pdFALSE, portMAX_DELAY);
    timeSyncPending = true;

    // wait until we are joined if we are not
    while (!LMIC.devaddr) {
      vTaskDelay(pdMS_TO_TICKS(3000));
    }

    // generate and collect timestamp samples
    for (i = 0; i < TIME_SYNC_SAMPLES; i++) {
      // send sync request to server
      payload.reset();
      payload.addByte(time_sync_seqNo);
      SendPayload(TIMEPORT, prio_high);

      // wait for a valid timestamp from recv_timesync_ans()
      while (seq_no != time_sync_seqNo) {
        if (xTaskNotifyWait(0x00, ULONG_MAX, &seq_no,
                            pdMS_TO_TICKS(TIME_SYNC_TIMEOUT * 1000)) ==
            pdFALSE) {
          ESP_LOGW(TAG, "[%0.3f] Timesync handshake error: timeout",
                   millis() / 1000.0);
          goto finish; // no valid sequence received before timeout
        }
      }

      // wrap around seqNo, keeping it in time port range
      time_sync_seqNo++;
      if (time_sync_seqNo > TIMEREQUEST_MAX_SEQNO) {
        time_sync_seqNo = 0;
      }

      if (i < TIME_SYNC_SAMPLES - 1) {
        // wait until next cycle
        vTaskDelay(pdMS_TO_TICKS(TIME_SYNC_CYCLE * 1000));
      } else { // before sending last time sample...
        // ...send flush to open a receive window for last time_sync_answer
        payload.reset();
        payload.addByte(0x99);
        SendPayload(RCMDPORT, prio_high);
        // ...send a alive open a receive window for last time_sync_answer
        LMIC_sendAlive();
      }

    } // end of for loop to collect timestamp samples

    // --- time critial part: evaluate timestamps and calculate time ---

    // mask application irq to ensure accurate timing
    mask_user_IRQ();

    time_offset_ms = 0;

    for (i = 0; i < TIME_SYNC_SAMPLES; i++) {
      // calculate time diff from collected timestamps and apply
      // a compensation constant TIME_SYNC_FIXUP for processing time

      time_offset_ms += timesync_timestamp[i][timesync_rx] -
                        timesync_timestamp[i][timesync_tx] + TIME_SYNC_FIXUP;
    }

    // decrement i for subscription to latest element in timestamp array
    i--;

    // average time offset over all collected diffs and add gateway time msec
    time_offset_ms /= TIME_SYNC_SAMPLES;
    time_offset_ms += timesync_timestamp[i][gwtime_msec];

    // calculate absolute time in UTC epoch: take latest time received from
    // gateway, convert to whole seconds, round to ceil, add fraction seconds

    setMyTime(timesync_timestamp[i][gwtime_sec] + time_offset_ms / 1000,
              time_offset_ms % 1000, _lora);

  finish:
    // end of time critical section: release app irq lock
    timeSyncPending = false;
    unmask_user_IRQ();

  } // infinite while(1)
}

// called from lorawan.cpp
void store_timestamp(uint32_t timestamp, timesync_t timestamp_type) {

  uint8_t seq = time_sync_seqNo % TIME_SYNC_SAMPLES;
  timesync_timestamp[seq][timestamp_type] = timestamp;

  ESP_LOGD(TAG, "[%0.3f] Timesync seq#%d: timestamp(%d) %d stored",
           millis() / 1000.0, seq, timestamp_type, timestamp);
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

  // extract 1 byte timerequest sequence number from buffer
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

    uint8_t k = seq_no % TIME_SYNC_SAMPLES;

    // pointers to 4 bytes containing UTC seconds since unix epoch, msb
    uint32_t timestamp_sec, *timestamp_ptr;

    // extract 1 byte timezone from buffer (one step being 15min * 60s = 900s)
    // uint32_t timezone_sec = buf[0] * 900; // for future use
    buf++;

    // extract 4 bytes timestamp from buffer
    // and convert it to uint32_t, octet order is big endian
    timestamp_ptr = (uint32_t *)buf;
    // swap byte order from msb to lsb, note: this is platform dependent
    timestamp_sec = __builtin_bswap32(*timestamp_ptr);
    buf += 4;
    // extract 1 byte fractional seconds in 2^-8 second steps
    // (= 1/250th sec), we convert this to ms
    uint16_t timestamp_msec = 4 * buf[0];
    // calculate absolute time received from gateway
    time_t t = timestamp_sec + timestamp_msec / 1000;

    // we guess timepoint is recent if it is newer than code compile date
    if (timeIsValid(t)) {
      ESP_LOGD(TAG, "[%0.3f] Timesync request #%d of %d rcvd at %0.3f",
               millis() / 1000.0, k + 1, TIME_SYNC_SAMPLES,
               osticks2ms(os_getTime()) / 1000.0);

      // store time received from gateway
      store_timestamp(timestamp_sec, gwtime_sec);
      store_timestamp(timestamp_msec, gwtime_msec);

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
