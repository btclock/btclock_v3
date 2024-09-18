#pragma once

#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "lib/shared.hpp"
#include "lib/screen_handler.hpp"

extern esp_timer_handle_t screenRotateTimer;
extern esp_timer_handle_t minuteTimer;

void setupTimeUpdateTimer(void *pvParameters);
void setupScreenRotateTimer(void *pvParameters);

void IRAM_ATTR minuteTimerISR(void *arg);
void IRAM_ATTR screenRotateTimerISR(void *arg);

uint getTimerSeconds();
bool isTimerActive();
void setTimerActive(bool status);
void toggleTimerActive();