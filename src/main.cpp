#include "Arduino.h"
#include "lib/config.hpp"

extern "C" void app_main()
{
    initArduino();

    Serial.begin(115200);
    setup();

    while (true)
    {
        eventSourceLoop();
        vTaskDelay(pdMS_TO_TICKS(2500));
    }
}