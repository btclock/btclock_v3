#include "bitaxe_fetch.hpp"

TaskHandle_t bitaxeFetchTaskHandle;

std::string bitaxeHashrate;
std::string bitaxeBestDiff;

std::string getBitAxeHashRate()
{
    return bitaxeHashrate;
}

std::string getBitaxeBestDiff()
{
    return bitaxeBestDiff;
}

void taskBitaxeFetch(void *pvParameters)
{
    for (;;)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        HTTPClient http;
        http.setUserAgent(USER_AGENT);
        String bitaxeApiUrl = "http://" + preferences.getString("bitaxeHostname", DEFAULT_BITAXE_HOSTNAME) + "/api/system/info";
        http.begin(bitaxeApiUrl.c_str());

        int httpCode = http.GET();

        if (httpCode == 200)
        {
            String payload = http.getString();
            JsonDocument doc;
            deserializeJson(doc, payload);
            bitaxeHashrate = std::to_string(static_cast<int>(std::round(doc["hashRate"].as<float>())));
            bitaxeBestDiff = doc["bestDiff"].as<std::string>();

            if (workQueue != nullptr && (getCurrentScreen() == SCREEN_BITAXE_HASHRATE || getCurrentScreen() == SCREEN_BITAXE_BESTDIFF))
            {
                WorkItem priceUpdate = {TASK_BITAXE_UPDATE, 0};
                xQueueSend(workQueue, &priceUpdate, portMAX_DELAY);
            }
        }
        else
        {
            Serial.print(
                F("Error retrieving BitAxe data. HTTP status code: "));
            Serial.println(httpCode);
            Serial.println(bitaxeApiUrl);
        }
    }
}

void setupBitaxeFetchTask()
{
    xTaskCreate(taskBitaxeFetch, "bitaxeFetch", (6 * 1024), NULL, tskIDLE_PRIORITY,
                &bitaxeFetchTaskHandle);

    xTaskNotifyGive(bitaxeFetchTaskHandle);
}