#pragma once

#include <Adafruit_MCP23X17.h>
#include <ArduinoJson.h>
#include <Preferences.h>

extern Adafruit_MCP23X17 mcp;
extern Preferences preferences;

const PROGMEM int SCREEN_BLOCK_HEIGHT = 0;
const PROGMEM int SCREEN_MSCW_TIME = 1;
const PROGMEM int SCREEN_BTC_TICKER = 2;
const PROGMEM int SCREEN_TIME = 3;
const PROGMEM int SCREEN_HALVING_COUNTDOWN = 4;
const PROGMEM int SCREEN_COUNTDOWN = 98;
const PROGMEM int SCREEN_CUSTOM = 99;
const PROGMEM int screens[5] = { SCREEN_BLOCK_HEIGHT, SCREEN_MSCW_TIME, SCREEN_BTC_TICKER, SCREEN_TIME, SCREEN_HALVING_COUNTDOWN };

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
