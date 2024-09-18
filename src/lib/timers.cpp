#include "timers.hpp"

esp_timer_handle_t screenRotateTimer;
esp_timer_handle_t minuteTimer;

void setupTimeUpdateTimer(void *pvParameters) {
  const esp_timer_create_args_t minuteTimerConfig = {
      .callback = &minuteTimerISR, .name = "minute_timer"};

  esp_timer_create(&minuteTimerConfig, &minuteTimer);

  time_t currentTime;
  struct tm timeinfo;
  time(&currentTime);
  localtime_r(&currentTime, &timeinfo);
  uint32_t secondsUntilNextMinute = 60 - timeinfo.tm_sec;

  if (secondsUntilNextMinute > 0)
    vTaskDelay(pdMS_TO_TICKS((secondsUntilNextMinute * 1000)));

  esp_timer_start_periodic(minuteTimer, usPerMinute);

  WorkItem timeUpdate = {TASK_TIME_UPDATE, 0};
  xQueueSend(workQueue, &timeUpdate, portMAX_DELAY);
  //    xTaskNotifyGive(timeUpdateTaskHandle);

  vTaskDelete(NULL);
}

void setupScreenRotateTimer(void *pvParameters) {
  const esp_timer_create_args_t screenRotateTimerConfig = {
      .callback = &screenRotateTimerISR, .name = "screen_rotate_timer"};

  esp_timer_create(&screenRotateTimerConfig, &screenRotateTimer);

  if (preferences.getBool("timerActive", DEFAULT_TIMER_ACTIVE)) {
    esp_timer_start_periodic(screenRotateTimer,
                             getTimerSeconds() * usPerSecond);
  }

  vTaskDelete(NULL);
}

uint getTimerSeconds() { return preferences.getUInt("timerSeconds", DEFAULT_TIMER_SECONDS); }

bool isTimerActive() { return esp_timer_is_active(screenRotateTimer); }

void setTimerActive(bool status) {
  if (status) {
    esp_timer_start_periodic(screenRotateTimer,
                             getTimerSeconds() * usPerSecond);
    queueLedEffect(LED_EFFECT_START_TIMER);
    preferences.putBool("timerActive", true);
  } else {
    esp_timer_stop(screenRotateTimer);
    queueLedEffect(LED_EFFECT_PAUSE_TIMER);
    preferences.putBool("timerActive", false);
  }

  if (eventSourceTaskHandle != NULL) xTaskNotifyGive(eventSourceTaskHandle);
}

void toggleTimerActive() { setTimerActive(!isTimerActive()); }

void IRAM_ATTR minuteTimerISR(void *arg) {
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  //    vTaskNotifyGiveFromISR(timeUpdateTaskHandle, &xHigherPriorityTaskWoken);
  WorkItem timeUpdate = {TASK_TIME_UPDATE, 0};
  xQueueSendFromISR(workQueue, &timeUpdate, &xHigherPriorityTaskWoken);

  if (bitaxeFetchTaskHandle != NULL) {
    vTaskNotifyGiveFromISR(bitaxeFetchTaskHandle, &xHigherPriorityTaskWoken);
  }

  if (xHigherPriorityTaskWoken == pdTRUE) {
    portYIELD_FROM_ISR();
  }
}

void IRAM_ATTR screenRotateTimerISR(void *arg) {
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  vTaskNotifyGiveFromISR(taskScreenRotateTaskHandle, &xHigherPriorityTaskWoken);
  if (xHigherPriorityTaskWoken == pdTRUE) {
    portYIELD_FROM_ISR();
  }
}