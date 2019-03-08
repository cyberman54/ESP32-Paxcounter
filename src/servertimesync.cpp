/*
/////////////////////// LICENSE NOTE for servertimesync.cpp /////////////////////////////////
PLEASE NOTE: There is a patent filed for the time sync algorithm used in the following code.
The shown implementation example is covered by the repository's licencse, but you may not be 
eligible to deploy the algorith in applications without granted license by the patent holder.
/////////////////////////////////////////////////////////////////////////////////////////////
*/


#ifdef ServertimeSYNC

#include "Servertimesync.h"

// Local logging tag
static const char TAG[] = __FILE__;

TaskHandle_t timeSyncReqTask, timeSyncAnsTask;

uint32_t time_sync_messages[SYNC_SAMPLES] = {0},
         time_sync_answers[SYNC_SAMPLES] = {0};

uint8_t time_sync_seqNo = 0;

// send time request message
void send_Servertime_req() {

  // if a running timesync handshake is pending then exit
  if (time_sync_seqNo)
    return;

  // initalize sample arrays
  for (uint8_t i = 0; i < SYNC_SAMPLES; i++)
    time_sync_messages[i] = time_sync_answers[i] = 0;

  // create temporary task sending sync requests
  if (timeSyncReqTask != NULL)
    xTaskCreatePinnedToCore(process_Servertime_sync_req, // task function
                            "timesync_req",          // name of task
                            2048,                    // stack size of task
                            (void *)1,               // task parameter
                            0,                       // priority of the task
                            &timeSyncReqTask,        // task handle
                            1);                      // CPU core

  // create temporary task for processing sync answers if not already active
  if (timeSyncAnsTask != NULL)
    xTaskCreatePinnedToCore(process_Servertime_sync_ans, // task function
                            "timesync_ans",          // name of task
                            2048,                    // stack size of task
                            (void *)1,               // task parameter
                            0,                       // priority of the task
                            &timeSyncAnsTask,        // task handle
                            1);                      // CPU core
}

// handle time sync response, called from rcommanc.cpp
void recv_Servertime_ans(uint8_t val[]) {

  uint8_t seq_no = val[0];
  uint32_t timestamp = 0;

  time_sync_seqNo--;

  for (int i = 1; i <= 4; ++i)
    timestamp = (timestamp << 8) | val[i];
  time_sync_answers[seq_no - 1] = timestamp;

  ESP_LOGI(TAG, "Timeserver timestamp received, sequence #%d: %d", seq_no,
           timestamp);

  // inform processing task
  if (timeSyncAnsTask)
    xTaskNotify(timeSyncAnsTask, seq_no, eSetBits);
}

void force_Servertime_sync(uint8_t val[]) {
  ESP_LOGI(TAG, "Timesync forced by timeserver");
  timeSync();
};

// task for sending time sync requests
void process_Servertime_sync_req(void *taskparameter) {

  TickType_t startTime = xTaskGetTickCount();

  // enqueue timestamp samples in lora sendqueue
  for (uint8_t i = 0; i < SYNC_SAMPLES; i++) {
    payload.reset();
    payload.addAlarm(TIME_REQ_OPCODE, time_sync_seqNo);
    SendPayload(TIMEPORT, prio_high);
    time_sync_seqNo++;
    // Wait for the next cycle
    vTaskDelayUntil(&startTime, pdMS_TO_TICKS(SYNC_CYCLE * 1000));
  }
  vTaskDelete(NULL); // end task
}

// task for processing a timesync handshake
void process_Servertime_sync_ans(void *taskparameter) {

  uint32_t seq_no = 0;
  uint32_t NetworkTime = 0;
  int32_t time_diff = 0;

  // collect incoming timestamp samples notified by rcommand
  for (uint8_t i = 0; i < SYNC_SAMPLES; i++) {
    if (xTaskNotifyWait(0x00, ULONG_MAX, &seq_no,
                        SYNC_TIMEOUT * 1000 / portTICK_PERIOD_MS) == pdTRUE)
      time_sync_seqNo--;
  }

  if (time_sync_seqNo) {
    ESP_LOGW(TAG, "Timesync handshake failed");
    time_sync_seqNo = 0;
  }

  else {

    // calculate time diff from set of collected timestamps
    for (uint8_t i = 0; i < SYNC_SAMPLES; i++)
      time_diff += time_sync_messages[i] - time_sync_answers[i];

    if ((time_diff / SYNC_SAMPLES * 1.0f) > SYNC_THRESHOLD) {
      NetworkTime = now() + time_diff;
      ESP_LOGD(TAG, "Timesync handshake completed, time offset = %d",
               time_diff);
    } else
      ESP_LOGD(TAG, "Timesync handshake completed, time is up to date");

    // Update system time with time read from the network
    if (timeIsValid(NetworkTime)) {
      setTime(NetworkTime);
      timeSource = _lora;
      timesyncer.attach(TIME_SYNC_INTERVAL * 60, timeSync); // regular repeat
      ESP_LOGI(TAG, "Recent time received from timeserver");
    } else
      ESP_LOGW(TAG, "Invalid time received from timeserver");
  }
  vTaskDelete(NULL); // end task
}

#endif