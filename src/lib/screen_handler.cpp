#include "screen_handler.hpp"

TaskHandle_t priceUpdateTaskHandle;
TaskHandle_t blockUpdateTaskHandle;
TaskHandle_t timeUpdateTaskHandle;
TaskHandle_t taskScreenRotateTaskHandle;
esp_timer_handle_t screenRotateTimer;
esp_timer_handle_t minuteTimer;

std::array<String, NUM_SCREENS> taskEpdContent = {"", "", "", "", "", "", ""};
std::string priceString;
const int usPerSecond = 1000000;
const int usPerMinute = 60 * usPerSecond;
int64_t next_callback_time = 0;

uint currentScreen;

void taskPriceUpdate(void *pvParameters)
{
    for (;;)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        unsigned long price = getPrice();
        uint firstIndex = 0;
        if (getCurrentScreen() == SCREEN_BTC_TICKER)
        {
            priceString = ("$" + String(price)).c_str();

            if (priceString.length() < (NUM_SCREENS))
            {
                priceString.insert(priceString.begin(), NUM_SCREENS - priceString.length(), ' ');
                taskEpdContent[0] = "BTC/USD";
                firstIndex = 1;
            }
        }
        else
        {
            priceString = String(int(round(1 / float(price) * 10e7))).c_str();

            if (priceString.length() < (NUM_SCREENS))
            {
                priceString.insert(priceString.begin(), NUM_SCREENS - priceString.length(), ' ');
                taskEpdContent[0] = "MSCW/TIME";
                firstIndex = 1;
            }
        }

        for (uint i = firstIndex; i < NUM_SCREENS; i++)
        {
            taskEpdContent[i] = priceString[i];
        }

        setEpdContent(taskEpdContent);
    }
}

void taskScreenRotate(void *pvParameters)
{
    for (;;)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        setCurrentScreen((currentScreen+1) % 5);
    }
}

void taskBlockUpdate(void *pvParameters)
{
    for (;;)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        std::string blockNrString = String(getBlockHeight()).c_str();
        uint firstIndex = 0;

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
            taskEpdContent[(NUM_SCREENS - 3)] = String(days) + "/HRS";
            taskEpdContent[(NUM_SCREENS - 2)] = String(mins) + "/MINS";
            taskEpdContent[(NUM_SCREENS - 1)] = "TO/GO";
        }

        setEpdContent(taskEpdContent);
    }
}

void taskTimeUpdate(void *pvParameters)
{
    for (;;)
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

        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    }
}

const char* int64_to_iso8601(int64_t timestamp) {
    time_t seconds = timestamp / 1000000; // Convert microseconds to seconds
    struct tm timeinfo;
    gmtime_r(&seconds, &timeinfo);

    // Define a buffer to store the formatted time string
    static char iso8601[21]; // ISO 8601 time string has the format "YYYY-MM-DDTHH:MM:SSZ"

    // Format the time into the buffer
    strftime(iso8601, sizeof(iso8601), "%Y-%m-%dT%H:%M:%SZ", &timeinfo);

    return iso8601;
}

void IRAM_ATTR minuteTimerISR(void *arg)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    vTaskNotifyGiveFromISR(timeUpdateTaskHandle, &xHigherPriorityTaskWoken);
    if (xHigherPriorityTaskWoken == pdTRUE)
    {
        portYIELD_FROM_ISR();
    }
    int64_t current_time = esp_timer_get_time();
    next_callback_time = current_time + usPerMinute;
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
    xTaskCreate(taskPriceUpdate, "updatePrice", 2048, NULL, tskIDLE_PRIORITY, &priceUpdateTaskHandle);
    xTaskCreate(taskBlockUpdate, "updateBlock", 2048, NULL, tskIDLE_PRIORITY, &blockUpdateTaskHandle);
    xTaskCreate(taskTimeUpdate, "updateTime", 4096, NULL, tskIDLE_PRIORITY, &timeUpdateTaskHandle);
    xTaskCreate(taskScreenRotate, "rotateScreen", 2048, NULL, tskIDLE_PRIORITY, &taskScreenRotateTaskHandle);
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
    xTaskNotifyGive(timeUpdateTaskHandle);

    vTaskDelete(NULL);
}

void setupScreenRotateTimer(void *pvParameters)
{
    const esp_timer_create_args_t screenRotateTimerConfig = {
        .callback = &screenRotateTimerISR,
        .name = "screen_rotate_timer"};

    esp_timer_create(&screenRotateTimerConfig, &screenRotateTimer);

    esp_timer_start_periodic(screenRotateTimer, getTimerSeconds() * usPerSecond);

    Serial.println("Set up Screen Rotate Timer");

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
    }
    else
    {
        esp_timer_stop(screenRotateTimer);
    }
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
        xTaskNotifyGive(timeUpdateTaskHandle);
        break;
    case SCREEN_HALVING_COUNTDOWN:
    case SCREEN_BLOCK_HEIGHT:
        xTaskNotifyGive(blockUpdateTaskHandle);
        break;
    case SCREEN_MSCW_TIME:
    case SCREEN_BTC_TICKER:
        xTaskNotifyGive(priceUpdateTaskHandle);
        break;
    }
}