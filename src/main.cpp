#include "Arduino.h"
#include <WiFiManager.h>
#define WEBSERVER_H
#include "ESPAsyncWebServer.h" 

#include "lib/config.hpp"

//char ptrTaskList[400];

uint wifiLostConnection;

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