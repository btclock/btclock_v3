#include <Arduino.h>
#include <ArduinoOTA.h>
#include "config.hpp"
#include "shared.hpp"

void setupOTA();
void onOTAStart();
void handleOTATask(void *parameter);