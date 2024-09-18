#pragma once

#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <data_handler.hpp>
#include <bitaxe_handler.hpp>

#include "lib/epd.hpp"
#include "lib/shared.hpp"

// extern TaskHandle_t priceUpdateTaskHandle;
// extern TaskHandle_t blockUpdateTaskHandle;
// extern TaskHandle_t timeUpdateTaskHandle;
extern TaskHandle_t workerTaskHandle;
extern TaskHandle_t taskScreenRotateTaskHandle;

extern QueueHandle_t workQueue;

typedef enum {
  TASK_PRICE_UPDATE,
  TASK_BLOCK_UPDATE,
  TASK_FEE_UPDATE,
  TASK_TIME_UPDATE,
  TASK_BITAXE_UPDATE
} TaskType;

typedef struct {
  TaskType type;
  char data;
} WorkItem;

void workerTask(void *pvParameters);
uint getCurrentScreen();
void setCurrentScreen(uint newScreen);
void nextScreen();
void previousScreen();

void showSystemStatusScreen();



// void taskPriceUpdate(void *pvParameters);
// void taskBlockUpdate(void *pvParameters);
// void taskTimeUpdate(void *pvParameters);
void taskScreenRotate(void *pvParameters);



void setupTasks();
void setCurrentCurrency(char currency);

uint getCurrentCurrency();