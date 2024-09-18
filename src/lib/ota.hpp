#pragma once

#include <Arduino.h>
#include <ArduinoOTA.h>

#include "lib/config.hpp"
#include "lib/shared.hpp"
#include "lib/timers.hpp"

#ifndef UPDATE_MESSAGE_HPP
#define UPDATE_MESSAGE_HPP
typedef struct {
    char updateType;
} UpdateMessage;
#endif 

extern QueueHandle_t otaQueue;

struct ReleaseInfo {
  String fileUrl;
  String checksumUrl;
};

void setupOTA();
void onOTAStart();
void handleOTATask(void *parameter);
void onOTAProgress(unsigned int progress, unsigned int total);
// void downloadUpdate();
void onOTAError(ota_error_t error);
void onOTAComplete();
int downloadUpdateHandler(char updateType);
ReleaseInfo getLatestRelease(const String& fileToDownload);

bool getIsOTAUpdating();

void updateWebUi(String latestRelease, int command);
String downloadSHA256(const String& filename);

