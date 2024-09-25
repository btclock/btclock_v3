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
    WiFiClientSecure client;

    client.setCACert(srpool_ca);

    for (;;)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        HTTPClient http;
        http.setUserAgent(USER_AGENT);
        String bitaxeApiUrl = preferences.getString("bitaxeHostname", DEFAULT_BITAXE_HOSTNAME);
        http.begin(client, bitaxeApiUrl.c_str());

        int httpCode = http.GET();

        if (httpCode == 200)
        {
            String payload = http.getString();
            JsonDocument doc;
            deserializeJson(doc, payload);
            bitaxeHashrate = doc["hashrate1m"].as<std::string>();
            bitaxeBestDiff = formatNumberWithSuffix(doc["bestever"].as<uint64_t>());

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