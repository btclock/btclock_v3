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

    if (!WiFi.isConnected()) {
      if (!wifiLostConnection) {
        wifiLostConnection = esp_timer_get_time() / 1000000;
        Serial.println("Lost WiFi connection, trying to reconnect...");
      }

      if (((esp_timer_get_time() / 1000000) - wifiLostConnection) > 600) {
        Serial.println("Still no connection after 10 minutes, restarting...");
        delay(2000);
        ESP.restart();
      }

      WiFi.begin();
    } else if (wifiLostConnection) {
      wifiLostConnection = 0;
      Serial.println("Connection restored, reset timer.");
    }

    vTaskDelay(pdMS_TO_TICKS(5000));
  }
}