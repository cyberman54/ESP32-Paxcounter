/*

///--> IMPORTANT LICENSE NOTE for this file <--///

PLEASE NOTE: There is a patent filed for the time sync algorithm used in the
code of this file. The shown implementation example is covered by the
repository's licencse, but you may not be eligible to deploy the applied
algorithm in applications without granted license for the algorithm by the
patent holder.

*/

#ifdef TIME_SYNC_TIMESERVER

#include "timesync.h"

// Local logging tag
static const char TAG[] = __FILE__;

TaskHandle_t timeSyncReqTask, timeSyncAnsTask;
time_sync_message_t
    time_sync_messages[TIME_SYNC_SAMPLES + 1] = {0},
                                           time_sync_answers[TIME_SYNC_SAMPLES +
                                                             1] = {0};
uint8_t time_sync_seqNo = 0; // used in lorawan.cpp to store timestamp

// send time request message
void send_Servertime_req() {

  // if a timesync handshake is pending then exit
  if ((timeSyncAnsTask) || (timeSyncReqTask)) {
    ESP_LOGI(TAG, "Timesync sync request already running");
    return;
  } else {
    ESP_LOGI(TAG, "Timeserver sync request started");

    // clear timestamp array
    for (uint8_t i = 0; i <= TIME_SYNC_SAMPLES + 1; i++) {
      time_sync_messages[i].seconds = time_sync_answers[i].seconds = 0;
      time_sync_messages[i].fractions = time_sync_answers[i].fractions = 0;
    }

    time_sync_seqNo = 0;

    // create temporary task for processing sync answers if not already active
    if (!timeSyncAnsTask)
      xTaskCreatePinnedToCore(process_Servertime_sync_ans, // task function
                              "timesync_ans",              // name of task
                              2048,                        // stack size of task
                              (void *)1,                   // task parameter
                              0,                // priority of the task
                              &timeSyncAnsTask, // task handle
                              1);               // CPU core

    // create temporary task sending sync requests
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
void recv_Servertime_ans(uint8_t val[]) {

  // if no timesync handshake is pending then exit
  if (!time_sync_seqNo)
    return;

  uint8_t seq_no = val[0];
  uint32_t timestamp_sec = 0, timestamp_ms = 0;

  for (int i = 1; i <= 4; i++) {
    timestamp_sec = (timestamp_sec << 8) | val[i];
    time_sync_answers[seq_no].seconds = timestamp_sec;
  }
  time_sync_answers[seq_no].fractions = val[5];

  ESP_LOGD(TAG, "Timeserver timestamp #%d received: time=%d.%d", seq_no,
           timestamp_sec, timestamp_ms);

  // inform processing task
  if (timeSyncAnsTask)
    xTaskNotify(timeSyncAnsTask, seq_no, eSetBits);
}

// task for sending time sync requests
void process_Servertime_sync_req(void *taskparameter) {
  // enqueue timestamp samples in lora sendqueue
  for (uint8_t i = 1; i <= TIME_SYNC_SAMPLES; i++) {
    time_sync_seqNo++;
    payload.reset();
    payload.addWord(TIME_SYNC_REQ_OPCODE | time_sync_seqNo << 8);
    SendPayload(TIMEPORT, prio_high);
    ESP_LOGD(TAG, "Timeserver request #%d sent", time_sync_seqNo);
    // Wait for the next cycle
    vTaskDelay(pdMS_TO_TICKS(TIME_SYNC_CYCLE * 1000));
  }
  payload.reset();
  payload.addByte(TIME_SYNC_STOP_OPCODE);
  SendPayload(TIMEPORT, prio_high); // necessary to receive last timestamp
  timeSyncReqTask = NULL;
  vTaskDelete(NULL); // end task
}

// task for processing a timesync handshake
void process_Servertime_sync_ans(void *taskparameter) {

  uint32_t seq_no = 0, time_diff_sec = 0, time_diff_ms = 0;
  time_t time_to_set = 0;
  float time_offset = 0.0f;

  // collect incoming timestamp samples notified by rcommand
  for (uint8_t i = 1; i <= TIME_SYNC_SAMPLES; i++) {
    if ((xTaskNotifyWait(0x00, ULONG_MAX, &seq_no,
                         (TIME_SYNC_CYCLE + TIME_SYNC_TIMEOUT) * 1000 /
                             portTICK_PERIOD_MS) == pdFALSE) ||
        (seq_no != i)) {
      ESP_LOGW(TAG, "Timesync handshake timeout");
      goto finish; // no valid sequence received before timeout
    } else         // calculate time diff from set of collected timestamps
    {
      time_diff_sec += time_sync_messages[seq_no].seconds -
                       time_sync_answers[seq_no].seconds;
      time_diff_ms += 4 * (time_sync_messages[seq_no].fractions -
                           time_sync_answers[seq_no].fractions);
    }
  } // for

  time_offset = (time_diff_sec + time_diff_ms / 1000.0) / TIME_SYNC_SAMPLES;

  ESP_LOGD(TAG, "Timesync finished, time offset=%.4f seconds", time_offset);

  // check time diff and if necessary set time
  if (time_offset >= TIME_SYNC_THRESHOLD) {

    // wait until top of second
    if (time_diff_ms > 0) {
      vTaskDelay(1000 - time_diff_ms); // clock is fast
      time_diff_sec--;
    } else if (time_diff_ms < 0) { // clock is stale
      vTaskDelay(1000 + time_diff_ms);
      time_diff_sec++;
    }

    time_to_set = time_t(now() - time_diff_sec);

    ESP_LOGD(TAG, "Time to set = %d", time_to_set);

    // Update system time with time read from the network
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

  time_sync_seqNo = 0;
  timeSyncAnsTask = NULL;
  vTaskDelete(NULL); // end task
}

// called from lorawan.cpp when tine_sync_req was sent
void store_time_sync_req(time_t secs, uint32_t micros) {

  time_sync_messages[time_sync_seqNo].seconds = secs;
  time_sync_messages[time_sync_seqNo].fractions =
      micros / 250; // 4ms resolution

  ESP_LOGD(TAG, "Timeserver request #%d was sent at %d.%d", time_sync_seqNo,
           time_sync_messages[time_sync_seqNo].seconds,
           time_sync_messages[time_sync_seqNo].fractions);
}

void force_Servertime_sync(uint8_t val[]) {
  ESP_LOGI(TAG, "Timesync requested by timeserver");
  timeSync();
};

#endif