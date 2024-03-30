#include "block_notify.hpp"

char *wsServer;
esp_websocket_client_handle_t blockNotifyClient = NULL;
uint currentBlockHeight = 816000;
uint blockMedianFee = 1;
bool blockNotifyInit = false;
unsigned long int lastBlockUpdate;

// const char *mempoolWsCert = R"(-----BEGIN CERTIFICATE-----
// MIIHfTCCBmWgAwIBAgIRANFX3mhqRYDt1NFuENoSyaAwDQYJKoZIhvcNAQELBQAw
// gZUxCzAJBgNVBAYTAkdCMRswGQYDVQQIExJHcmVhdGVyIE1hbmNoZXN0ZXIxEDAO
// BgNVBAcTB1NhbGZvcmQxGDAWBgNVBAoTD1NlY3RpZ28gTGltaXRlZDE9MDsGA1UE
// AxM0U2VjdGlnbyBSU0EgT3JnYW5pemF0aW9uIFZhbGlkYXRpb24gU2VjdXJlIFNl
// cnZlciBDQTAeFw0yMzA3MjQwMDAwMDBaFw0yNDA4MjIyMzU5NTlaMFcxCzAJBgNV
// BAYTAkpQMQ4wDAYDVQQIEwVUb2t5bzEgMB4GA1UEChMXTUVNUE9PTCBTUEFDRSBD
// Ty4sIExURC4xFjAUBgNVBAMTDW1lbXBvb2wuc3BhY2UwggEiMA0GCSqGSIb3DQEB
// AQUAA4IBDwAwggEKAoIBAQCqmiPRWgo58d25R0biQjAksXMq5ciH7z7ZQo2w2AbB
// rHxpnlIry74b9S4wRY5UJeYmd6ZwA76NdSioDvxTJc29bLplY+Ftmfc4ET0zYb2k
// Fi86z7GOWb6Ezor/qez9uMM9cxd021Bvcs0/2OrL6Sgp66u9keDZv9NyvFPpXfuR
// tdV2r4HF57VJqZn105PN4k80kNWgDbae8aw+BuUNvQYKEe71yfB7Bh6zSh9pCSfM
// I6pIJdQzoada2uY1dQMoJeIq8qKNKqAPKGsH5McemUT5ZIKU/tjk3nfX0pz/sQa4
// CN7tLH6UeUlctei92GFd6Xtn7RbKLhDUbc4Sq02Cc9iXAgMBAAGjggQDMIID/zAf
// BgNVHSMEGDAWgBQX2dYlJ2f5McJJQ9kwNkSMbKlP6zAdBgNVHQ4EFgQUXkxoddJ6
// rKobsbmDdtuCK1ywXuIwDgYDVR0PAQH/BAQDAgWgMAwGA1UdEwEB/wQCMAAwHQYD
// VR0lBBYwFAYIKwYBBQUHAwEGCCsGAQUFBwMCMEoGA1UdIARDMEEwNQYMKwYBBAGy
// MQECAQMEMCUwIwYIKwYBBQUHAgEWF2h0dHBzOi8vc2VjdGlnby5jb20vQ1BTMAgG
// BmeBDAECAjBaBgNVHR8EUzBRME+gTaBLhklodHRwOi8vY3JsLnNlY3RpZ28uY29t
// L1NlY3RpZ29SU0FPcmdhbml6YXRpb25WYWxpZGF0aW9uU2VjdXJlU2VydmVyQ0Eu
// Y3JsMIGKBggrBgEFBQcBAQR+MHwwVQYIKwYBBQUHMAKGSWh0dHA6Ly9jcnQuc2Vj
// dGlnby5jb20vU2VjdGlnb1JTQU9yZ2FuaXphdGlvblZhbGlkYXRpb25TZWN1cmVT
// ZXJ2ZXJDQS5jcnQwIwYIKwYBBQUHMAGGF2h0dHA6Ly9vY3NwLnNlY3RpZ28uY29t
// MIIBgAYKKwYBBAHWeQIEAgSCAXAEggFsAWoAdwB2/4g/Crb7lVHCYcz1h7o0tKTN
// uyncaEIKn+ZnTFo6dAAAAYmc9m/gAAAEAwBIMEYCIQD8XOozx411S/bnZambGjTB
// yTcr2fCmggUfQLSmqksD5gIhAIjiEMg0o1VSuQW31gWzfzL6idCkIZeSKN104cdp
// xa4SAHcA2ra/az+1tiKfm8K7XGvocJFxbLtRhIU0vaQ9MEjX+6sAAAGJnPZwPwAA
// BAMASDBGAiEA2sPTZTzvxewzQ8vk36+BWAKuJS7AvJ5W3clvfwCa8OUCIQC74ekT
// Ged2fqQE4sVy74aS6HRA2ihC9VLtNrASJx1YjQB2AO7N0GTV2xrOxVy3nbTNE6Iy
// h0Z8vOzew1FIWUZxH7WbAAABiZz2cA8AAAQDAEcwRQIgEklH7wYCFuuJIFUHX5PY
// /vZ3bDoxOp+061PT3caa+rICIQC0abgfGlBKiHxp47JZxnW3wcVqWdiYX4ViLm9H
// xfx4ljCBxgYDVR0RBIG+MIG7gg1tZW1wb29sLnNwYWNlghMqLmZtdC5tZW1wb29s
// LnNwYWNlghMqLmZyYS5tZW1wb29sLnNwYWNlgg8qLm1lbXBvb2wuc3BhY2WCEyou
// dGs3Lm1lbXBvb2wuc3BhY2WCEyoudmExLm1lbXBvb2wuc3BhY2WCDGJpc3EubWFy
// a2V0c4IKYmlzcS5uaW5qYYIObGlxdWlkLm5ldHdvcmuCDGxpcXVpZC5wbGFjZYIN
// bWVtcG9vbC5uaW5qYTANBgkqhkiG9w0BAQsFAAOCAQEAFvOSRnlHDfq9C8acjZEG
// 5XIqjNYigyWyjOvx83of6Z3PBKkAZB5D/UHBPp+jBDJiEb/QXC7Z7Y7kpuvnoVib
// b4jDc0RjGEsxL+3F7cSw26m3wILJhhHooGZRmFY4GOAeCZtYCOTzJsiZvFpDoQjU
// hTBxtaps05z0Ly9/eYvkXnjnBNROZJVR+KYHlq4TIoGNc4q4KvpfHv2I/vhS2M1e
// bECNNPEyRxHGKdXXO3huocE7aVKpy+JDR6cWwDu6hpdc1j/SCDqdTDFQ7McHOrqA
// fpPh4FcfePMh7Mqxtg2pSs5pXPtiP0ZjLgxd7HbAXct8Y+/jGk+k3sx3SeYXVimr
// ew==
// -----END CERTIFICATE-----)";

void setupBlockNotify()
{
  IPAddress result;

  int dnsErr = -1;
  String mempoolInstance =
      preferences.getString("mempoolInstance", DEFAULT_MEMPOOL_INSTANCE);

  while (dnsErr != 1)
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

  currentBlockHeight = getBlockFetch();

  if (currentBlockHeight != -1)
  {
    lastBlockUpdate = esp_timer_get_time() / 1000000;
  }

  if (workQueue != nullptr)
  {
    WorkItem blockUpdate = {TASK_BLOCK_UPDATE, 0};
    xQueueSend(workQueue, &blockUpdate, portMAX_DELAY);
  }

  // std::strcpy(wsServer, String("wss://" + mempoolInstance +
  // "/api/v1/ws").c_str());

  esp_websocket_client_config_t config = {
      .uri = "wss://mempool.space/api/v1/ws",
      //  .task_stack = (6*1024),
      // .cert_pem = mempoolWsCert,
      .user_agent = USER_AGENT,
  };

  blockNotifyClient = esp_websocket_client_init(&config);
  esp_websocket_register_events(blockNotifyClient, WEBSOCKET_EVENT_ANY,
                                onWebsocketEvent, blockNotifyClient);
  esp_websocket_client_start(blockNotifyClient);
}

void onWebsocketEvent(void *handler_args, esp_event_base_t base,
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
    onWebsocketMessage(data);
    break;
  case WEBSOCKET_EVENT_ERROR:
    Serial.println(F("Mempool.space WS Connnection error"));
    break;
  case WEBSOCKET_EVENT_DISCONNECTED:
    Serial.println(F("Mempool.space WS Connnection Closed"));
    break;
  }
}

void onWebsocketMessage(esp_websocket_event_data_t *event_data)
{
  JsonDocument doc;

  JsonDocument filter;
  filter["block"]["height"] = true;
  filter["mempool-blocks"][0]["medianFee"] = true;

  DeserializationError error = deserializeJson(doc, (char *)event_data->data_ptr, DeserializationOption::Filter(filter));

  // if (error) {
  //   Serial.print("deserializeJson() failed: ");
  //   Serial.println(error.c_str());
  //   return;
  // }

  if (doc.containsKey("block"))
  {
    JsonObject block = doc["block"];

    currentBlockHeight = block["height"].as<uint>();

    // Serial.printf("New block found: %d\r\n", block["height"].as<uint>());
    preferences.putUInt("blockHeight", currentBlockHeight);
    lastBlockUpdate = esp_timer_get_time() / 1000000;

    if (workQueue != nullptr)
    {
      WorkItem blockUpdate = {TASK_BLOCK_UPDATE, 0};
      xQueueSend(workQueue, &blockUpdate, portMAX_DELAY);
      // xTaskNotifyGive(blockUpdateTaskHandle);

      if (getCurrentScreen() != SCREEN_BLOCK_HEIGHT &&
          preferences.getBool("stealFocus", true))
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
      }

      if (preferences.getBool("ledFlashOnUpd", false))
      {
        vTaskDelay(pdMS_TO_TICKS(250)); // Wait until screens are updated
        queueLedEffect(LED_FLASH_BLOCK_NOTIFY);
      }
    }
  }
  else if (doc.containsKey("mempool-blocks"))
  {
    JsonArray blockInfo = doc["mempool-blocks"].as<JsonArray>();

    uint medianFee = (uint)round(blockInfo[0]["medianFee"].as<double>());

    if (blockMedianFee == medianFee)
    {
      doc.clear();
      return;
    }

    //  Serial.printf("New median fee: %d\r\n", medianFee);
    blockMedianFee = medianFee;

    if (workQueue != nullptr)
    {
      WorkItem blockUpdate = {TASK_FEE_UPDATE, 0};
      xQueueSend(workQueue, &blockUpdate, portMAX_DELAY);
    }
  }

  doc.clear();
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

  esp_websocket_client_close(blockNotifyClient, portMAX_DELAY);
  esp_websocket_client_stop(blockNotifyClient);
  esp_websocket_client_destroy(blockNotifyClient);

  blockNotifyClient = NULL;
}

int getBlockFetch()
{
  String mempoolInstance =
      preferences.getString("mempoolInstance", DEFAULT_MEMPOOL_INSTANCE);

  // Get current block height through regular API
  HTTPClient *http = new HTTPClient();
  http->begin("https://" + mempoolInstance + "/api/blocks/tip/height");
  int httpCode = http->GET();

  if (httpCode > 0 && httpCode == HTTP_CODE_OK)
  {
    String blockHeightStr = http->getString();
    return blockHeightStr.toInt();
  }

  return -1;
}

uint getLastBlockUpdate()
{
  return lastBlockUpdate;
}

void setLastBlockUpdate(uint lastUpdate)
{
  lastBlockUpdate = lastUpdate;
}