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
typedef std::chrono::duration<double> myClock_secTick;

myClock_timepoint time_sync_tx[TIME_SYNC_SAMPLES];
myClock_timepoint time_sync_rx[TIME_SYNC_SAMPLES];

// send time request message
void send_timesync_req() {

  // if a timesync handshake is pending then exit
  if (lora_time_sync_pending) {
    ESP_LOGI(TAG, "Timeserver sync request already pending");
    return;
  } else {
    ESP_LOGI(TAG, "[%0.3f] Timeserver sync request started", millis() / 1000.0);

    lora_time_sync_pending = true;

    // initialize timestamp array
    for (uint8_t i = 0; i < TIME_SYNC_SAMPLES; i++)
      time_sync_tx[i] = time_sync_rx[i] = myClock_timepoint();

    // kick off temporary task for timeserver handshake processing
    if (!timeSyncReqTask)
      xTaskCreatePinnedToCore(process_timesync_req, // task function
                              "timesync_req",       // name of task
                              2048,                 // stack size of task
                              (void *)1,            // task parameter
                              4,                    // priority of the task
                              &timeSyncReqTask,     // task handle
                              1);                   // CPU core
  }
}

// task for sending time sync requests
void process_timesync_req(void *taskparameter) {

  uint32_t seq_no = 0, time_to_set_us;
  long long int time_to_set_ms;
  uint16_t time_to_set_fraction_msec;
  uint8_t k = 0, i = 0;
  time_t time_to_set;
  auto time_offset = myClock_msecTick::zero();

  // wait until we are joined
  while (!LMIC.devaddr) {
    vTaskDelay(pdMS_TO_TICKS(2000));
  }

  // enqueue timestamp samples in lora sendqueue
  for (uint8_t i = 0; i < TIME_SYNC_SAMPLES; i++) {

    // wrap around seqNo 0 .. 254
    time_sync_seqNo = (time_sync_seqNo >= 255) ? 0 : time_sync_seqNo + 1;

    // send sync request to server
    payload.reset();
    payload.addByte(time_sync_seqNo);
    SendPayload(TIMEPORT, prio_high);

    // process answer, wait for notification from recv_timesync_ans()
    if ((xTaskNotifyWait(0x00, ULONG_MAX, &seq_no,
                         pdMS_TO_TICKS(TIME_SYNC_TIMEOUT * 1000)) == pdFALSE) ||
        (seq_no != time_sync_seqNo)) {

      ESP_LOGW(TAG, "[%0.3f] Timeserver handshake failed", millis() / 1000.0);
      goto finish;
    } // no valid sequence received before timeout

    else { // calculate time diff from collected timestamps
      k = seq_no % TIME_SYNC_SAMPLES;

      auto t_tx = time_point_cast<milliseconds>(
          time_sync_tx[k]); // timepoint when node TX_completed
      auto t_rx = time_point_cast<milliseconds>(
          time_sync_rx[k]); // timepoint when message was seen on gateway

      time_offset += t_rx - t_tx; // cumulate timepoint diffs

      if (i < TIME_SYNC_SAMPLES - 1) {
        // wait until next cycle
        vTaskDelay(pdMS_TO_TICKS(TIME_SYNC_CYCLE * 1000));
      } else {
        // send flush to open a receive window for last time_sync_ans
        payload.reset();
        payload.addByte(0x99);
        SendPayload(RCMDPORT, prio_high);
      }
    }
  } // for

  // calculate time offset from collected diffs
  time_offset /= TIME_SYNC_SAMPLES;
  ESP_LOGD(TAG, "[%0.3f] avg time diff: %0.3f sec", millis() / 1000.0,
           myClock_secTick(time_offset).count());

  // calculate absolute time with millisecond precision
  time_to_set_ms = (long long)now(time_to_set_us) * 1000LL +
                   time_to_set_us / 1000LL + time_offset.count();
  // convert to seconds
  time_to_set = (time_t)(time_to_set_ms / 1000LL);
  // calculate fraction milliseconds
  time_to_set_fraction_msec = (uint16_t)(time_to_set_ms % 1000LL);

  ESP_LOGD(TAG, "[%0.3f] Calculated UTC epoch time: %d.%03d sec",
           millis() / 1000.0, time_to_set, time_to_set_fraction_msec);

  // adjust system time
  if (timeIsValid(time_to_set)) {

    if (abs(time_offset.count()) >=
        TIME_SYNC_TRIGGER) { // milliseconds threshold

      // wait until top of second
      ESP_LOGD(TAG, "[%0.3f] waiting %d ms", millis() / 1000.0,
               1000 - time_to_set_fraction_msec);
      vTaskDelay(pdMS_TO_TICKS(1000 - time_to_set_fraction_msec));

      // sync timer pps to top of second
      if (ppsIRQ) {
        timerRestart(ppsIRQ); // reset pps timer
        CLOCKIRQ();           // fire clock pps interrupt
      }

      setTime(time_to_set + 1);
      timeSource = _lora;

      timesyncer.attach(TIME_SYNC_INTERVAL * 60,
                        timeSync); // set to regular repeat
      ESP_LOGI(TAG, "[%0.3f] Timesync finished, time adjusted by %.3f sec",
               millis() / 1000.0, myClock_secTick(time_offset).count());
    } else
      ESP_LOGI(TAG, "Timesync finished, time not adjusted, is up to date");
  } else
    ESP_LOGW(TAG, "Invalid time received from timeserver");

finish:

  lora_time_sync_pending = false;
  timeSyncReqTask = NULL;
  vTaskDelete(NULL); // end task
}

// called from lorawan.cpp after time_sync_req was sent
void store_time_sync_req(uint32_t t_millisec) {

  uint8_t k = time_sync_seqNo % TIME_SYNC_SAMPLES;
  time_sync_tx[k] += milliseconds(t_millisec);

  ESP_LOGD(TAG, "[%0.3f] Timesync request #%d sent at %d.%03d",
           millis() / 1000.0, time_sync_seqNo, t_millisec / 1000,
           t_millisec % 1000);
}

// process timeserver timestamp answer, called from lorawan.cpp
int recv_timesync_ans(uint8_t buf[], uint8_t buf_len) {

  // if no timesync handshake is pending or spurious buffer then exit
  if ((!lora_time_sync_pending) || (buf_len != TIME_SYNC_FRAME_LENGTH))
    return 0; // failure

  uint8_t seq_no = buf[0], k = seq_no % TIME_SYNC_SAMPLES;
  uint16_t timestamp_msec = 4 * buf[5]; // convert 1/250th sec fractions to ms
  uint32_t timestamp_sec = 0, tmp_sec = 0;

  for (uint8_t i = 1; i <= 4; i++) {
    timestamp_sec = (tmp_sec <<= 8) |= buf[i];
  }

  if (timestamp_sec + timestamp_msec) // field validation: timestamp not 0 ?
    time_sync_rx[k] += seconds(timestamp_sec) + milliseconds(timestamp_msec);
  else
    return 0; // failure

  ESP_LOGD(TAG, "[%0.3f] Timesync request #%d rcvd at %d.%03d",
           millis() / 1000.0, seq_no, timestamp_sec, timestamp_msec);

  // inform processing task
  if (timeSyncReqTask)
    xTaskNotify(timeSyncReqTask, seq_no, eSetBits);

  return 1; // success
}

#endif