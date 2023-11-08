#include "price_notify.hpp"

const char *wsServerPrice = "wss://ws.coincap.io/prices?assets=bitcoin";
// WebsocketsClient client;
esp_websocket_client_handle_t clientPrice = NULL;
unsigned long int currentPrice;
unsigned long int lastPriceUpdate = 0;

void setupPriceNotify()
{
    esp_websocket_client_config_t config = {
        .uri = wsServerPrice,
        .user_agent = USER_AGENT,
    };

    clientPrice = esp_websocket_client_init(&config);
    esp_websocket_register_events(clientPrice, WEBSOCKET_EVENT_ANY, onWebsocketPriceEvent, clientPrice);
    esp_websocket_client_start(clientPrice);
}

void onWebsocketPriceEvent(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_websocket_event_data_t *data = (esp_websocket_event_data_t *)event_data;

    switch (event_id)
    {
    case WEBSOCKET_EVENT_CONNECTED:
        Serial.println(F("Connected to CoinCap.io WebSocket"));
        break;
    case WEBSOCKET_EVENT_DATA:
        onWebsocketPriceMessage(data);
        break;
    case WEBSOCKET_EVENT_ERROR:
        Serial.println(F("Price WS Connnection error"));
        break;
    case WEBSOCKET_EVENT_DISCONNECTED:
        Serial.println(F("Price WS Connnection Closed"));
        break;
    }
}

void onWebsocketPriceMessage(esp_websocket_event_data_t* event_data)
{
    SpiRamJsonDocument doc(event_data->data_len);

    deserializeJson(doc, (char *)event_data->data_ptr);

    if (doc.containsKey("bitcoin")) {
        if (currentPrice != doc["bitcoin"].as<long>()) {

            uint minSecPriceUpd = preferences.getUInt("minSecPriceUpd", DEFAULT_SECONDS_BETWEEN_PRICE_UPDATE);
            uint currentTime = esp_timer_get_time() / 1000000;

            if (lastPriceUpdate == 0 || (currentTime - lastPriceUpdate) > minSecPriceUpd) {
             //   const unsigned long oldPrice = currentPrice;
                currentPrice = doc["bitcoin"].as<long>();

                lastPriceUpdate = currentTime;
            // if (abs((int)(oldPrice-currentPrice)) > round(0.0015*oldPrice)) {
                    if (priceUpdateTaskHandle != nullptr && (getCurrentScreen() == SCREEN_BTC_TICKER || getCurrentScreen() == SCREEN_MSCW_TIME))
                        xTaskNotifyGive(priceUpdateTaskHandle);
                //}
            }
        }
    }
}

unsigned long getPrice() {
    return currentPrice;
}

bool isPriceNotifyConnected() {
    if (clientPrice == NULL)
        return false;
    return esp_websocket_client_is_connected(clientPrice);
}