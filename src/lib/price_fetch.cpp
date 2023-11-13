#include "price_fetch.hpp"

const PROGMEM char *cgApiUrl = "https://api.coingecko.com/api/v3/simple/price?ids=bitcoin&vs_currencies=usd%2Ceur";

TaskHandle_t priceFetchTaskHandle;

void taskPriceFetch(void *pvParameters)
{
    WiFiClientSecure *client = new WiFiClientSecure;
    client->setInsecure();
    for (;;)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        HTTPClient *http = new HTTPClient();
        http->setUserAgent(USER_AGENT);

        // Send HTTP request to CoinGecko API
        http->begin(*client, cgApiUrl);

        int httpCode = http->GET();

        // Parse JSON response and extract average price
        uint usdPrice, eurPrice;
        if (httpCode == 200)
        {
            String payload = http->getString();
            StaticJsonDocument<96> doc;
            deserializeJson(doc, payload);
//            usdPrice = doc["bitcoin"]["usd"];
            eurPrice = doc["bitcoin"]["eur"].as<uint>();

            setPrice(eurPrice);
            if (workQueue != nullptr && (getCurrentScreen() == SCREEN_BTC_TICKER || getCurrentScreen() == SCREEN_MSCW_TIME || getCurrentScreen() == SCREEN_MARKET_CAP))
            {
                WorkItem priceUpdate = {TASK_PRICE_UPDATE, 0};
                xQueueSend(workQueue, &priceUpdate, portMAX_DELAY);
            }

            preferences.putUInt("lastPrice", eurPrice);
        }
        else
        {
            Serial.print(F("Error retrieving BTC/USD price (CoinGecko). HTTP status code: "));
            Serial.println(httpCode);
            if (httpCode == -1)
            {
                WiFi.reconnect();
            }
        }
    }
}

void setupPriceFetchTask()
{
    xTaskCreate(taskPriceFetch, "priceFetch", (6*1024), NULL, tskIDLE_PRIORITY, &priceFetchTaskHandle);

    xTaskNotifyGive(priceFetchTaskHandle);

}