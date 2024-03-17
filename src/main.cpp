/*
 * Copyright 2023 Djuri Baars
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "Arduino.h"
#include <WiFiManager.h>
#define WEBSERVER_H
#include "ESPAsyncWebServer.h"
#include "lib/config.hpp"

uint wifiLostConnection;
uint priceNotifyLostConnection = 0;
uint blockNotifyLostConnection = 0;

extern "C" void app_main() {
  initArduino();

  Serial.begin(115200);
  setup();

  while (true) {
    // vTaskList(ptrTaskList);
    // Serial.println(F("**********************************"));
    // Serial.println(F("Task  State   Prio    Stack    Num"));
    // Serial.println(F("**********************************"));
    // Serial.print(ptrTaskList);
    // Serial.println(F("**********************************"));
    if (eventSourceTaskHandle != NULL)
      xTaskNotifyGive(eventSourceTaskHandle);

    int64_t currentUptime = esp_timer_get_time() / 1000000;;

    if (!WiFi.isConnected()) {
      if (!wifiLostConnection) {
        wifiLostConnection = currentUptime;
        Serial.println("Lost WiFi connection, trying to reconnect...");
      }

      if ((currentUptime - wifiLostConnection) > 600) {
        Serial.println("Still no connection after 10 minutes, restarting...");
        delay(2000);
        ESP.restart();
      }

      WiFi.begin();
    } else if (wifiLostConnection) {
      wifiLostConnection = 0;
      Serial.println("Connection restored, reset timer.");
    } else if (getPriceNotifyInit() && !preferences.getBool("fetchEurPrice", false) && !isPriceNotifyConnected()) {
      priceNotifyLostConnection++;
      Serial.println("Lost price data connection...");
      queueLedEffect(LED_DATA_PRICE_ERROR);

      // if price WS connection does not come back after 6*5 seconds, destroy and recreate
      if (priceNotifyLostConnection > 6) {
        Serial.println("Restarting price handler...");

        stopPriceNotify();
        setupPriceNotify();
        priceNotifyLostConnection = 0;
      }
    } else if (getBlockNotifyInit() && !isBlockNotifyConnected()) {
      blockNotifyLostConnection++;
      Serial.println("Lost block data connection...");
      queueLedEffect(LED_DATA_BLOCK_ERROR);
      // if mempool WS connection does not come back after 6*5 seconds, destroy and recreate
      if (blockNotifyLostConnection > 6) {
        Serial.println("Restarting block handler...");

        stopBlockNotify();
        setupBlockNotify();
        blockNotifyLostConnection = 0;
      }
    } else if (blockNotifyLostConnection > 0 || priceNotifyLostConnection > 0) {
      blockNotifyLostConnection = 0;
      priceNotifyLostConnection = 0;
    }

    // if more than 5 price updates are missed, there is probably something wrong, reconnect
    if ((getLastPriceUpdate() - currentUptime) > (preferences.getUInt("minSecPriceUpd", DEFAULT_SECONDS_BETWEEN_PRICE_UPDATE)*5)) {
        stopPriceNotify();
        setupPriceNotify();

        priceNotifyLostConnection = 0;
    }

    vTaskDelay(pdMS_TO_TICKS(5000));
  }
}