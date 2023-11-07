#include "Arduino.h"
#include "lib/config.hpp"


extern "C" void app_main()
{
    initArduino();

    setup();
    Serial.begin(115200);
    static char sBuffer[240];

    while (true)
    {
        // Serial.println("-------");
        // vTaskGetRunTimeStats((char *)sBuffer);
        // Serial.println(sBuffer);
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}