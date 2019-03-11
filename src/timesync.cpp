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
time_sync_message_t time_sync_messages[TIME_SYNC_SAMPLES] = {0},
                    time_sync_answers[TIME_SYNC_SAMPLES] = {0};
static uint8_t time_sync_seqNo = 0;
static bool time_sync_pending = false;

// send time request message
void send_Servertime_req() {

  // if a timesync handshake is pending then exit
  if (time_sync_pending) {
    ESP_LOGI(TAG, "Timeserver sync request already pending");
    return;
  } else {
    ESP_LOGI(TAG, "Timeserver sync request started");

    time_sync_pending = true;

    // clear timestamp array
    for (uint8_t i = 0; i <= TIME_SYNC_SAMPLES + 1; i++) {
      time_sync_messages[i].seconds = time_sync_answers[i].seconds =
          time_sync_messages[i].fractions = time_sync_answers[i].fractions = 0;
    }

    // kick off temporary task for timeserver handshake processing
    if (!timeSyncReqTask)
      xTaskCreatePinnedToCore(process_Servertime_sync_req, // task function
                              "timesync_req",              // name of task
                              2048,                        // stack size of task
                              (void *)1,                   // task parameter
                              0,                // priority of the task
                              &timeSyncReqTask, // task handle
                              1);               // CPU core
  }
}

// process timeserver timestamp response, called from rcommand.cpp
void recv_Servertime_ans(uint8_t buf[], uint8_t buf_len) {

  // if no timesync handshake is pending or invalid buffer then exit
  if ((!time_sync_pending) || (buf_len != TIME_SYNC_FRAME_LENGTH))
    return;

  uint8_t seq_no = buf[0], k = seq_no % TIME_SYNC_SAMPLES;
  uint32_t timestamp_sec = 0;

  for (uint8_t i = 1; i <= 4; i++) {
    time_sync_answers[k].seconds = (timestamp_sec <<= 8) |= buf[i];
  }
  time_sync_answers[k].fractions = buf[5];

  ESP_LOGD(TAG, "Timeserver answer:");

  ESP_LOGD(TAG, "ans.sec(%d)=%d / ans.ms(%d)=%d", k,
           time_sync_answers[k].seconds, k, time_sync_answers[k].fractions);

  // inform processing task
  if (timeSyncReqTask)
    xTaskNotify(timeSyncReqTask, seq_no, eSetBits);
}

// task for sending time sync requests
void process_Servertime_sync_req(void *taskparameter) {

  time_t t = 0, time_to_set = 0;
  uint32_t seq_no = 0, k = 0;
  int time_diff_frac = 0, time_diff_ms = 0;
  long time_diff_sec = 0, time_offset = 0;

  // enqueue timestamp samples in lora sendqueue
  for (uint8_t i = 1; i <= TIME_SYNC_SAMPLES; i++) {

    // wrap around seqNo 0 .. 254
    time_sync_seqNo = (time_sync_seqNo >= 255) ? 0 : time_sync_seqNo + 1;

    // send sync request to server
    payload.reset();
    payload.addByte(time_sync_seqNo);
    SendPayload(TIMEPORT, prio_high);

    /* -> do we really need this? maybe for SF9 up?
    // send dummy packet to trigger receive answer
    payload.reset();
    payload.addByte(0x99);           // flush
    SendPayload(RCMDPORT, prio_low); // to open receive slot for answer
    */

    // process answer
    if ((xTaskNotifyWait(0x00, ULONG_MAX, &seq_no,
                         TIME_SYNC_TIMEOUT * 1000 / portTICK_PERIOD_MS) ==
         pdFALSE) ||
        (seq_no != time_sync_seqNo)) {
      ESP_LOGW(TAG, "Timeserver handshake failed");
      goto finish;
    } // no valid sequence received before timeout

    else { // calculate time diff from set of collected timestamps

      k = seq_no % TIME_SYNC_SAMPLES;

      time_diff_sec +=
          ((time_sync_messages[k].seconds - time_sync_answers[k].seconds) / TIME_SYNC_SAMPLES);

      time_diff_frac +=
          ((time_sync_messages[k].fractions - time_sync_answers[k].fractions) / TIME_SYNC_SAMPLES);

      ESP_LOGD(TAG, "time_diff_sec=%d / time_diff_frac=%d", time_diff_sec,
               time_diff_frac);
    }
  } // for

  // calculate time offset and set time if necessary
  time_diff_ms = 4 * time_diff_frac;
  time_offset = (time_diff_sec + (long) (time_diff_ms / 1000));

  ESP_LOGD(TAG, "Timesync time offset=%d", time_offset);

  t = now();

  if (labs(time_offset) >= (t + TIME_SYNC_TRIGGER)) {
    // wait until top of second
    if (time_diff_ms > 0) // clock is fast
      vTaskDelay(pdMS_TO_TICKS(time_diff_ms));
    else if (time_diff_ms < 0) // clock is slow
      vTaskDelay(pdMS_TO_TICKS(1000 + time_diff_ms));

    time_diff_sec++;
    time_to_set = t - time_t(time_diff_sec);
    ESP_LOGD(TAG, "Now()=%d, Time to set = %d", t, time_to_set);

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

  time_sync_pending = false;
  timeSyncReqTask = NULL;
  vTaskDelete(NULL); // end task
}

// called from lorawan.cpp after time_sync_req was sent
void store_time_sync_req(time_t secs, uint32_t micros) {
  uint8_t k = time_sync_seqNo % TIME_SYNC_SAMPLES;

  time_sync_messages[k].seconds = (uint32_t) secs;
  time_sync_messages[k].fractions = (uint8_t) (micros / 4000); // 4ms resolution

  ESP_LOGD(TAG, "Timeserver request #%d sent at %d.%03d", time_sync_seqNo,
           time_sync_messages[k].seconds, time_sync_messages[k].fractions);
}

#endif