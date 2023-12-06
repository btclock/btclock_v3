#include "nostr_subscribe.hpp"

TaskHandle_t nostrSubscribeTaskHandle;
NostrEvent nostr;
NostrRelayManager nostrRelayManager;
NostrQueueProcessor nostrQueue;

bool hasSentEvent = false;
unsigned long int lastNostrPriceUpdate;
unsigned long int lastNostrBlockUpdate;

void okEvent(const std::string& key, const char* payload) {
  Serial.println("OK event");
  Serial.println("payload is: ");
  Serial.println(payload);
}

void nip01Event(const std::string& key, const char* payload) {
  // Serial.println("NIP01 event");
  // Serial.println("payload is: ");
  // Serial.println(payload);

  uint minSecPriceUpd = preferences.getUInt(
      "minSecPriceUpd", DEFAULT_SECONDS_BETWEEN_PRICE_UPDATE);
  uint currentTime = esp_timer_get_time() / 1000000;

  SpiRamJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, payload);

  if (doc[2].containsKey("tags")) {
    for (int i = 0; i < doc[2]["tags"].size(); i++) {
      if (doc[2]["tags"][i][0].as<String>().equals("type") &&
          doc[2]["tags"][i][1].as<String>().equals("blockHeight")) {
        uint currentBlockHeight = doc[2]["content"].as<uint>();

        if (currentBlockHeight <= getBlockHeight())
          return;

        Serial.printf("Block Height: %d\n", currentBlockHeight);
        setBlockHeight(currentBlockHeight);
        if (workQueue != nullptr) {
          WorkItem blockUpdate = {TASK_BLOCK_UPDATE, 0};
          xQueueSend(workQueue, &blockUpdate, portMAX_DELAY);
        }

        if (getCurrentScreen() != SCREEN_BLOCK_HEIGHT &&
            preferences.getBool("stealFocus", true)) {
          uint64_t timerPeriod = 0;
          if (isTimerActive()) {
            // store timer periode before making inactive to prevent artifacts
            timerPeriod = getTimerSeconds();
            esp_timer_stop(screenRotateTimer);
          }
          setCurrentScreen(SCREEN_BLOCK_HEIGHT);
          if (timerPeriod > 0) {
            esp_timer_start_periodic(screenRotateTimer,
                                     timerPeriod * usPerSecond);
          }
        }

        if (getCurrentScreen() == SCREEN_BLOCK_HEIGHT &&
            preferences.getBool("ledFlashOnUpd", false)) {
          vTaskDelay(pdMS_TO_TICKS(250));  // Wait until screens are updated
          queueLedEffect(LED_FLASH_BLOCK_NOTIFY);
        }

        preferences.putUInt("blockHeight", currentBlockHeight);
      } else if (doc[2]["tags"][i][0].as<String>().equals("type") &&
                 doc[2]["tags"][i][1].as<String>().equals("priceUsd")) {
        uint usdPrice = doc[2]["content"].as<uint>();

        if (lastNostrPriceUpdate == 0 ||
            (currentTime - lastNostrPriceUpdate) > minSecPriceUpd) {
          setPrice(usdPrice);

          if (workQueue != nullptr &&
              (getCurrentScreen() == SCREEN_BTC_TICKER ||
               getCurrentScreen() == SCREEN_MSCW_TIME ||
               getCurrentScreen() == SCREEN_MARKET_CAP)) {
            WorkItem priceUpdate = {TASK_PRICE_UPDATE, 0};
            xQueueSend(workQueue, &priceUpdate, portMAX_DELAY);
          }
          lastNostrPriceUpdate = currentTime;

          preferences.putUInt("lastPrice", usdPrice);
        }
      }
    }
  }
}

void taskNostrSubscribe(void* pvParameters) {
  std::vector<String> relays = retrieveStringVector("nostrRelays");

  nostr.setLogging(false);
  nostrRelayManager.setRelays(relays);
  nostrRelayManager.setMinRelaysAndTimeout(2, 10000);

  nostrRelayManager.setEventCallback("ok", okEvent);
  nostrRelayManager.setEventCallback(1, nip01Event);
  nostrRelayManager.connect();

  NostrRequestOptions* eventRequestOptions = new NostrRequestOptions();
  String authors[] = {
      preferences.getString("nostrBlocksAuth", DEFAULT_NOSTR_BLOCKS_AUTHOR),
      preferences.getString("nostrPriceAuth", DEFAULT_NOSTR_PRICE_AUTHOR)};
  eventRequestOptions->authors = authors;
  eventRequestOptions->authors_count = sizeof(authors) / sizeof(authors[0]);

  int kinds[] = {1};
  eventRequestOptions->kinds = kinds;
  eventRequestOptions->kinds_count = sizeof(kinds) / sizeof(kinds[0]);

  eventRequestOptions->limit = 5;
  nostrRelayManager.requestEvents(eventRequestOptions);
  while (1) {
    nostrRelayManager.loop();
    nostrRelayManager.broadcastEvents();

    vTaskDelay(100);
  }
}

void setupNostrSubscribeTask() {
  xTaskCreate(taskNostrSubscribe, "nostrSubscribe", (10 * 1024), NULL, 12,
              &nostrSubscribeTaskHandle);

  // taskNostrSubscribe(NULL);
}

void storeStringVector(const char* key, const std::vector<String>& vec) {
  String serializedVector = "";

  // Serialize the vector of strings
  for (size_t i = 0; i < vec.size(); i++) {
    serializedVector += vec[i];
    if (i < vec.size() - 1) {
      serializedVector += ",";
    }
  }

  // Store the serialized vector as a string
  preferences.putString(key, serializedVector);
}

std::vector<String> retrieveStringVector(const char* key) {
  String serializedVector = preferences.getString(key, DEFAULT_NOSTR_RELAYS);
  std::vector<String> result;

  // Deserialize the string into a vector of strings
  size_t startPos = 0;
  size_t commaPos = serializedVector.indexOf(",");
  while (commaPos != -1) {
    result.push_back(serializedVector.substring(startPos, commaPos));
    startPos = commaPos + 1;
    commaPos = serializedVector.indexOf(",", startPos);
  }

  // Add the last element
  result.push_back(serializedVector.substring(startPos));

  return result;
}