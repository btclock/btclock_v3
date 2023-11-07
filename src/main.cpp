#include "Arduino.h"
#include "lib/config.hpp"

extern "C" void app_main()
{
    initArduino();

    Serial.begin(115200);
    setup();

    while (true)
    {
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}