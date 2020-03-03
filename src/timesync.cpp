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
static uint8_t sample_idx = 0;
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

      ESP_LOGD(TAG, "sample_idx = %d", sample_idx);

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

  ESP_LOGD(TAG, "[%0.3f] seq#%d[%d]: timestamp(t%d)=%d",
           millis() / 1000.0, time_sync_seqNo, sample_idx, timestamp_type,
           timestamp);

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

    // pointers to 4 bytes containing UTC seconds since unix epoch, msb
    uint32_t timestamp_sec, *timestamp_ptr;

    // extract 1 byte timezone from payload (one step being 15min * 60s = 900s)
    // uint32_t timezone_sec = buf[0] * 900; // for future use
    buf++;

    // extract 4 bytes timestamp from payload
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
      ESP_LOGD(TAG, "[%0.3f] Timesync request seq#%d rcvd at %0.3f",
               millis() / 1000.0, seq_no, osticks2ms(os_getTime()) / 1000.0);

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
