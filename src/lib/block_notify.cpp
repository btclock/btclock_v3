#include "block_notify.hpp"

char *wsServer;
esp_websocket_client_handle_t blockNotifyClient = NULL;
uint currentBlockHeight = 860000;
uint blockMedianFee = 1;
bool blockNotifyInit = false;
unsigned long int lastBlockUpdate;

const char *mempoolWsCert = R"EOF(
-----BEGIN CERTIFICATE-----
MIIF3jCCA8agAwIBAgIQAf1tMPyjylGoG7xkDjUDLTANBgkqhkiG9w0BAQwFADCB
iDELMAkGA1UEBhMCVVMxEzARBgNVBAgTCk5ldyBKZXJzZXkxFDASBgNVBAcTC0pl
cnNleSBDaXR5MR4wHAYDVQQKExVUaGUgVVNFUlRSVVNUIE5ldHdvcmsxLjAsBgNV
BAMTJVVTRVJUcnVzdCBSU0EgQ2VydGlmaWNhdGlvbiBBdXRob3JpdHkwHhcNMTAw
MjAxMDAwMDAwWhcNMzgwMTE4MjM1OTU5WjCBiDELMAkGA1UEBhMCVVMxEzARBgNV
BAgTCk5ldyBKZXJzZXkxFDASBgNVBAcTC0plcnNleSBDaXR5MR4wHAYDVQQKExVU
aGUgVVNFUlRSVVNUIE5ldHdvcmsxLjAsBgNVBAMTJVVTRVJUcnVzdCBSU0EgQ2Vy
dGlmaWNhdGlvbiBBdXRob3JpdHkwggIiMA0GCSqGSIb3DQEBAQUAA4ICDwAwggIK
AoICAQCAEmUXNg7D2wiz0KxXDXbtzSfTTK1Qg2HiqiBNCS1kCdzOiZ/MPans9s/B
3PHTsdZ7NygRK0faOca8Ohm0X6a9fZ2jY0K2dvKpOyuR+OJv0OwWIJAJPuLodMkY
tJHUYmTbf6MG8YgYapAiPLz+E/CHFHv25B+O1ORRxhFnRghRy4YUVD+8M/5+bJz/
Fp0YvVGONaanZshyZ9shZrHUm3gDwFA66Mzw3LyeTP6vBZY1H1dat//O+T23LLb2
VN3I5xI6Ta5MirdcmrS3ID3KfyI0rn47aGYBROcBTkZTmzNg95S+UzeQc0PzMsNT
79uq/nROacdrjGCT3sTHDN/hMq7MkztReJVni+49Vv4M0GkPGw/zJSZrM233bkf6
c0Plfg6lZrEpfDKEY1WJxA3Bk1QwGROs0303p+tdOmw1XNtB1xLaqUkL39iAigmT
Yo61Zs8liM2EuLE/pDkP2QKe6xJMlXzzawWpXhaDzLhn4ugTncxbgtNMs+1b/97l
c6wjOy0AvzVVdAlJ2ElYGn+SNuZRkg7zJn0cTRe8yexDJtC/QV9AqURE9JnnV4ee
UB9XVKg+/XRjL7FQZQnmWEIuQxpMtPAlR1n6BB6T1CZGSlCBst6+eLf8ZxXhyVeE
Hg9j1uliutZfVS7qXMYoCAQlObgOK6nyTJccBz8NUvXt7y+CDwIDAQABo0IwQDAd
BgNVHQ4EFgQUU3m/WqorSs9UgOHYm8Cd8rIDZsswDgYDVR0PAQH/BAQDAgEGMA8G
A1UdEwEB/wQFMAMBAf8wDQYJKoZIhvcNAQEMBQADggIBAFzUfA3P9wF9QZllDHPF
Up/L+M+ZBn8b2kMVn54CVVeWFPFSPCeHlCjtHzoBN6J2/FNQwISbxmtOuowhT6KO
VWKR82kV2LyI48SqC/3vqOlLVSoGIG1VeCkZ7l8wXEskEVX/JJpuXior7gtNn3/3
ATiUFJVDBwn7YKnuHKsSjKCaXqeYalltiz8I+8jRRa8YFWSQEg9zKC7F4iRO/Fjs
8PRF/iKz6y+O0tlFYQXBl2+odnKPi4w2r78NBc5xjeambx9spnFixdjQg3IM8WcR
iQycE0xyNN+81XHfqnHd4blsjDwSXWXavVcStkNr/+XeTWYRUc+ZruwXtuhxkYze
Sf7dNXGiFSeUHM9h4ya7b6NnJSFd5t0dCy5oGzuCr+yDZ4XUmFF0sbmZgIn/f3gZ
XHlKYC6SQK5MNyosycdiyA5d9zZbyuAlJQG03RoHnHcAP9Dc1ew91Pq7P8yF1m9/
qS3fuQL39ZeatTXaw2ewh0qpKJ4jjv9cJ2vhsE/zB+4ALtRZh8tSQZXq9EfX7mRB
VXyNWQKV3WKdwrnuWih0hKWbt5DHDAff9Yk2dDLWKMGwsAvgnEzDHNb842m1R0aB
L6KCq9NjRHDEjf8tM7qtj3u1cIiuPhnPQCjY/MiQu12ZIvVS5ljFH4gxQ+6IHdfG
jjxDah2nGN59PRbxYvnKkKj9
-----END CERTIFICATE-----
)EOF";

void setupBlockNotify()
{
  IPAddress result;

  int dnsErr = -1;
  String mempoolInstance =
      preferences.getString("mempoolInstance", DEFAULT_MEMPOOL_INSTANCE);

  while (dnsErr != 1 &&  !strchr(mempoolInstance.c_str(), ':'))
  {
    dnsErr = WiFi.hostByName(mempoolInstance.c_str(), result);

    if (dnsErr != 1)
    {
      Serial.print(mempoolInstance);
      Serial.println(F("mempool DNS could not be resolved"));
      WiFi.reconnect();
      vTaskDelay(pdMS_TO_TICKS(1000));
    }
  }

  // Get current block height through regular API
  int blockFetch = getBlockFetch();

  if (blockFetch > currentBlockHeight)
    currentBlockHeight = blockFetch;

  if (currentBlockHeight != -1)
  {
    lastBlockUpdate = esp_timer_get_time() / 1000000;
  }

  if (workQueue != nullptr)
  {
    WorkItem blockUpdate = {TASK_BLOCK_UPDATE, 0};
    xQueueSend(workQueue, &blockUpdate, portMAX_DELAY);
  }

  if (!preferences.getBool("fetchEurPrice", DEFAULT_FETCH_EUR_PRICE) && preferences.getBool("ownDataSource", DEFAULT_OWN_DATA_SOURCE))
  {
    return;
  }

  // std::strcpy(wsServer, String("wss://" + mempoolInstance +
  // "/api/v1/ws").c_str());

  const String protocol = preferences.getBool("mempoolSecure", DEFAULT_MEMPOOL_SECURE) ? "wss" : "ws";

  String mempoolUri = protocol + "://" + preferences.getString("mempoolInstance", DEFAULT_MEMPOOL_INSTANCE) + "/api/v1/ws";
  
  esp_websocket_client_config_t config = {
     // .uri = "wss://mempool.space/api/v1/ws",
      .task_stack = (6*1024),
      .user_agent = USER_AGENT
  };

  if (preferences.getBool("mempoolSecure", DEFAULT_MEMPOOL_SECURE)) {
    config.cert_pem = mempoolWsCert;
  }

  config.uri = mempoolUri.c_str();

  Serial.printf("Connecting to %s\r\n", preferences.getString("mempoolInstance", DEFAULT_MEMPOOL_INSTANCE));

  blockNotifyClient = esp_websocket_client_init(&config);
  esp_websocket_register_events(blockNotifyClient, WEBSOCKET_EVENT_ANY,
                                onWebsocketBlockEvent, blockNotifyClient);
  esp_websocket_client_start(blockNotifyClient);
}

void onWebsocketBlockEvent(void *handler_args, esp_event_base_t base,
                      int32_t event_id, void *event_data)
{
  esp_websocket_event_data_t *data = (esp_websocket_event_data_t *)event_data;
  const String sub = "{\"action\": \"want\", \"data\":[\"blocks\", \"mempool-blocks\"]}";
  switch (event_id)
  {
  case WEBSOCKET_EVENT_CONNECTED:
    blockNotifyInit = true;

    Serial.println(F("Connected to Mempool.space WebSocket"));

    Serial.println(sub);
    if (esp_websocket_client_send_text(blockNotifyClient, sub.c_str(),
                                       sub.length(), portMAX_DELAY) == -1)
    {
      Serial.println(F("Mempool.space WS Block Subscribe Error"));
    }

    break;
  case WEBSOCKET_EVENT_DATA:
    onWebsocketBlockMessage(data);
    break;
  case WEBSOCKET_EVENT_ERROR:
    Serial.println(F("Mempool.space WS Connnection error"));
    break;
  case WEBSOCKET_EVENT_DISCONNECTED:
    Serial.println(F("Mempool.space WS Connnection Closed"));
    break;
  }
}

void onWebsocketBlockMessage(esp_websocket_event_data_t *event_data)
{
  JsonDocument doc;

  JsonDocument filter;
  filter["block"]["height"] = true;
  filter["mempool-blocks"][0]["medianFee"] = true;

  deserializeJson(doc, (char *)event_data->data_ptr, DeserializationOption::Filter(filter));

  // if (error) {
  //   Serial.print("deserializeJson() failed: ");
  //   Serial.println(error.c_str());
  //   return;
  // }

  if (doc.containsKey("block"))
  {
    JsonObject block = doc["block"];

    if (block["height"].as<uint>() == currentBlockHeight) {
      return;
    }

    processNewBlock(block["height"].as<uint>());
  }
  else if (doc.containsKey("mempool-blocks"))
  {
    JsonArray blockInfo = doc["mempool-blocks"].as<JsonArray>();

    uint medianFee = (uint)round(blockInfo[0]["medianFee"].as<double>());

    processNewBlockFee(medianFee);
  }

  doc.clear();
}

void processNewBlock(uint newBlockHeight) {
  if (newBlockHeight < currentBlockHeight)
    return;

  currentBlockHeight = newBlockHeight;

    // Serial.printf("New block found: %d\r\n", block["height"].as<uint>());
    preferences.putUInt("blockHeight", currentBlockHeight);
    lastBlockUpdate = esp_timer_get_time() / 1000000;

    if (workQueue != nullptr)
    {
      WorkItem blockUpdate = {TASK_BLOCK_UPDATE, 0};
      xQueueSend(workQueue, &blockUpdate, portMAX_DELAY);
      // xTaskNotifyGive(blockUpdateTaskHandle);

      if (getCurrentScreen() != SCREEN_BLOCK_HEIGHT &&
          preferences.getBool("stealFocus", DEFAULT_STEAL_FOCUS))
      {
        uint64_t timerPeriod = 0;
        if (isTimerActive())
        {
          // store timer periode before making inactive to prevent artifacts
          timerPeriod = getTimerSeconds();
          esp_timer_stop(screenRotateTimer);
        }
        setCurrentScreen(SCREEN_BLOCK_HEIGHT);
        if (timerPeriod > 0)
        {
          esp_timer_start_periodic(screenRotateTimer,
                                   timerPeriod * usPerSecond);
        }
        vTaskDelay(pdMS_TO_TICKS(315*NUM_SCREENS)); // Extra delay because of screen switching
      }

      if (preferences.getBool("ledFlashOnUpd", DEFAULT_LED_FLASH_ON_UPD))
      {
        vTaskDelay(pdMS_TO_TICKS(250)); // Wait until screens are updated
        queueLedEffect(LED_FLASH_BLOCK_NOTIFY);
      }
    }
}

void processNewBlockFee(uint newBlockFee) {
  if (blockMedianFee == newBlockFee)
    {
      return;
    }

    //  Serial.printf("New median fee: %d\r\n", medianFee);
    blockMedianFee = newBlockFee;

    if (workQueue != nullptr)
    {
      WorkItem blockUpdate = {TASK_FEE_UPDATE, 0};
      xQueueSend(workQueue, &blockUpdate, portMAX_DELAY);
    }
}

uint getBlockHeight() { return currentBlockHeight; }

void setBlockHeight(uint newBlockHeight)
{
  currentBlockHeight = newBlockHeight;
}

uint getBlockMedianFee() { return blockMedianFee; }

void setBlockMedianFee(uint newBlockMedianFee)
{
  blockMedianFee = newBlockMedianFee;
}

bool isBlockNotifyConnected()
{
  if (blockNotifyClient == NULL)
    return false;
  return esp_websocket_client_is_connected(blockNotifyClient);
}

bool getBlockNotifyInit()
{
  return blockNotifyInit;
}

void stopBlockNotify()
{
  if (blockNotifyClient == NULL)
    return;

  esp_websocket_client_close(blockNotifyClient, pdMS_TO_TICKS(5000));
  esp_websocket_client_stop(blockNotifyClient);
  esp_websocket_client_destroy(blockNotifyClient);

  blockNotifyClient = NULL;
}

void restartBlockNotify()
{
  stopBlockNotify();
  
  if (blockNotifyClient == NULL) {
    setupBlockNotify();
    return;
  }

  // esp_websocket_client_close(blockNotifyClient, pdMS_TO_TICKS(5000));
  // esp_websocket_client_stop(blockNotifyClient);
  // esp_websocket_client_start(blockNotifyClient);
}


int getBlockFetch()
{
  try {
    WiFiClientSecure client;

    if (preferences.getBool("mempoolSecure", DEFAULT_MEMPOOL_SECURE)) {
      client.setCACert(mempoolWsCert);
    }

    String mempoolInstance =
        preferences.getString("mempoolInstance", DEFAULT_MEMPOOL_INSTANCE);

    // Get current block height through regular API
    HTTPClient http;

    const String protocol = preferences.getBool("mempoolSecure", DEFAULT_MEMPOOL_SECURE) ? "https" : "http";

    if (preferences.getBool("mempoolSecure", DEFAULT_MEMPOOL_SECURE))
      http.begin(client, protocol + "://" + mempoolInstance + "/api/blocks/tip/height");
    else
      http.begin(protocol + "://" + mempoolInstance + "/api/blocks/tip/height");

    Serial.println("Fetching block height from " + protocol + "://" + mempoolInstance + "/api/blocks/tip/height");
    int httpCode = http.GET();

    if (httpCode > 0 && httpCode == HTTP_CODE_OK)
    {
      String blockHeightStr = http.getString();
      return blockHeightStr.toInt();
    } else {
      Serial.println("HTTP code" + String(httpCode));
      return 0;
    }
  }
  catch (...) {
    Serial.println(F("An exception occured while trying to get the latest block"));
  }

  return 2203; // B-T-C
}

uint getLastBlockUpdate()
{
  return lastBlockUpdate;
}

void setLastBlockUpdate(uint lastUpdate)
{
  lastBlockUpdate = lastUpdate;
}