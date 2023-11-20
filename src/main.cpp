#include "Arduino.h"
#include <WiFiManager.h>
#define WEBSERVER_H
#include "ESPAsyncWebServer.h" 

#include "lib/config.hpp"
#define USE_QR

//char ptrTaskList[400];

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
            WiFi.begin();
        }

        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}