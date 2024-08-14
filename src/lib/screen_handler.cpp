#include "screen_handler.hpp"

// TaskHandle_t priceUpdateTaskHandle;
// TaskHandle_t blockUpdateTaskHandle;
// TaskHandle_t timeUpdateTaskHandle;
TaskHandle_t taskScreenRotateTaskHandle;
TaskHandle_t workerTaskHandle;
esp_timer_handle_t screenRotateTimer;
esp_timer_handle_t minuteTimer;

std::array<std::string, NUM_SCREENS> taskEpdContent = {"", "", "", "",
                                                       "", "", ""};
std::string priceString;

#define WORK_QUEUE_SIZE 10
QueueHandle_t workQueue = NULL;

uint currentScreen;

void workerTask(void *pvParameters) {
  WorkItem receivedItem;

  while (1) {
    // Wait for a work item to be available in the queue
    if (xQueueReceive(workQueue, &receivedItem, portMAX_DELAY)) {
      uint firstIndex = 0;

      // Process the work item based on its type
      switch (receivedItem.type) {
        case TASK_BITAXE_UPDATE: {
          if (getCurrentScreen() == SCREEN_BITAXE_HASHRATE) {
          taskEpdContent =
                parseBitaxeHashRate(getBitAxeHashRate());
          } else if (getCurrentScreen() == SCREEN_BITAXE_BESTDIFF) {
          taskEpdContent =
                parseBitaxeBestDiff(getBitaxeBestDiff());
          }
          setEpdContent(taskEpdContent);

        }
        break;
        case TASK_PRICE_UPDATE: {
          uint price = getPrice();
          u_char priceSymbol = '$';
          if (preferences.getBool("fetchEurPrice", DEFAULT_FETCH_EUR_PRICE)) {
            priceSymbol = '[';
          }
          if (getCurrentScreen() == SCREEN_BTC_TICKER) {
            taskEpdContent = parsePriceData(price, priceSymbol, preferences.getBool("suffixPrice", DEFAULT_SUFFIX_PRICE));
          } else if (getCurrentScreen() == SCREEN_MSCW_TIME) {
            taskEpdContent = parseSatsPerCurrency(price, priceSymbol, preferences.getBool("useSatsSymbol", DEFAULT_USE_SATS_SYMBOL));
          } else {
            taskEpdContent =
                parseMarketCap(getBlockHeight(), price, priceSymbol,
                               preferences.getBool("mcapBigChar", DEFAULT_MCAP_BIG_CHAR));
          }

          setEpdContent(taskEpdContent);
          break;
        }
        case TASK_FEE_UPDATE: {
          if (getCurrentScreen() == SCREEN_BLOCK_FEE_RATE) {
            taskEpdContent = parseBlockFees(static_cast<std::uint16_t>(getBlockMedianFee()));
            setEpdContent(taskEpdContent);
          } 
          break;
        }
        case TASK_BLOCK_UPDATE: {
          if (getCurrentScreen() != SCREEN_HALVING_COUNTDOWN) {
            taskEpdContent = parseBlockHeight(getBlockHeight());
          } else {
            taskEpdContent = parseHalvingCountdown(getBlockHeight(), preferences.getBool("useBlkCountdown", DEFAULT_USE_BLOCK_COUNTDOWN));
          }

          if (getCurrentScreen() == SCREEN_HALVING_COUNTDOWN ||
              getCurrentScreen() == SCREEN_BLOCK_HEIGHT) {
            setEpdContent(taskEpdContent);
          }
          break;
        }
        case TASK_TIME_UPDATE: {
          if (getCurrentScreen() == SCREEN_TIME) {
            time_t currentTime;
            struct tm timeinfo;
            time(&currentTime);
            localtime_r(&currentTime, &timeinfo);
            std::string timeString;

            String minute = String(timeinfo.tm_min);
            if (minute.length() < 2) {
              minute = "0" + minute;
            }

            timeString =
                std::to_string(timeinfo.tm_hour) + ":" + minute.c_str();
            timeString.insert(timeString.begin(),
                              NUM_SCREENS - timeString.length(), ' ');
            taskEpdContent[0] = std::to_string(timeinfo.tm_mday) + "/" +
                                std::to_string(timeinfo.tm_mon + 1);

            for (uint i = 1; i < NUM_SCREENS; i++) {
              taskEpdContent[i] = timeString[i];
            }
            setEpdContent(taskEpdContent);
          }

          break;
        }
          // Add more cases for additional task types
      }
    }
  }
}

void taskScreenRotate(void *pvParameters) {
  for (;;) {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

    nextScreen();
  }
}

void IRAM_ATTR minuteTimerISR(void *arg) {
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  //    vTaskNotifyGiveFromISR(timeUpdateTaskHandle, &xHigherPriorityTaskWoken);
  WorkItem timeUpdate = {TASK_TIME_UPDATE, 0};
  xQueueSendFromISR(workQueue, &timeUpdate, &xHigherPriorityTaskWoken);
  if (priceFetchTaskHandle != NULL) {
    vTaskNotifyGiveFromISR(priceFetchTaskHandle, &xHigherPriorityTaskWoken);
  }

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

void setupTasks() {
  workQueue = xQueueCreate(WORK_QUEUE_SIZE, sizeof(WorkItem));

  xTaskCreate(workerTask, "workerTask", 4096, NULL, tskIDLE_PRIORITY,
              &workerTaskHandle);

  xTaskCreate(taskScreenRotate, "rotateScreen", 2048, NULL, tskIDLE_PRIORITY,
              &taskScreenRotateTaskHandle);

  waitUntilNoneBusy();
  setCurrentScreen(preferences.getUInt("currentScreen", DEFAULT_CURRENT_SCREEN));
}

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

uint getCurrentScreen() { return currentScreen; }

void setCurrentScreen(uint newScreen) {
  if (newScreen != SCREEN_CUSTOM) {
    preferences.putUInt("currentScreen", newScreen);
  }

  currentScreen = newScreen;

  switch (currentScreen) {
    case SCREEN_TIME: {
      WorkItem timeUpdate = {TASK_TIME_UPDATE, 0};
      xQueueSend(workQueue, &timeUpdate, portMAX_DELAY);
      //  xTaskNotifyGive(timeUpdateTaskHandle);
      break;
    }
    case SCREEN_HALVING_COUNTDOWN:
    case SCREEN_BLOCK_HEIGHT: {
      WorkItem blockUpdate = {TASK_BLOCK_UPDATE, 0};
      xQueueSend(workQueue, &blockUpdate, portMAX_DELAY);
      // xTaskNotifyGive(blockUpdateTaskHandle);
      break;
    }
    case SCREEN_MARKET_CAP:
    case SCREEN_MSCW_TIME:
    case SCREEN_BTC_TICKER: {
      WorkItem priceUpdate = {TASK_PRICE_UPDATE, 0};
      xQueueSend(workQueue, &priceUpdate, portMAX_DELAY);
      // xTaskNotifyGive(priceUpdateTaskHandle);
      break;
    }
    case SCREEN_BLOCK_FEE_RATE: {
      WorkItem blockUpdate = {TASK_FEE_UPDATE, 0};
      xQueueSend(workQueue, &blockUpdate, portMAX_DELAY);
      break;
    }
    case SCREEN_BITAXE_BESTDIFF:
    case SCREEN_BITAXE_HASHRATE: {
      if (preferences.getBool("bitaxeEnabled", DEFAULT_BITAXE_ENABLED)) {
        WorkItem bitaxeUpdate = {TASK_BITAXE_UPDATE, 0};
        xQueueSend(workQueue, &bitaxeUpdate, portMAX_DELAY);
      } else {
        setCurrentScreen(SCREEN_BLOCK_HEIGHT);
        return;
      }
      break;
    }
  }

  if (eventSourceTaskHandle != NULL) xTaskNotifyGive(eventSourceTaskHandle);
}

void nextScreen() {
  int currentIndex = findScreenIndexByValue(getCurrentScreen());
  std::vector<ScreenMapping> screenMappings = getScreenNameMap();

  int newCurrentScreen;

  if (currentIndex < screenMappings.size() - 1) {
    newCurrentScreen = (screenMappings[currentIndex + 1].value);
  } else {
    newCurrentScreen = screenMappings.front().value;
  }

  String key = "screen" + String(screenMappings[currentIndex - 1].value) + "Visible";

  while (!preferences.getBool(key.c_str(), true)) {
    currentIndex = findScreenIndexByValue(newCurrentScreen);
    if (currentIndex < screenMappings.size() - 1) {
      newCurrentScreen = (screenMappings[currentIndex + 1].value);
    } else {
      newCurrentScreen = screenMappings.front().value;
    }

    key = "screen" + String(newCurrentScreen) + "Visible";
  }

  setCurrentScreen(newCurrentScreen);
}

void previousScreen() {
  int currentIndex = findScreenIndexByValue(getCurrentScreen());
  std::vector<ScreenMapping> screenMappings = getScreenNameMap();

  int newCurrentScreen;

  if (currentIndex > 0) {
    newCurrentScreen = screenMappings[currentIndex - 1].value;
  } else {
    newCurrentScreen = screenMappings.back().value;
  }

  String key = "screen" + String(newCurrentScreen) + "Visible";

  while (!preferences.getBool(key.c_str(), true)) {
    int currentIndex = findScreenIndexByValue(newCurrentScreen);
    if (currentIndex > 0) {
      newCurrentScreen = screenMappings[currentIndex - 1].value;
    } else {
      newCurrentScreen = screenMappings.back().value;
    }

    key = "screen" + String(newCurrentScreen) + "Visible";
  }
  setCurrentScreen(newCurrentScreen);
}

void showSystemStatusScreen() {
  std::array<String, NUM_SCREENS> sysStatusEpdContent = {"", "", "", "",
                                                         "", "", ""};

  String ipAddr = WiFi.localIP().toString();
  String subNet = WiFi.subnetMask().toString();

  sysStatusEpdContent[0] = "IP/Subnet";

  int ipAddrPos = 0;
  int subnetPos = 0;
  for (int i = 0; i < 4; i++) {
    sysStatusEpdContent[1 + i] = ipAddr.substring(0, ipAddr.indexOf('.')) +
                                 "/" + subNet.substring(0, subNet.indexOf('.'));
    ipAddrPos = ipAddr.indexOf('.') + 1;
    subnetPos = subNet.indexOf('.') + 1;
    ipAddr = ipAddr.substring(ipAddrPos);
    subNet = subNet.substring(subnetPos);
  }
  sysStatusEpdContent[NUM_SCREENS - 2] = "RAM/Status";

  sysStatusEpdContent[NUM_SCREENS - 1] =
      String((int)round(ESP.getFreeHeap() / 1024)) + "/" +
      (int)round(ESP.getHeapSize() / 1024);
  setCurrentScreen(SCREEN_CUSTOM);
  setEpdContent(sysStatusEpdContent);
}