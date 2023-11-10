#pragma once

#include <esp_timer.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "shared.hpp"
#include "lib/epd.hpp"

extern TaskHandle_t priceUpdateTaskHandle;
extern TaskHandle_t blockUpdateTaskHandle;
extern TaskHandle_t timeUpdateTaskHandle;
extern TaskHandle_t taskScreenRotateTaskHandle;

uint getCurrentScreen();
void setCurrentScreen(uint newScreen);
void nextScreen();
void previousScreen();

void showSystemStatusScreen();

void setupTimeUpdateTimer(void *pvParameters);
void setupScreenRotateTimer(void *pvParameters);

void IRAM_ATTR minuteTimerISR(void* arg);
void IRAM_ATTR screenRotateTimerISR(void* arg);

void taskPriceUpdate(void *pvParameters);
void taskBlockUpdate(void *pvParameters);
void taskTimeUpdate(void *pvParameters);
void taskScreenRotate(void *pvParameters);

uint getTimerSeconds();
bool isTimerActive();
void setTimerActive(bool status);
void toggleTimerActive();

void setupTasks();
