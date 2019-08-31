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

using namespace std::chrono;

typedef std::chrono::system_clock myClock;
typedef myClock::time_point myClock_timepoint;
typedef std::chrono::duration<long long int, std::ratio<1, 1000>>
    myClock_msecTick;

TaskHandle_t timeSyncReqTask = NULL;

static uint8_t time_sync_seqNo = random(TIMEANSWERPORT_MIN, TIMEANSWERPORT_MAX);
static bool timeSyncPending = false;
static myClock_timepoint time_sync_tx[TIME_SYNC_SAMPLES];
static myClock_timepoint time_sync_rx[TIME_SYNC_SAMPLES];

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

  uint8_t k;
  uint16_t time_to_set_fraction_msec;
  uint32_t seq_no = 0, time_to_set;
  auto time_offset_ms = myClock_msecTick::zero();

  while (1) {

    // reset all timestamps before next sync run
    time_offset_ms = myClock_msecTick::zero();
    for (uint8_t i = 0; i < TIME_SYNC_SAMPLES; i++)
      time_sync_tx[i] = time_sync_rx[i] = myClock_timepoint();

    // wait for kickoff
    ulTaskNotifyTake(pdFALSE, portMAX_DELAY);
    timeSyncPending = true;

    // wait until we are joined if we are not
    while (!LMIC.devaddr) {
      vTaskDelay(pdMS_TO_TICKS(3000));
    }

    // collect timestamp samples
    for (uint8_t i = 0; i < TIME_SYNC_SAMPLES; i++) {
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

      // process answer
      k = seq_no % TIME_SYNC_SAMPLES;

      // calculate time diff from collected timestamps
      time_offset_ms += time_point_cast<milliseconds>(time_sync_rx[k]) -
                        time_point_cast<milliseconds>(time_sync_tx[k]);

      // wrap around seqNo, keeping it in time port range
      time_sync_seqNo = (time_sync_seqNo < TIMEANSWERPORT_MAX)
                            ? time_sync_seqNo + 1
                            : TIMEANSWERPORT_MIN;

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

    // mask application irq to ensure accurate timing
    mask_user_IRQ();

    // average time offset over all collected diffs
    time_offset_ms /= TIME_SYNC_SAMPLES;

    // calculate time offset with millisecond precision using LMIC's time base,
    // since we use LMIC's ostime_t txEnd as tx timestamp.
    // Also apply calibration const to compensate processing time.
    time_offset_ms +=
        milliseconds(osticks2ms(os_getTime())) + milliseconds(TIME_SYNC_FIXUP);

    // calculate absolute time in UTC epoch: convert to whole seconds, round to
    // ceil, and calculate fraction milliseconds
    time_to_set = (uint32_t)(time_offset_ms.count() / 1000) + 1;
    // calculate fraction milliseconds
    time_to_set_fraction_msec = (uint16_t)(time_offset_ms.count() % 1000);

    setMyTime(time_to_set, time_to_set_fraction_msec, _lora);

  finish:
    // end of time critical section: release app irq lock
    timeSyncPending = false;
    unmask_user_IRQ();

  } // infinite while(1)
}

// called from lorawan.cpp after time_sync_req was sent
void store_time_sync_req(uint32_t timestamp) {

  // if no timesync handshake is pending then exit
  if (!timeSyncPending)
    return;

  uint8_t k = time_sync_seqNo % TIME_SYNC_SAMPLES;
  time_sync_tx[k] += milliseconds(timestamp);

  ESP_LOGD(TAG, "[%0.3f] Timesync request #%d of %d sent at %d.%03d",
           millis() / 1000.0, k + 1, TIME_SYNC_SAMPLES, timestamp / 1000,
           timestamp % 1000);
}

// process timeserver timestamp answer, called from lorawan.cpp
int recv_timesync_ans(const uint8_t seq_no, const uint8_t buf[], const uint8_t buf_len) {

  // if no timesync handshake is pending then exit
  if (!timeSyncPending)
    return 0; // failure

  // if no time is available or spurious buffer then exit
  if (buf_len != TIME_SYNC_FRAME_LENGTH) {
    if (buf[0] == 0xff)
      ESP_LOGI(TAG, "[%0.3f] Timeserver error: no confident time available",
               millis() / 1000.0);
    else
      ESP_LOGW(TAG, "[%0.3f] Timeserver error: spurious data received",
               millis() / 1000.0);
    return 0; // failure
  }

  else { // we received a probably valid time frame

    uint8_t k = seq_no % TIME_SYNC_SAMPLES;

    // the 5th byte contains the fractional seconds in 2^-8 second steps
    // (= 1/250th sec), we convert this to ms
    uint16_t timestamp_msec = 4 * buf[4];
    // pointers to 4 bytes containing UTC seconds since unix epoch, msb
    uint32_t timestamp_sec, *timestamp_ptr;

    // convert buffer to uint32_t, octet order is big endian
    timestamp_ptr = (uint32_t *)buf;
    // swap byte order from msb to lsb, note: this is platform dependent
    timestamp_sec = __builtin_bswap32(*timestamp_ptr);

    // construct the timepoint when message was seen on gateway
    time_sync_rx[k] += seconds(timestamp_sec) + milliseconds(timestamp_msec);

    // we guess timepoint is recent if it newer than code compile date
    if (timeIsValid(myClock::to_time_t(time_sync_rx[k]))) {
      ESP_LOGD(TAG, "[%0.3f] Timesync request #%d of %d rcvd at %d.%03d",
               millis() / 1000.0, k + 1, TIME_SYNC_SAMPLES, timestamp_sec,
               timestamp_msec);

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