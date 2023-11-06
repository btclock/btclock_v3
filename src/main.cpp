#include "Arduino.h"

extern "C" void app_main()
{
    initArduino();

    Serial.begin(115200);

}