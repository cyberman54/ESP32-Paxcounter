/*

///-->  IMPORTANT LICENSE NOTE for this file <--///

PLEASE NOTE: There is a patent filed for the time sync algorithm used in the
followin code in this file. This shown implementation example is covered by the
repository's licencse, but you may not be eligible to deploy the applied
algorithm in applications without granted license for the algorithm by the
patent holder.

*/

#ifdef TIME_SYNC_TIMESERVER

#include "timesync.h"

// Local logging tag
static const char TAG[] = __FILE__;

TaskHandle_t timeSyncReqTask, timeSyncAnsTask;
uint32_t time_sync_messages[TIME_SYNC_SAMPLES +
                            1] = {0},
                            time_sync_answers[TIME_SYNC_SAMPLES + 1] = {0};
uint8_t volatile time_sync_seqNo = 0; // used in lorawan.cpp to store timestamp

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
      time_sync_messages[i] = time_sync_answers[i] = 0;
    }

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
  uint32_t timestamp = 0;

  for (int i = 1; i <= 4; i++)
    timestamp = (timestamp << 8) | val[i];
  time_sync_answers[seq_no] = timestamp;

  ESP_LOGD(TAG, "Timeserver timestamp #%d received: time=%d", seq_no,
           timestamp);

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
    payload.add2Bytes(TIME_SYNC_REQ_OPCODE, i);
    SendPayload(TIMEPORT, prio_high);
    ESP_LOGD(TAG, "Timeserver request #%d sent", i);
    // Wait for the next cycle
    vTaskDelay(pdMS_TO_TICKS(TIME_SYNC_CYCLE * 1000));
  }
  timeSyncReqTask = NULL;
  vTaskDelete(NULL); // end task
}

// task for processing a timesync handshake
void process_Servertime_sync_ans(void *taskparameter) {
  uint32_t seq_no = 0;
  uint32_t NetworkTime = 0;
  int32_t time_diff = 0;
  uint8_t ans_counter = TIME_SYNC_SAMPLES;

  // collect incoming timestamp samples notified by rcommand
  for (uint8_t i = 1; i <= TIME_SYNC_SAMPLES; i++) {
    if (xTaskNotifyWait(0x00, ULONG_MAX, &seq_no,
                        (TIME_SYNC_CYCLE + TIME_SYNC_TIMEOUT) * 1000 /
                            portTICK_PERIOD_MS) == pdFALSE)
      continue; // no answer received before timeout

    time_diff += time_sync_messages[seq_no] - time_sync_answers[seq_no];
    ans_counter--;
  }

  if (ans_counter) {
    ESP_LOGW(TAG, "Timesync handshake timeout");
  } else {
    // calculate time diff from set of collected timestamps
    if (time_diff / TIME_SYNC_SAMPLES) {
      NetworkTime = now() + time_diff;
      ESP_LOGI(TAG, "Timesync finished, time offset=%d seconds",
               time_diff);
      // Update system time with time read from the network
      if (timeIsValid(NetworkTime)) {
        setTime(NetworkTime);
        timeSource = _lora;
        timesyncer.attach(TIME_SYNC_INTERVAL * 60,
                          timeSync); // set to regular repeat
        ESP_LOGI(TAG, "Recent time received from timeserver");
      } else
        ESP_LOGW(TAG, "Invalid time received from timeserver");
    } else
      ESP_LOGI(TAG, "Timesync finished, time is up to date");
  } // if (ans_counter)

  time_sync_seqNo = 0;
  timeSyncAnsTask = NULL;
  vTaskDelete(NULL); // end task
}

void force_Servertime_sync(uint8_t val[]) {
  ESP_LOGI(TAG, "Timesync requested by timeserver");
  timeSync();
};

#endif