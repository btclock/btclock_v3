#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <Adafruit_MCP23X17.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <mutex>

#include "utils.hpp"

extern Adafruit_MCP23X17 mcp1;
#ifdef IS_BTCLOCK_S3
extern Adafruit_MCP23X17 mcp2;
#endif
extern Preferences preferences;
extern std::mutex mcpMutex;

const PROGMEM int SCREEN_BLOCK_HEIGHT = 0;
const PROGMEM int SCREEN_MSCW_TIME = 1;
const PROGMEM int SCREEN_BTC_TICKER = 2;
const PROGMEM int SCREEN_TIME = 3;
const PROGMEM int SCREEN_HALVING_COUNTDOWN = 4;
const PROGMEM int SCREEN_MARKET_CAP = 5;

const PROGMEM int SCREEN_COUNTDOWN = 98;
const PROGMEM int SCREEN_CUSTOM = 99;
const int SCREEN_COUNT = 6;
const PROGMEM int screens[SCREEN_COUNT] = { SCREEN_BLOCK_HEIGHT, SCREEN_MSCW_TIME, SCREEN_BTC_TICKER, SCREEN_TIME, SCREEN_HALVING_COUNTDOWN, SCREEN_MARKET_CAP };
const int usPerSecond = 1000000;
const int usPerMinute = 60 * usPerSecond;

struct SpiRamAllocator {
  void* allocate(size_t size) {
    return heap_caps_malloc(size, MALLOC_CAP_SPIRAM);
  }

  void deallocate(void* pointer) {
    heap_caps_free(pointer);
  }

  void* reallocate(void* ptr, size_t new_size) {
    return heap_caps_realloc(ptr, new_size, MALLOC_CAP_SPIRAM);
  }
};

using SpiRamJsonDocument = BasicJsonDocument<SpiRamAllocator>;

