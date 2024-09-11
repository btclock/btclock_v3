#pragma once

#include <Adafruit_MCP23X17.h>
#include <ArduinoJson.h>
#include <WiFiClientSecure.h>
#include <Preferences.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <GxEPD2.h>
#include <GxEPD2_BW.h>
#include <mbedtls/md.h>

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

const PROGMEM int SCREEN_TIME = 3;
const PROGMEM int SCREEN_HALVING_COUNTDOWN = 4;
const PROGMEM int SCREEN_BLOCK_FEE_RATE = 6;

const PROGMEM int SCREEN_SATS_PER_CURRENCY = 10;

const PROGMEM int SCREEN_BTC_TICKER = 20;
// const PROGMEM int SCREEN_BTC_TICKER_USD = 20;
// const PROGMEM int SCREEN_BTC_TICKER_EUR = 21;
// const PROGMEM int SCREEN_BTC_TICKER_GBP = 22;
// const PROGMEM int SCREEN_BTC_TICKER_JPY = 23;
// const PROGMEM int SCREEN_BTC_TICKER_AUD = 24;
// const PROGMEM int SCREEN_BTC_TICKER_CAD = 25;

const PROGMEM int SCREEN_MARKET_CAP = 30;
// const PROGMEM int SCREEN_MARKET_CAP_USD = 30;
// const PROGMEM int SCREEN_MARKET_CAP_EUR = 31;
// const PROGMEM int SCREEN_MARKET_CAP_GBP = 32;
// const PROGMEM int SCREEN_MARKET_CAP_JPY = 33;
// const PROGMEM int SCREEN_MARKET_CAP_AUD = 34;
// const PROGMEM int SCREEN_MARKET_CAP_CAD = 35;

const PROGMEM int SCREEN_BITAXE_HASHRATE = 80;
const PROGMEM int SCREEN_BITAXE_BESTDIFF = 81;

const PROGMEM int SCREEN_COUNTDOWN = 98;
const PROGMEM int SCREEN_CUSTOM = 99;
const int SCREEN_COUNT = 7;
const PROGMEM int screens[SCREEN_COUNT] = {
    SCREEN_BLOCK_HEIGHT, SCREEN_SATS_PER_CURRENCY,         SCREEN_BTC_TICKER,
    SCREEN_TIME,         SCREEN_HALVING_COUNTDOWN, SCREEN_MARKET_CAP,
    SCREEN_BLOCK_FEE_RATE};
const int usPerSecond = 1000000;
const int usPerMinute = 60 * usPerSecond;

extern const char *github_root_ca;

const PROGMEM char UPDATE_FIRMWARE = 0;
const PROGMEM char UPDATE_WEBUI = 1;


struct ScreenMapping {
    int value;
    const char* name;
};

String calculateSHA256(uint8_t* data, size_t len);
String calculateSHA256(WiFiClient *stream, size_t contentLength);