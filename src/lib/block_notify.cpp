#include "block_notify.hpp"

const char *wsServer = "wss://mempool.space/api/v1/ws";
// WebsocketsClient client;
esp_websocket_client_handle_t client;
unsigned long int currentBlockHeight;

void setupBlockNotify()
{
    IPAddress result;

    int dnsErr = -1;
    String mempoolInstance = preferences.getString("mempoolInstance", DEFAULT_MEMPOOL_INSTANCE);

    while (dnsErr != 1) {
        dnsErr = WiFi.hostByName(mempoolInstance.c_str(), result);

        if (dnsErr != 1) {
            Serial.print(mempoolInstance);
            Serial.println("mempool DNS could not be resolved");
            WiFi.reconnect();
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
    Serial.println("mempool DNS can be resolved");

    // Get current block height through regular API
    HTTPClient *http = new HTTPClient();
    http->begin("https://" + mempoolInstance + "/api/blocks/tip/height");
    int httpCode = http->GET();

    if (httpCode > 0 && httpCode == HTTP_CODE_OK)
    {
        String blockHeightStr = http->getString();
        currentBlockHeight = blockHeightStr.toInt();
        xTaskNotifyGive(blockUpdateTaskHandle);
    }

    esp_websocket_client_config_t config = {
        .uri = "wss://mempool.bitcoin.nl/api/v1/ws",
    };

    Serial.printf("Connecting to %s\r\n", config.uri);

    client = esp_websocket_client_init(&config);
    esp_websocket_register_events(client, WEBSOCKET_EVENT_ANY, onWebsocketEvent, client);
    esp_websocket_client_start(client);
}

void onWebsocketEvent(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_websocket_event_data_t *data = (esp_websocket_event_data_t *)event_data;
    String init;
    String sub;
    switch (event_id)
    {
    case WEBSOCKET_EVENT_CONNECTED:
        Serial.println("Connected to Mempool.space WebSocket");
        
        sub = "{\"action\": \"want\", \"data\":[\"blocks\"]}";
        if (esp_websocket_client_send_text(client, sub.c_str(), sub.length(), portMAX_DELAY) == -1)
        {
            Serial.println("Mempool.space WS Block Subscribe Error");
        }

        break;
    case WEBSOCKET_EVENT_DATA:
        onWebsocketMessage(data);
        // Handle the received WebSocket message (block notifications) here
        break;
    case WEBSOCKET_EVENT_ERROR:
        Serial.println("Mempool.space WS Connnection error");
        break;
    case WEBSOCKET_EVENT_DISCONNECTED:
        Serial.println("Mempool.space WS Connnection Closed");
        break;
    }
}

void onWebsocketMessage(esp_websocket_event_data_t *event_data)
{
    SpiRamJsonDocument doc(event_data->data_len);

    deserializeJson(doc, (char *)event_data->data_ptr);
    // serializeJsonPretty(doc, Serial);

    if (doc.containsKey("block"))
    {
        JsonObject block = doc["block"];

        currentBlockHeight = block["height"].as<long>();
        Serial.print("New block found: ");
        Serial.println(block["height"].as<long>());

        if (blockUpdateTaskHandle != nullptr)
            xTaskNotifyGive(blockUpdateTaskHandle);
    }

    doc.clear();
}

unsigned long getBlockHeight()
{
    return currentBlockHeight;
}