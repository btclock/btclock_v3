#pragma once

#include <Arduino.h>
#include <ArduinoOTA.h>

#include "lib/config.hpp"
#include "lib/shared.hpp"

#ifndef UPDATE_MESSAGE_HPP
#define UPDATE_MESSAGE_HPP
typedef struct {
    char updateType;
} UpdateMessage;
#endif 

extern QueueHandle_t otaQueue;

void setupOTA();
void onOTAStart();
void handleOTATask(void *parameter);
void onOTAProgress(unsigned int progress, unsigned int total);
// void downloadUpdate();
void onOTAError(ota_error_t error);
void onOTAComplete();
int downloadUpdateHandler(char updateType);
String getLatestRelease(const String& fileToDownload);

bool getIsOTAUpdating();

void updateWebUi(String latestRelease, int command);
String downloadSHA256(const String& filename);