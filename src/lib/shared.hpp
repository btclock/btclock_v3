#pragma once

#include <Adafruit_MCP23X17.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <GxEPD2.h>
#include <GxEPD2_BW.h>

#include <mutex>
#include <utils.hpp>

#include "defaults.hpp"

extern Adafruit_MCP23X17 mcp1;
#ifdef IS_BTCLOCK_S3
extern Adafruit_MCP23X17 mcp2;
#endif
extern Preferences preferences;
extern std::mutex mcpMutex;

#ifdef VERSION_EPD_2_13
  #define EPD_CLASS GxEPD2_213_B74
#endif

#ifdef VERSION_EPD_2_9
  #define EPD_CLASS GxEPD2_290_T94
#endif

const PROGMEM int SCREEN_BLOCK_HEIGHT = 0;
const PROGMEM int SCREEN_MSCW_TIME = 1;
const PROGMEM int SCREEN_BTC_TICKER = 2;
const PROGMEM int SCREEN_TIME = 3;
const PROGMEM int SCREEN_HALVING_COUNTDOWN = 4;
const PROGMEM int SCREEN_MARKET_CAP = 5;
const PROGMEM int SCREEN_BLOCK_FEE_RATE = 6;
const PROGMEM int SCREEN_BITAXE_HASHRATE = 80;
const PROGMEM int SCREEN_BITAXE_BESTDIFF = 81;

const PROGMEM int SCREEN_COUNTDOWN = 98;
const PROGMEM int SCREEN_CUSTOM = 99;
const int SCREEN_COUNT = 7;
const PROGMEM int screens[SCREEN_COUNT] = {
    SCREEN_BLOCK_HEIGHT, SCREEN_MSCW_TIME,         SCREEN_BTC_TICKER,
    SCREEN_TIME,         SCREEN_HALVING_COUNTDOWN, SCREEN_MARKET_CAP,
    SCREEN_BLOCK_FEE_RATE};
const int usPerSecond = 1000000;
const int usPerMinute = 60 * usPerSecond;

struct SpiRamAllocator : ArduinoJson::Allocator {
  void* allocate(size_t size) override {
    return heap_caps_malloc(size, MALLOC_CAP_SPIRAM);
  }

  void deallocate(void* pointer) override {
    heap_caps_free(pointer);
  }

  void* reallocate(void* ptr, size_t new_size) override {
    return heap_caps_realloc(ptr, new_size, MALLOC_CAP_SPIRAM);
  }
};

struct ScreenMapping {
    int value;
    const char* name;
};