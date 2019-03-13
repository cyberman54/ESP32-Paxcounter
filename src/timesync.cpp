/*

///--> IMPORTANT LICENSE NOTE for this file <--///

PLEASE NOTE: There is a patent filed for the time sync algorithm used in the
code of this file. The shown implementation example is covered by the
repository's licencse, but you may not be eligible to deploy the applied
algorithm in applications without granted license by the patent holder.

*/

#ifdef TIME_SYNC_TIMESERVER

#include "timesync.h"

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
    ESP_LOGI(TAG, "Timeserver sync request already pending");
    return;
  } else {
    ESP_LOGI(TAG, "Timeserver sync request started");

    lora_time_sync_pending = true;

    // clear timestamp array
    for (uint8_t i = 0; i < TIME_SYNC_SAMPLES; i++) {
      time_sync_tx[i] = time_sync_rx[i] = myClock_timepoint(); // set to epoch
    }

    // kick off temporary task for timeserver handshake processing
    if (!timeSyncReqTask)
      xTaskCreatePinnedToCore(process_timesync_req, // task function
                              "timesync_req",       // name of task
                              2048,                 // stack size of task
                              (void *)1,            // task parameter
                              0,                    // priority of the task
                              &timeSyncReqTask,     // task handle
                              1);                   // CPU core
  }
}

// process timeserver timestamp answer, called from lorawan.cpp
void recv_timesync_ans(uint8_t buf[], uint8_t buf_len) {

  // if no timesync handshake is pending or spurious buffer then exit
  if ((!lora_time_sync_pending) || (buf_len != TIME_SYNC_FRAME_LENGTH))
    return;

  uint8_t seq_no = buf[0], k = seq_no % TIME_SYNC_SAMPLES;
  uint16_t timestamp_msec = 4 * buf[5]; // convert 1/250th sec fractions to ms
  uint32_t timestamp_sec = 0, tmp_sec = 0;

  for (uint8_t i = 1; i <= 4; i++) {
    timestamp_sec = (tmp_sec <<= 8) |= buf[i];
  }

  time_sync_rx[k] += std::chrono::seconds(timestamp_sec) +
                     std::chrono::milliseconds(timestamp_msec);

  ESP_LOGD(TAG, "Timesync request #%d rcvd at %d", seq_no,
           myClock::to_time_t(time_sync_rx[k]));

  // inform processing task
  if (timeSyncReqTask)
    xTaskNotify(timeSyncReqTask, seq_no, eSetBits);
}

// task for sending time sync requests
void process_timesync_req(void *taskparameter) {

  time_t time_to_set = 0;
  uint8_t k = 0, i = 0;
  uint32_t seq_no = 0;
  auto time_offset = myClock_msecTick::zero();

  // enqueue timestamp samples in lora sendqueue
  for (uint8_t i = 0; i < TIME_SYNC_SAMPLES; i++) {

    // wrap around seqNo 0 .. 254
    time_sync_seqNo = (time_sync_seqNo >= 255) ? 0 : time_sync_seqNo + 1;

    // send sync request to server
    payload.reset();
    payload.addByte(time_sync_seqNo);
    SendPayload(TIMEPORT, prio_high);

    // process answer
    if ((xTaskNotifyWait(0x00, ULONG_MAX, &seq_no,
                         pdMS_TO_TICKS(TIME_SYNC_TIMEOUT * 1000)) == pdFALSE) ||
        (seq_no != time_sync_seqNo)) {

      ESP_LOGW(TAG, "Timeserver handshake failed");
      goto finish;
    } // no valid sequence received before timeout

    else { // calculate time diff from collected timestamps
      k = seq_no % TIME_SYNC_SAMPLES;

      auto t_tx = std::chrono::time_point_cast<std::chrono::milliseconds>(
          time_sync_tx[k]); // timepoint when node TX_completed
      auto t_rx = std::chrono::time_point_cast<std::chrono::milliseconds>(
          time_sync_rx[k]); // timepoint when message was seen on gateway

      time_offset += t_rx - t_tx; // cumulate timepoint diffs

      if (i < TIME_SYNC_SAMPLES - 1) // wait until next cycle
        vTaskDelay(pdMS_TO_TICKS(TIME_SYNC_CYCLE * 1000));
    }
  } // for

  // calculate time offset from collected diffs and set time if necessary
  time_offset /= TIME_SYNC_SAMPLES;
  ESP_LOGD(TAG, "Avg time diff: %lldms", time_offset.count());

  if (abs(time_offset.count()) >= TIME_SYNC_TRIGGER) {

    /*
    // wait until top of second
    if (time_offset_ms > 0) // clock is fast
      vTaskDelay(pdMS_TO_TICKS(time_diff_ms));
    else if (time_offset_ms < 0) // clock is slow
      vTaskDelay(pdMS_TO_TICKS(1000 + time_offset_ms));

    time_to_set = t - time_t(time_offset_sec + 1);
    */

    time_t time_to_set = myClock::to_time_t(myClock::now() + time_offset);
    ESP_LOGD(TAG, "New UTC epoch time: %d", time_to_set);

    // adjust system time
    if (timeIsValid(time_to_set)) {
      setTime(time_to_set);
      SyncToPPS();
      timeSource = _lora;
      timesyncer.attach(TIME_SYNC_INTERVAL * 60,
                        timeSync); // set to regular repeat
      ESP_LOGI(TAG, "Timesync finished, time was adjusted");
    } else
      ESP_LOGW(TAG, "Invalid time received from timeserver");
  } else
    ESP_LOGI(TAG, "Timesync finished, time is up to date");

finish:

  lora_time_sync_pending = false;
  timeSyncReqTask = NULL;
  vTaskDelete(NULL); // end task
}

// called from lorawan.cpp after time_sync_req was sent
void store_time_sync_req(time_t t_sec, uint32_t t_microsec) {

  uint8_t k = time_sync_seqNo % TIME_SYNC_SAMPLES;

  time_sync_tx[k] +=
      std::chrono::seconds(t_sec) + std::chrono::microseconds(t_microsec);

  ESP_LOGD(TAG, "Timesync request #%d sent at %d", time_sync_seqNo,
           myClock::to_time_t(time_sync_tx[k]));
}

#endif