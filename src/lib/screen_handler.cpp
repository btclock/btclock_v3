#include "screen_handler.hpp"

// TaskHandle_t priceUpdateTaskHandle;
// TaskHandle_t blockUpdateTaskHandle;
// TaskHandle_t timeUpdateTaskHandle;
TaskHandle_t taskScreenRotateTaskHandle;
TaskHandle_t workerTaskHandle;
esp_timer_handle_t screenRotateTimer;
esp_timer_handle_t minuteTimer;

std::array<String, NUM_SCREENS> taskEpdContent = {"", "", "", "", "", "", ""};
std::string priceString;


// typedef enum
// {
//     TASK_PRICE_UPDATE,
//     TASK_BLOCK_UPDATE,
//     TASK_TIME_UPDATE
// } TaskType;

// typedef struct
// {
//     TaskType type;
//     unsigned long data;
// } WorkItem;

#define WORK_QUEUE_SIZE 10
QueueHandle_t workQueue = NULL;

uint currentScreen;

void workerTask(void *pvParameters)
{
    WorkItem receivedItem;

    while (1)
    {
        // Wait for a work item to be available in the queue
        if (xQueueReceive(workQueue, &receivedItem, portMAX_DELAY))
        {
            uint firstIndex = 0;

            // Process the work item based on its type
            switch (receivedItem.type)
            {
            case TASK_PRICE_UPDATE:
            {
                firstIndex = 0;
                uint price = getPrice();
                char priceSymbol = '$';
                if (getCurrentScreen() == SCREEN_BTC_TICKER)
                {
                    if (preferences.getBool("fetchEurPrice", false)) {
                        priceSymbol = '[';
                    }

                    priceString = (priceSymbol + String(price)).c_str();

                    if (priceString.length() < (NUM_SCREENS))
                    {
                        priceString.insert(priceString.begin(), NUM_SCREENS - priceString.length(), ' ');
                        if (preferences.getBool("fetchEurPrice", false)) {
                            taskEpdContent[0] = "BTC/EUR";
                        } else {
                            taskEpdContent[0] = "BTC/USD";
                        }
                        firstIndex = 1;
                    }
                }
                else if (getCurrentScreen() == SCREEN_MSCW_TIME)
                {
                    priceString = String(int(round(1 / float(price) * 10e7))).c_str();

                    if (priceString.length() < (NUM_SCREENS))
                    {
                        priceString.insert(priceString.begin(), NUM_SCREENS - priceString.length(), ' ');
                        taskEpdContent[0] = "MSCW/TIME";
                        firstIndex = 1;
                    }
                }
                else
                {
                    double supply = getSupplyAtBlock(getBlockHeight());
                    int64_t marketCap = static_cast<std::int64_t>(supply * double(price));

                    taskEpdContent[0] = "USD/MCAP";

                    if (preferences.getBool("mcapBigChar", true))
                    {
                        firstIndex = 1;

                        priceString = "$" + formatNumberWithSuffix(marketCap);
                        priceString.insert(priceString.begin(), NUM_SCREENS - priceString.length(), ' ');
                    }
                    else
                    {
                        std::string stringValue = std::to_string(marketCap);
                        size_t mcLength = stringValue.length();
                        size_t leadingSpaces = (3 - mcLength % 3) % 3;
                        stringValue = std::string(leadingSpaces, ' ') + stringValue;

                        uint groups = (mcLength + leadingSpaces) / 3;

                        if (groups < NUM_SCREENS)
                        {
                            firstIndex = 1;
                        }

                        for (int i = firstIndex; i < NUM_SCREENS - groups - 1; i++)
                        {
                            taskEpdContent[i] = "";
                        }

                        taskEpdContent[NUM_SCREENS - groups - 1] = " $ ";
                        for (uint i = 0; i < groups; i++)
                        {
                            taskEpdContent[(NUM_SCREENS - groups + i)] = stringValue.substr(i * 3, 3).c_str();
                        }
                    }
                }

                if (!(getCurrentScreen() == SCREEN_MARKET_CAP && !preferences.getBool("mcapBigChar", true)))
                {
                    for (uint i = firstIndex; i < NUM_SCREENS; i++)
                    {
                        taskEpdContent[i] = priceString[i];
                    }
                }

                setEpdContent(taskEpdContent);
                break;
            }
            case TASK_BLOCK_UPDATE:
            {
                std::string blockNrString = String(getBlockHeight()).c_str();
                firstIndex = 0;

                if (getCurrentScreen() != SCREEN_HALVING_COUNTDOWN)
                {
                    if (blockNrString.length() < NUM_SCREENS)
                    {
                        blockNrString.insert(blockNrString.begin(), NUM_SCREENS - blockNrString.length(), ' ');
                        taskEpdContent[0] = "BLOCK/HEIGHT";
                        firstIndex = 1;
                    }

                    for (uint i = firstIndex; i < NUM_SCREENS; i++)
                    {
                        taskEpdContent[i] = blockNrString[i];
                    }
                }
                else
                {
                    const uint nextHalvingBlock = 210000 - (getBlockHeight() % 210000);
                    const uint minutesToHalving = nextHalvingBlock * 10;

                    const int years = floor(minutesToHalving / 525600);
                    const int days = floor((minutesToHalving - (years * 525600)) / (24 * 60));
                    const int hours = floor((minutesToHalving - (years * 525600) - (days * (24 * 60))) / 60);
                    const int mins = floor(minutesToHalving - (years * 525600) - (days * (24 * 60)) - (hours * 60));
                    taskEpdContent[0] = "BIT/COIN";
                    taskEpdContent[1] = "HALV/ING";
                    taskEpdContent[(NUM_SCREENS - 5)] = String(years) + "/YRS";
                    taskEpdContent[(NUM_SCREENS - 4)] = String(days) + "/DAYS";
                    taskEpdContent[(NUM_SCREENS - 3)] = String(hours) + "/HRS";
                    taskEpdContent[(NUM_SCREENS - 2)] = String(mins) + "/MINS";
                    taskEpdContent[(NUM_SCREENS - 1)] = "TO/GO";
                }

                if (getCurrentScreen() == SCREEN_HALVING_COUNTDOWN || getCurrentScreen() == SCREEN_BLOCK_HEIGHT)
                {
                    setEpdContent(taskEpdContent);
                }
                break;
            }
            case TASK_TIME_UPDATE:
            {
                if (getCurrentScreen() == SCREEN_TIME)
                {
                    time_t currentTime;
                    struct tm timeinfo;
                    time(&currentTime);
                    localtime_r(&currentTime, &timeinfo);
                    std::string timeString;

                    String minute = String(timeinfo.tm_min);
                    if (minute.length() < 2)
                    {
                        minute = "0" + minute;
                    }

                    timeString = std::to_string(timeinfo.tm_hour) + ":" + minute.c_str();
                    timeString.insert(timeString.begin(), NUM_SCREENS - timeString.length(), ' ');
                    taskEpdContent[0] = String(timeinfo.tm_mday) + "/" + String(timeinfo.tm_mon + 1);

                    for (uint i = 1; i < NUM_SCREENS; i++)
                    {
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


void taskScreenRotate(void *pvParameters)
{
    for (;;)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        int nextScreen = (currentScreen + 1) % 5;
        String key = "screen" + String(nextScreen) + "Visible";

        while (!preferences.getBool(key.c_str(), true))
        {
            nextScreen = (nextScreen + 1) % 5;
            key = "screen" + String(nextScreen) + "Visible";
        }

        setCurrentScreen(nextScreen);
    }
}

void IRAM_ATTR minuteTimerISR(void *arg)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
//    vTaskNotifyGiveFromISR(timeUpdateTaskHandle, &xHigherPriorityTaskWoken);
    WorkItem timeUpdate = {TASK_TIME_UPDATE, 0};
    xQueueSendFromISR(workQueue, &timeUpdate, &xHigherPriorityTaskWoken);
    if (priceFetchTaskHandle != NULL) {
        vTaskNotifyGiveFromISR(priceFetchTaskHandle, &xHigherPriorityTaskWoken);
    }
    if (xHigherPriorityTaskWoken == pdTRUE)
    {
        portYIELD_FROM_ISR();
    }
}

void IRAM_ATTR screenRotateTimerISR(void *arg)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    vTaskNotifyGiveFromISR(taskScreenRotateTaskHandle, &xHigherPriorityTaskWoken);
    if (xHigherPriorityTaskWoken == pdTRUE)
    {
        portYIELD_FROM_ISR();
    }
}

void setupTasks()
{
    workQueue = xQueueCreate(WORK_QUEUE_SIZE, sizeof(WorkItem));

    // xTaskCreate(taskPriceUpdate, "updatePrice", 1024, NULL, tskIDLE_PRIORITY, &priceUpdateTaskHandle);
    // xTaskCreate(taskBlockUpdate, "updateBlock", 1024, NULL, tskIDLE_PRIORITY, &blockUpdateTaskHandle);
    // xTaskCreate(taskTimeUpdate, "updateTime", 1024, NULL, tskIDLE_PRIORITY, &timeUpdateTaskHandle);
    xTaskCreate(workerTask, "workerTask", 4096, NULL, tskIDLE_PRIORITY, &workerTaskHandle);

    xTaskCreate(taskScreenRotate, "rotateScreen", 2048, NULL, tskIDLE_PRIORITY, &taskScreenRotateTaskHandle);

    waitUntilNoneBusy();
    setCurrentScreen(preferences.getUInt("currentScreen", 0));
}

void setupTimeUpdateTimer(void *pvParameters)
{
    const esp_timer_create_args_t minuteTimerConfig = {
        .callback = &minuteTimerISR,
        .name = "minute_timer"};

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

void setupScreenRotateTimer(void *pvParameters)
{
    const esp_timer_create_args_t screenRotateTimerConfig = {
        .callback = &screenRotateTimerISR,
        .name = "screen_rotate_timer"};

    esp_timer_create(&screenRotateTimerConfig, &screenRotateTimer);

    if (preferences.getBool("timerActive", true))
    {
        esp_timer_start_periodic(screenRotateTimer, getTimerSeconds() * usPerSecond);
    }

    vTaskDelete(NULL);
}

uint getTimerSeconds()
{
    return preferences.getUInt("timerSeconds", 1800);
}

bool isTimerActive()
{
    return esp_timer_is_active(screenRotateTimer);
}

void setTimerActive(bool status)
{
    if (status)
    {
        esp_timer_start_periodic(screenRotateTimer, getTimerSeconds() * usPerSecond);
        queueLedEffect(LED_EFFECT_START_TIMER);
        preferences.putBool("timerActive", true);
    }
    else
    {
        esp_timer_stop(screenRotateTimer);
        queueLedEffect(LED_EFFECT_PAUSE_TIMER);
        preferences.putBool("timerActive", false);
    }

    if (eventSourceTaskHandle != NULL)
        xTaskNotifyGive(eventSourceTaskHandle);
}

void toggleTimerActive()
{
    setTimerActive(!isTimerActive());
}

uint getCurrentScreen()
{
    return currentScreen;
}

void setCurrentScreen(uint newScreen)
{
    if (newScreen != SCREEN_CUSTOM)
    {
        preferences.putUInt("currentScreen", newScreen);
    }

    currentScreen = newScreen;

    switch (currentScreen)
    {
    case SCREEN_TIME:
    {
        WorkItem timeUpdate = {TASK_TIME_UPDATE, 0};
        xQueueSend(workQueue, &timeUpdate, portMAX_DELAY);
      //  xTaskNotifyGive(timeUpdateTaskHandle);
        break;
    }
    case SCREEN_HALVING_COUNTDOWN:
    case SCREEN_BLOCK_HEIGHT:
    {
        WorkItem blockUpdate = {TASK_BLOCK_UPDATE, 0};
        xQueueSend(workQueue, &blockUpdate, portMAX_DELAY);
        //xTaskNotifyGive(blockUpdateTaskHandle);
        break;
    }
    case SCREEN_MARKET_CAP:
    case SCREEN_MSCW_TIME:
    case SCREEN_BTC_TICKER:
    {
        WorkItem priceUpdate = {TASK_PRICE_UPDATE, 0};
        xQueueSend(workQueue, &priceUpdate, portMAX_DELAY);
        //xTaskNotifyGive(priceUpdateTaskHandle);
        break;
    }
    }

    if (eventSourceTaskHandle != NULL)
        xTaskNotifyGive(eventSourceTaskHandle);
}

void nextScreen()
{
    int newCurrentScreen = (getCurrentScreen() + 1) % SCREEN_COUNT;
    String key = "screen" + String(newCurrentScreen) + "Visible";

    while (!preferences.getBool(key.c_str(), true))
    {
        newCurrentScreen = (newCurrentScreen + 1) % SCREEN_COUNT;
        key = "screen" + String(newCurrentScreen) + "Visible";
    }
    setCurrentScreen(newCurrentScreen);
}

void previousScreen()
{

    int newCurrentScreen = modulo(getCurrentScreen() - 1, SCREEN_COUNT);
    String key = "screen" + String(newCurrentScreen) + "Visible";

    while (!preferences.getBool(key.c_str(), true))
    {
        newCurrentScreen = modulo(newCurrentScreen - 1, SCREEN_COUNT);
        key = "screen" + String(newCurrentScreen) + "Visible";
    }
    setCurrentScreen(newCurrentScreen);
}

void showSystemStatusScreen()
{
    std::array<String, NUM_SCREENS> sysStatusEpdContent = {"", "", "", "", "", "", ""};

    String ipAddr = WiFi.localIP().toString();
    String subNet = WiFi.subnetMask().toString();

    sysStatusEpdContent[0] = "IP/Subnet";

    int ipAddrPos = 0;
    int subnetPos = 0;
    for (int i = 0; i < 4; i++)
    {
        sysStatusEpdContent[1 + i] = ipAddr.substring(0, ipAddr.indexOf('.')) + "/" + subNet.substring(0, subNet.indexOf('.'));
        ipAddrPos = ipAddr.indexOf('.') + 1;
        subnetPos = subNet.indexOf('.') + 1;
        ipAddr = ipAddr.substring(ipAddrPos);
        subNet = subNet.substring(subnetPos);
    }
    sysStatusEpdContent[NUM_SCREENS - 2] = "RAM/Status";

    sysStatusEpdContent[NUM_SCREENS - 1] = String((int)round(ESP.getFreeHeap() / 1024)) + "/" + (int)round(ESP.getHeapSize() / 1024);
    setCurrentScreen(SCREEN_CUSTOM);
    setEpdContent(sysStatusEpdContent);
}