/*

///--> IMPORTANT LICENSE NOTE for this file <--///

PLEASE NOTE: There is a patent filed for the time sync algorithm used in the
code of this file. The shown implementation example is covered by the
repository's licencse, but you may not be eligible to deploy the applied
algorithm in applications without granted license by the patent holder.

*/

#ifdef TIME_SYNC_TIMESERVER

#include "timesync.h"

using namespace std::chrono;

// Local logging tag
static const char TAG[] = __FILE__;

TaskHandle_t timeSyncReqTask;

static uint8_t time_sync_seqNo = 0;
static bool lora_time_sync_pending = false;

typedef std::chrono::system_clock myClock;
typedef myClock::time_point myClock_timepoint;
typedef std::chrono::duration<long long int, std::ratio<1, 1000>>
    myClock_msecTick;

myClock_timepoint time_sync_tx[TIME_SYNC_SAMPLES];
myClock_timepoint time_sync_rx[TIME_SYNC_SAMPLES];

// send time request message
void send_timesync_req() {

  // if a timesync handshake is pending then exit
  if (lora_time_sync_pending) {
    // ESP_LOGI(TAG, "Timeserver sync request already pending");
    return;
  } else {
    ESP_LOGI(TAG, "[%0.3f] Timeserver sync request started", millis() / 1000.0);

    lora_time_sync_pending = true;

    // clear timestamp array
    for (uint8_t i = 0; i < TIME_SYNC_SAMPLES; i++)
      time_sync_tx[i] = time_sync_rx[i] = myClock_timepoint();

    // kick off temporary task for timeserver handshake processing
    if (!timeSyncReqTask)
      xTaskCreatePinnedToCore(process_timesync_req, // task function
                              "timesync_req",       // name of task
                              2048,                 // stack size of task
                              (void *)1,            // task parameter
                              2,                    // priority of the task
                              &timeSyncReqTask,     // task handle
                              1);                   // CPU core
  }
}

// task for sending time sync requests
void process_timesync_req(void *taskparameter) {

  uint8_t k = 0;
  uint32_t seq_no = 0, time_to_set;
  auto time_offset_ms = myClock_msecTick::zero();
  uint16_t time_to_set_fraction_msec;

  // wait until we are joined
  while (!LMIC.devaddr) {
    vTaskDelay(pdMS_TO_TICKS(2000));
  }

  // enqueue timestamp samples in lora sendqueue
  for (uint8_t i = 0; i < TIME_SYNC_SAMPLES; i++) {

    // wrap around seqNo 0 .. 254
    time_sync_seqNo = (time_sync_seqNo < 255) ? time_sync_seqNo + 1 : 0;

    // send sync request to server
    payload.reset();
    payload.addByte(time_sync_seqNo);
    SendPayload(TIMEPORT, prio_high);

    // process answer, wait for notification from recv_timesync_ans()
    if ((xTaskNotifyWait(0x00, ULONG_MAX, &seq_no,
                         pdMS_TO_TICKS(TIME_SYNC_TIMEOUT * 1000)) == pdFALSE) ||
        (seq_no != time_sync_seqNo))
      goto error; // no valid sequence received before timeout

    else { // calculate time diff from collected timestamps
      k = seq_no % TIME_SYNC_SAMPLES;

      // cumulate timepoint diffs
      time_offset_ms += time_point_cast<milliseconds>(time_sync_rx[k]) -
                        time_point_cast<milliseconds>(time_sync_tx[k]);

      if (i < TIME_SYNC_SAMPLES - 1) {
        // wait until next cycle
        vTaskDelay(pdMS_TO_TICKS(TIME_SYNC_CYCLE * 1000));
      } else { // before sending last time sample...
        // ...send flush to open a receive window for last time_sync_answer
        // payload.reset();
        // payload.addByte(0x99);
        // SendPayload(RCMDPORT, prio_high);
        // ...send a alive open a receive window for last time_sync_answer
        LMIC_sendAlive();
      }
    }
  } // for

  // begin of time critical section: lock I2C bus to ensure accurate timing
  // don't move the mutex, will impact accuracy of time up to 1 sec!
  if (!I2C_MUTEX_LOCK())
    goto error; // failure

  // average time offset from collected diffs
  time_offset_ms /= TIME_SYNC_SAMPLES;

  // calculate time offset with millisecond precision using LMIC's time base,
  // since we use LMIC's ostime_t txEnd as tx timestamp.
  // Finally apply calibration const for processing time.
  time_offset_ms +=
      milliseconds(osticks2ms(os_getTime())) + milliseconds(TIME_SYNC_FIXUP);

  // calculate absolute time in UTC epoch: convert to whole seconds, round to
  // ceil, and calculate fraction milliseconds
  time_to_set = (uint32_t)(time_offset_ms.count() / 1000) + 1;
  // calculate fraction milliseconds
  time_to_set_fraction_msec = (uint16_t)(time_offset_ms.count() % 1000);

  adjustTime(time_to_set, time_to_set_fraction_msec);

  // end of time critical section: release I2C bus
  I2C_MUTEX_UNLOCK();

finish:
  lora_time_sync_pending = false;
  timeSyncReqTask = NULL;
  vTaskDelete(NULL); // end task

error:
  ESP_LOGW(TAG, "[%0.3f] Timeserver error: handshake timed out",
           millis() / 1000.0);
  goto finish;
}

// called from lorawan.cpp after time_sync_req was sent
void store_time_sync_req(uint32_t timestamp) {

  uint8_t k = time_sync_seqNo % TIME_SYNC_SAMPLES;

  time_sync_tx[k] += milliseconds(timestamp);

  ESP_LOGD(TAG, "[%0.3f] Timesync request #%d sent at %d.%03d",
           millis() / 1000.0, time_sync_seqNo, timestamp / 1000,
           timestamp % 1000);
}

// process timeserver timestamp answer, called from lorawan.cpp
int recv_timesync_ans(uint8_t buf[], uint8_t buf_len) {

  // if no timesync handshake is pending or spurious buffer then exit
  if (!lora_time_sync_pending)
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

    uint8_t seq_no = buf[0], k = seq_no % TIME_SYNC_SAMPLES;
    uint16_t timestamp_msec; // convert 1/250th sec fractions to ms
    uint32_t timestamp_sec;

    // fetch timeserver time from 4 bytes containing the UTC seconds since
    // unix epoch. Octet order is big endian. Casts are necessary, because buf
    // is an array of single byte values, and they might overflow when shifted
    timestamp_sec = ((uint32_t)buf[4]) | (((uint32_t)buf[3]) << 8) |
                    (((uint32_t)buf[2]) << 16) | (((uint32_t)buf[1]) << 24);

    // the 5th byte contains the fractional seconds in 2^-8 second steps
    timestamp_msec = 4 * buf[5];

    // construct the timepoint when message was seen on gateway
    time_sync_rx[k] += seconds(timestamp_sec) + milliseconds(timestamp_msec);

    // guess timepoint is recent if newer than code compile date
    if (timeIsValid(myClock::to_time_t(time_sync_rx[k]))) {
      ESP_LOGD(TAG, "[%0.3f] Timesync request #%d rcvd at %d.%03d",
               millis() / 1000.0, seq_no, timestamp_sec, timestamp_msec);

      // inform processing task
      if (timeSyncReqTask)
        xTaskNotify(timeSyncReqTask, seq_no, eSetBits);

      return 1; // success
    } else {
      ESP_LOGW(TAG, "[%0.3f] Timeserver error: outdated time received",
               millis() / 1000.0);
      return 0; // failure
    }
  }
}

// adjust system time, calibrate RTC and RTC_INT pps
int adjustTime(uint32_t t_sec, uint16_t t_msec) {

  time_t time_to_set = (time_t)t_sec;

  ESP_LOGD(TAG, "[%0.3f] Calculated UTC epoch time: %d.%03d sec",
           millis() / 1000.0, time_to_set, t_msec);

  if (timeIsValid(time_to_set)) {

    // wait until top of second with millisecond precision
    vTaskDelay(pdMS_TO_TICKS(1000 - t_msec));

#ifdef HAS_RTC
    time_to_set++; // advance time 1 sec wait time
    // set RTC time and calibrate RTC_INT pulse on top of second
    set_rtctime(time_to_set, no_mutex);
#endif

#if (!defined GPS_INT && !defined RTC_INT)
    // sync pps timer to top of second
    timerRestart(ppsIRQ); // reset pps timer
    CLOCKIRQ();           // fire clock pps, advances time 1 sec
#endif

    setTime(time_to_set); // set the time on top of second

    timeSource = _lora;
    timesyncer.attach(TIME_SYNC_INTERVAL * 60, timeSync); // regular repeat
    ESP_LOGI(TAG, "[%0.3f] Timesync finished, time was adjusted",
             millis() / 1000.0);
  } else
    ESP_LOGW(TAG, "[%0.3f] Timesync failed, outdated time calculated",
             millis() / 1000.0);
}

#endif