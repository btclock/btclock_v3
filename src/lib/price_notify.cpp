#include "price_notify.hpp"

const char *wsOwnServerPrice = "wss://ws.btclock.dev/ws?assets=bitcoin";
const char *wsServerPrice = "wss://ws.coincap.io/prices?assets=bitcoin";

// const char* coinCapWsCert = R"(-----BEGIN CERTIFICATE-----
// MIIFMjCCBNmgAwIBAgIQBtgXvFyc28MsvQ1HjCnXJTAKBggqhkjOPQQDAjBKMQsw
// CQYDVQQGEwJVUzEZMBcGA1UEChMQQ2xvdWRmbGFyZSwgSW5jLjEgMB4GA1UEAxMX
// Q2xvdWRmbGFyZSBJbmMgRUNDIENBLTMwHhcNMjMwNTEwMDAwMDAwWhcNMjQwNTA5
// MjM1OTU5WjB1MQswCQYDVQQGEwJVUzETMBEGA1UECBMKQ2FsaWZvcm5pYTEWMBQG
// A1UEBxMNU2FuIEZyYW5jaXNjbzEZMBcGA1UEChMQQ2xvdWRmbGFyZSwgSW5jLjEe
// MBwGA1UEAxMVc25pLmNsb3VkZmxhcmVzc2wuY29tMFkwEwYHKoZIzj0CAQYIKoZI
// zj0DAQcDQgAEpvFIXzQKHuqTo+IE6c6sB4p0PMXK1KsseEGf2UN/CNRhG5hO7lr8
// JtXrPZkawWBysZxOsEoetkPrDHMugCLfXKOCA3QwggNwMB8GA1UdIwQYMBaAFKXO
// N+rrsHUOlGeItEX62SQQh5YfMB0GA1UdDgQWBBShsZDJohaR1a5E0Qj7yblZjKDC
// gDA6BgNVHREEMzAxggwqLmNvaW5jYXAuaW+CCmNvaW5jYXAuaW+CFXNuaS5jbG91
// ZGZsYXJlc3NsLmNvbTAOBgNVHQ8BAf8EBAMCB4AwHQYDVR0lBBYwFAYIKwYBBQUH
// AwEGCCsGAQUFBwMCMHsGA1UdHwR0MHIwN6A1oDOGMWh0dHA6Ly9jcmwzLmRpZ2lj
// ZXJ0LmNvbS9DbG91ZGZsYXJlSW5jRUNDQ0EtMy5jcmwwN6A1oDOGMWh0dHA6Ly9j
// cmw0LmRpZ2ljZXJ0LmNvbS9DbG91ZGZsYXJlSW5jRUNDQ0EtMy5jcmwwPgYDVR0g
// BDcwNTAzBgZngQwBAgIwKTAnBggrBgEFBQcCARYbaHR0cDovL3d3dy5kaWdpY2Vy
// dC5jb20vQ1BTMHYGCCsGAQUFBwEBBGowaDAkBggrBgEFBQcwAYYYaHR0cDovL29j
// c3AuZGlnaWNlcnQuY29tMEAGCCsGAQUFBzAChjRodHRwOi8vY2FjZXJ0cy5kaWdp
// Y2VydC5jb20vQ2xvdWRmbGFyZUluY0VDQ0NBLTMuY3J0MAwGA1UdEwEB/wQCMAAw
// ggF+BgorBgEEAdZ5AgQCBIIBbgSCAWoBaAB1AO7N0GTV2xrOxVy3nbTNE6Iyh0Z8
// vOzew1FIWUZxH7WbAAABiAPnoRAAAAQDAEYwRAIgAP2W09OozuhmKeKKMsaVBcae
// o+nPHF1WUWk0i387YYYCIDIM1Wll7/4O3GNx2/Fx9bC6pi69Uya4pLxsCfW3fZMe
// AHYASLDja9qmRzQP5WoC+p0w6xxSActW3SyB2bu/qznYhHMAAAGIA+eg+QAABAMA
// RzBFAiEAuNpSqrbx47gYBgBMz5M6q0CnV/WMJqWQOxYFKrwfwVACIH3nCs4bKToT
// e+MiBrqSDaekixk4kPFEQESO9qHCkWY5AHcA2ra/az+1tiKfm8K7XGvocJFxbLtR
// hIU0vaQ9MEjX+6sAAAGIA+eg1gAABAMASDBGAiEAolCFl2IfbOHUPAOxoi4BLclS
// v9FVXb7LwIvTuCfyrEQCIQDcvehwhV9XGopKGl17F2LYYKI7hvlO3RmpPZQJt1da
// MDAKBggqhkjOPQQDAgNHADBEAiAXRWZ/JVMsfpSFFTHQHUSqRnQ/7cCOWx+9svIy
// mYnFZQIgHMEG0Cm7O4cn5KUzKOsTwwK+2U15s/jPUQi2n2IDTEM=
// -----END CERTIFICATE-----)";

// WebsocketsClient client;
esp_websocket_client_handle_t clientPrice = NULL;
esp_websocket_client_config_t config;
uint currentPrice = 50000;
unsigned long int lastPriceUpdate;
bool priceNotifyInit = false;

void setupPriceNotify()
{
  if (preferences.getBool("ownDataSource", DEFAULT_OWN_DATA_SOURCE))
  {
    config = {.uri = wsOwnServerPrice,
              .user_agent = USER_AGENT};
  }
  else
  {
    config = {.uri = wsServerPrice,
              .user_agent = USER_AGENT};
  }

  clientPrice = esp_websocket_client_init(&config);
  esp_websocket_register_events(clientPrice, WEBSOCKET_EVENT_ANY,
                                onWebsocketPriceEvent, clientPrice);
  esp_websocket_client_start(clientPrice);
}

void onWebsocketPriceEvent(void *handler_args, esp_event_base_t base,
                           int32_t event_id, void *event_data)
{
  esp_websocket_event_data_t *data = (esp_websocket_event_data_t *)event_data;

  switch (event_id)
  {
  case WEBSOCKET_EVENT_CONNECTED:
    Serial.println("Connected to " + String(config.uri) + " WebSocket");
    priceNotifyInit = true;

    break;
  case WEBSOCKET_EVENT_DATA:
    onWebsocketPriceMessage(data);
    if (preferences.getBool("ownDataSource", DEFAULT_OWN_DATA_SOURCE))
    {
      onWebsocketBlockMessage(data);
    }
    break;
  case WEBSOCKET_EVENT_ERROR:
    Serial.println(F("Price WS Connnection error"));
    break;
  case WEBSOCKET_EVENT_DISCONNECTED:
    Serial.println(F("Price WS Connnection Closed"));
    break;
  }
}

void onWebsocketPriceMessage(esp_websocket_event_data_t *event_data)
{
  JsonDocument doc;

  deserializeJson(doc, (char *)event_data->data_ptr);

  if (doc.containsKey("bitcoin"))
  {
    if (currentPrice != doc["bitcoin"].as<long>())
    {
      uint minSecPriceUpd = preferences.getUInt(
          "minSecPriceUpd", DEFAULT_SECONDS_BETWEEN_PRICE_UPDATE);
      uint currentTime = esp_timer_get_time() / 1000000;

      if (lastPriceUpdate == 0 ||
          (currentTime - lastPriceUpdate) > minSecPriceUpd)
      {
        //   const unsigned long oldPrice = currentPrice;
        currentPrice = doc["bitcoin"].as<uint>();
        preferences.putUInt("lastPrice", currentPrice);
        lastPriceUpdate = currentTime;
        // if (abs((int)(oldPrice-currentPrice)) > round(0.0015*oldPrice)) {
        if (workQueue != nullptr && (getCurrentScreen() == SCREEN_BTC_TICKER ||
                                     getCurrentScreen() == SCREEN_MSCW_TIME ||
                                     getCurrentScreen() == SCREEN_MARKET_CAP))
        {
          WorkItem priceUpdate = {TASK_PRICE_UPDATE, 0};
          xQueueSend(workQueue, &priceUpdate, portMAX_DELAY);
        }
        //}
      }
    }
  }
}

uint getLastPriceUpdate()
{
  return lastPriceUpdate;
}

uint getPrice() { return currentPrice; }

void setPrice(uint newPrice) { currentPrice = newPrice; }

bool isPriceNotifyConnected()
{
  if (clientPrice == NULL)
    return false;
  return esp_websocket_client_is_connected(clientPrice);
}

bool getPriceNotifyInit()
{
  return priceNotifyInit;
}

void stopPriceNotify()
{
  if (clientPrice == NULL)
    return;
  esp_websocket_client_close(clientPrice, pdMS_TO_TICKS(5000));
  esp_websocket_client_stop(clientPrice);
  esp_websocket_client_destroy(clientPrice);

  clientPrice = NULL;
}

void restartPriceNotify()
{
  stopPriceNotify();
  if (clientPrice == NULL)
  {
    setupPriceNotify();
    return;
  }
  // esp_websocket_client_close(clientPrice, pdMS_TO_TICKS(5000));
  // esp_websocket_client_stop(clientPrice);
  // esp_websocket_client_start(clientPrice);
}