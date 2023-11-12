#include <Arduino.h>
#include <ArduinoOTA.h>
#include "config.hpp"
#include "shared.hpp"

void setupOTA();
void onOTAStart();
void handleOTATask(void *parameter);
void onOTAProgress(unsigned int progress, unsigned int total);
void downloadUpdate();
void onOTAComplete();