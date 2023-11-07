#include "price_notify.hpp"

const char *wsServerPrice = "wss://ws.coincap.io/prices?assets=bitcoin";
// WebsocketsClient client;
esp_websocket_client_handle_t clientPrice;
unsigned long int currentPrice;

void setupPriceNotify()
{
    esp_websocket_client_config_t config = {
        .uri = wsServerPrice,
    };

    clientPrice = esp_websocket_client_init(&config);
    esp_websocket_register_events(clientPrice, WEBSOCKET_EVENT_ANY, onWebsocketPriceEvent, clientPrice);
    esp_websocket_client_start(clientPrice);
}

void onWebsocketPriceEvent(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_websocket_event_data_t *data = (esp_websocket_event_data_t *)event_data;
    String init;
    String sub;
    switch (event_id)
    {
    case WEBSOCKET_EVENT_CONNECTED:
        Serial.println("Connected to CoinCap.io WebSocket");
        break;
    case WEBSOCKET_EVENT_DATA:
        onWebsocketPriceMessage(data);
        break;
    case WEBSOCKET_EVENT_ERROR:
        Serial.println("Connnection error");
        break;
    case WEBSOCKET_EVENT_DISCONNECTED:
        Serial.println("Connnection Closed");
        break;
    }
}

void onWebsocketPriceMessage(esp_websocket_event_data_t* event_data)
{
    DynamicJsonDocument doc(event_data->data_len);

    deserializeJson(doc, (char *)event_data->data_ptr);

    if (doc.containsKey("bitcoin")) {
        if (currentPrice != doc["bitcoin"].as<long>()) {
            Serial.printf("New price %lu\r\n", currentPrice);

            const unsigned long oldPrice = currentPrice;
            currentPrice = doc["bitcoin"].as<long>();

           // if (abs((int)(oldPrice-currentPrice)) > round(0.0015*oldPrice)) {
                if (priceUpdateTaskHandle != nullptr)
                    xTaskNotifyGive(priceUpdateTaskHandle);
            //}
        }
    }
}

unsigned long getPrice() {
    return currentPrice;
}