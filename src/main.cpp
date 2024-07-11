/*
 * Copyright 2023-2024 Djuri Baars
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
// char ptrTaskList[1500];

extern "C" void app_main()
{
  initArduino();

  Serial.begin(115200);
  setup();

  while (true)
  {
    // vTaskList(ptrTaskList);
    // Serial.println(F("**********************************"));
    // Serial.println(F("Task  State   Prio    Stack    Num"));
    // Serial.println(F("**********************************"));
    // Serial.print(ptrTaskList);
    // Serial.println(F("**********************************"));
    if (eventSourceTaskHandle != NULL)
      xTaskNotifyGive(eventSourceTaskHandle);

    int64_t currentUptime = esp_timer_get_time() / 1000000;
    ;

    if (!getIsOTAUpdating())
    {
#ifdef HAS_FRONTLIGHT
      if (preferences.getUInt("luxLightToggle", DEFAULT_LUX_LIGHT_TOGGLE) != 0)
      {
        if (hasLightLevel() && getLightLevel() == 0)
        {
          if (frontlightIsOn()) {
            frontlightFadeOutAll();
          }
        }
        else if (hasLightLevel() && getLightLevel() < preferences.getUInt("luxLightToggle", DEFAULT_LUX_LIGHT_TOGGLE) && !frontlightIsOn())
        {
          frontlightFadeInAll();
        }
        else if (frontlightIsOn() && getLightLevel() > preferences.getUInt("luxLightToggle", DEFAULT_LUX_LIGHT_TOGGLE))
        {
          frontlightFadeOutAll();
        }
      }

#endif

      if (!WiFi.isConnected())
      {
        if (!wifiLostConnection)
        {
          wifiLostConnection = currentUptime;
          Serial.println(F("Lost WiFi connection, trying to reconnect..."));
        }

        if ((currentUptime - wifiLostConnection) > 600)
        {
          Serial.println(F("Still no connection after 10 minutes, restarting..."));
          delay(2000);
          ESP.restart();
        }

        WiFi.begin();
      }
      else if (wifiLostConnection)
      {
        wifiLostConnection = 0;
        Serial.println(F("Connection restored, reset timer."));
      }

      if (getPriceNotifyInit() && !preferences.getBool("fetchEurPrice", DEFAULT_FETCH_EUR_PRICE) && !isPriceNotifyConnected())
      {
        priceNotifyLostConnection++;
        Serial.println(F("Lost price data connection..."));
        queueLedEffect(LED_DATA_PRICE_ERROR);

        // if price WS connection does not come back after 6*5 seconds, destroy and recreate
        if (priceNotifyLostConnection > 6)
        {
          Serial.println(F("Restarting price handler..."));

          restartPriceNotify();
          //  setupPriceNotify();
          priceNotifyLostConnection = 0;
        }
      }
      else if (priceNotifyLostConnection > 0 && isPriceNotifyConnected())
      {
        priceNotifyLostConnection = 0;
      }

      if (getBlockNotifyInit() && !isBlockNotifyConnected())
      {
        blockNotifyLostConnection++;
        Serial.println(F("Lost block data connection..."));
        queueLedEffect(LED_DATA_BLOCK_ERROR);
        // if mempool WS connection does not come back after 6*5 seconds, destroy and recreate
        if (blockNotifyLostConnection > 6)
        {
          Serial.println(F("Restarting block handler..."));

          restartBlockNotify();
          // setupBlockNotify();
          blockNotifyLostConnection = 0;
        }
      }
      else if (blockNotifyLostConnection > 0 && isBlockNotifyConnected())
      {
        blockNotifyLostConnection = 0;
      }

      // if more than 5 price updates are missed, there is probably something wrong, reconnect
      if ((getLastPriceUpdate() - currentUptime) > (preferences.getUInt("minSecPriceUpd", DEFAULT_SECONDS_BETWEEN_PRICE_UPDATE) * 5))
      {
        Serial.println(F("Detected 5 missed price updates... restarting price handler."));

        restartPriceNotify();
        // setupPriceNotify();

        priceNotifyLostConnection = 0;
      }

      // If after 45 minutes no mempool blocks, check the rest API
      if ((getLastBlockUpdate() - currentUptime) > 45 * 60)
      {
        Serial.println(F("Long time (45 min) since last block, checking if I missed anything..."));
        int currentBlock = getBlockFetch();
        if (currentBlock != -1)
        {
          if (currentBlock != getBlockHeight())
          {
            Serial.println(F("Detected stuck block height... restarting block handler."));
            // Mempool source stuck, restart
            restartBlockNotify();
            // setupBlockNotify();
          }
          // set last block update so it doesn't fetch for 45 minutes
          setLastBlockUpdate(currentUptime);
        }
      }

      if (currentUptime - getLastTimeSync() > 24 * 60 * 60)
      {
        Serial.println(F("Last time update is longer than 24 hours ago, sync again"));
        syncTime();
      };
    }

    vTaskDelay(pdMS_TO_TICKS(5000));
  }
}