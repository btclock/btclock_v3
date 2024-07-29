#include "webserver.hpp"

AsyncWebServer server(80);
AsyncEventSource events("/events");
TaskHandle_t eventSourceTaskHandle;

void setupWebserver()
{
  events.onConnect([](AsyncEventSourceClient *client)
                   { client->send("welcome", NULL, millis(), 1000); });
  server.addHandler(&events);

  // server.serveStatic("/css", LittleFS, "/css/");
  server.serveStatic("/fonts", LittleFS, "/fonts/");
  server.serveStatic("/build", LittleFS, "/build");
  server.serveStatic("/swagger.json", LittleFS, "/swagger.json");
  server.serveStatic("/api.html", LittleFS, "/api.html");
  server.serveStatic("/fs_hash.txt", LittleFS, "/fs_hash.txt");

  server.on("/", HTTP_GET, onIndex);

  server.on("/api/status", HTTP_GET, onApiStatus);
  server.on("/api/system_status", HTTP_GET, onApiSystemStatus);
  server.on("/api/wifi_set_tx_power", HTTP_GET, onApiSetWifiTxPower);

  server.on("/api/full_refresh", HTTP_GET, onApiFullRefresh);

  server.on("/api/stop_datasources", HTTP_GET, onApiStopDataSources);
  server.on("/api/restart_datasources", HTTP_GET, onApiRestartDataSources);

  server.on("/api/action/pause", HTTP_GET, onApiActionPause);
  server.on("/api/action/timer_restart", HTTP_GET, onApiActionTimerRestart);

  server.on("/api/settings", HTTP_GET, onApiSettingsGet);
  server.on("/api/settings", HTTP_POST, onApiSettingsPost);

  server.on("/api/show/screen", HTTP_GET, onApiShowScreen);
  server.on("/api/show/text", HTTP_GET, onApiShowText);

  AsyncCallbackJsonWebHandler *settingsPatchHandler =
      new AsyncCallbackJsonWebHandler("/api/json/settings", onApiSettingsPatch);
  server.addHandler(settingsPatchHandler);

  AsyncCallbackJsonWebHandler *handler = new AsyncCallbackJsonWebHandler(
      "/api/show/custom", onApiShowTextAdvanced);
  server.addHandler(handler);

  AsyncCallbackJsonWebHandler *lightsJsonHandler =
      new AsyncCallbackJsonWebHandler("/api/lights/set", onApiLightsSetJson);
  server.addHandler(lightsJsonHandler);

  server.on("/api/lights/off", HTTP_GET, onApiLightsOff);
  server.on("/api/lights/color", HTTP_GET, onApiLightsSetColor);
  server.on("/api/lights", HTTP_GET, onApiLightsStatus);
  server.on("/api/identify", HTTP_GET, onApiIdentify);

#ifdef HAS_FRONTLIGHT
  server.on("/api/frontlight/on", HTTP_GET, onApiFrontlightOn);
  server.on("/api/frontlight/flash", HTTP_GET, onApiFrontlightFlash);
  server.on("/api/frontlight/status", HTTP_GET, onApiFrontlightStatus);

  server.on("/api/frontlight/brightness", HTTP_GET, onApiFrontlightSetBrightness);
  server.on("/api/frontlight/off", HTTP_GET, onApiFrontlightOff);

  server.addRewrite(
      new OneParamRewrite("/api/frontlight/brightness/{b}", "/api/frontlight/brightness?b={b}"));
#endif

  // server.on("^\\/api\\/lights\\/([A-Fa-f0-9]{6}|[A-Fa-f0-9]{3})$", HTTP_GET,
  // onApiLightsSetColor);

  if (preferences.getBool("otaEnabled", DEFAULT_OTA_ENABLED))
  {
    server.on("/upload/firmware", HTTP_POST, onFirmwareUpdate, asyncFirmwareUpdateHandler);
    server.on("/upload/webui", HTTP_POST, onFirmwareUpdate, asyncWebuiUpdateHandler);
  }

  server.on("/api/restart", HTTP_GET, onApiRestart);
  server.addRewrite(new OneParamRewrite("/api/lights/color/{color}",
                                        "/api/lights/color?c={color}"));
  server.addRewrite(
      new OneParamRewrite("/api/show/screen/{s}", "/api/show/screen?s={s}"));
  server.addRewrite(
      new OneParamRewrite("/api/show/text/{text}", "/api/show/text?t={text}"));
  server.addRewrite(new OneParamRewrite("/api/show/number/{number}",
                                        "/api/show/text?t={text}"));

  server.onNotFound(onNotFound);

  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods",
                                       "GET, PATCH, POST, OPTIONS");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "*");

  server.begin();

  if (preferences.getBool("mdnsEnabled", DEFAULT_MDNS_ENABLED))
  {
    if (!MDNS.begin(getMyHostname()))
    {
      Serial.println(F("Error setting up MDNS responder!"));
      while (1)
      {
        delay(1000);
      }
    }
    MDNS.addService("http", "tcp", 80);
    MDNS.addServiceTxt("http", "tcp", "model", "BTClock");
    MDNS.addServiceTxt("http", "tcp", "version", "3.0");
    MDNS.addServiceTxt("http", "tcp", "rev", GIT_REV);
    MDNS.addServiceTxt("http", "tcp", "hw_rev", getHwRev());
  }

  xTaskCreate(eventSourceTask, "eventSourceTask", 4096, NULL, tskIDLE_PRIORITY,
              &eventSourceTaskHandle);
}

void stopWebServer() { server.end(); }

void onFirmwareUpdate(AsyncWebServerRequest *request)
{
  bool shouldReboot = !Update.hasError();
  AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", shouldReboot ? "OK" : "FAIL");
  response->addHeader("Connection", "close");
  request->send(response);
}

void asyncWebuiUpdateHandler(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final)
{
  asyncFileUpdateHandler(request, filename, index, data, len, final, U_SPIFFS);
}

void asyncFileUpdateHandler(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final, int command)
{
  if (!index)
  {
    Serial.printf("Update Start: %s\n", filename.c_str());

    if (command == U_FLASH)
    {
      // Update.runAsync(true);
      if (!Update.begin((ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000), command)
      {
        Update.printError(Serial);
        return;
      }
    }
    else if (command == U_SPIFFS)
    {
      size_t fsSize = UPDATE_SIZE_UNKNOWN; // or specify the size of your filesystem partition
      if (!Update.begin(fsSize, U_SPIFFS)) // or U_FS for LittleFS
      {
        Update.printError(Serial);
        return;
      }
    }
  }
  if (!Update.hasError())
  {
    if (Update.write(data, len) != len)
    {
      Update.printError(Serial);
    }
  }
  if (final)
  {
    if (Update.end(true))
    {
      Serial.printf("Update Success: %uB\n", index + len);
      onApiRestart(request);
    }
    else
    {
      Update.printError(Serial);
    }
  }
}

void asyncFirmwareUpdateHandler(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final)
{
  asyncFileUpdateHandler(request, filename, index, data, len, final, U_FLASH);

  // if (!index)
  // {
  //   Serial.printf("Update Start: %s\n", filename.c_str());

  //   // Update.runAsync(true);
  //   if (!Update.begin((ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000))
  //   {
  //     Update.printError(Serial);
  //   }
  // }
  // if (!Update.hasError())
  // {
  //   if (Update.write(data, len) != len)
  //   {
  //     Update.printError(Serial);
  //   }
  // }
  // if (final)
  // {
  //   if (Update.end(true))
  //   {
  //     Serial.printf("Update Success: %uB\n", index + len);
  //     onApiRestart(request);
  //   }
  //   else
  //   {
  //     Update.printError(Serial);
  //   }
  // }
}

JsonDocument getStatusObject()
{
  JsonDocument root;

  root["currentScreen"] = getCurrentScreen();
  root["numScreens"] = NUM_SCREENS;
  root["timerRunning"] = isTimerActive();
  root["espUptime"] = esp_timer_get_time() / 1000000;
  // root["currentPrice"] = getPrice();
  // root["currentBlockHeight"] = getBlockHeight();
  root["espFreeHeap"] = ESP.getFreeHeap();
  root["espHeapSize"] = ESP.getHeapSize();
  // root["espFreePsram"] = ESP.getFreePsram();
  // root["espPsramSize"] = ESP.getPsramSize();

  JsonObject conStatus = root["connectionStatus"].to<JsonObject>();
  conStatus["price"] = isPriceNotifyConnected();
  conStatus["blocks"] = isBlockNotifyConnected();
  conStatus["nostr"] = nostrConnected();

  root["rssi"] = WiFi.RSSI();

#ifdef HAS_FRONTLIGHT
  std::vector<uint16_t> statuses = frontlightGetStatus();
  uint16_t arr[NUM_SCREENS];
  std::copy(statuses.begin(), statuses.end(), arr);

  JsonArray data = root["flStatus"].to<JsonArray>();
  copyArray(arr, data);

  if (hasLightLevel())
  {
    root["lightLevel"] = getLightLevel();
  }
#endif

  return root;
}

JsonDocument getLedStatusObject()
{
  JsonDocument root;
  JsonArray colors = root["data"].to<JsonArray>();
  //    Adafruit_NeoPixel pix = getPixels();

  for (uint i = 0; i < pixels.numPixels(); i++)
  {
    uint32_t pixColor = pixels.getPixelColor(pixels.numPixels() - i - 1);
    uint alpha = (pixColor >> 24) & 0xFF;
    uint red = (pixColor >> 16) & 0xFF;
    uint green = (pixColor >> 8) & 0xFF;
    uint blue = pixColor & 0xFF;
    char hexColor[8];
    sprintf(hexColor, "#%02X%02X%02X", red, green, blue);

    JsonObject object = colors.add<JsonObject>();
    object["red"] = red;
    object["green"] = green;
    object["blue"] = blue;
    object["hex"] = hexColor;
  }

  return root;
}

void eventSourceUpdate()
{
  if (!events.count())
    return;
  JsonDocument root = getStatusObject();
  JsonArray data = root["data"].to<JsonArray>();

  root["leds"] = getLedStatusObject()["data"];

  String epdContent[NUM_SCREENS];
  std::array<String, NUM_SCREENS> retEpdContent = getCurrentEpdContent();
  std::copy(std::begin(retEpdContent), std::end(retEpdContent), epdContent);

  copyArray(epdContent, data);

  String bufString;
  serializeJson(root, bufString);

  events.send(bufString.c_str(), "status");
}

/**
 * @Api
 * @Path("/api/status")
 */
void onApiStatus(AsyncWebServerRequest *request)
{
  AsyncResponseStream *response =
      request->beginResponseStream("application/json");

  JsonDocument root = getStatusObject();
  JsonArray data = root["data"].to<JsonArray>();
  JsonArray rendered = root["rendered"].to<JsonArray>();
  String epdContent[NUM_SCREENS];

  root["leds"] = getLedStatusObject()["data"];

  std::array<String, NUM_SCREENS> retEpdContent = getCurrentEpdContent();

  std::copy(std::begin(retEpdContent), std::end(retEpdContent), epdContent);

  copyArray(epdContent, data);
  copyArray(epdContent, rendered);
  serializeJson(root, *response);

  request->send(response);
}

/**
 * @Api
 * @Path("/api/action/pause")
 */
void onApiActionPause(AsyncWebServerRequest *request)
{
  setTimerActive(false);
  request->send(200);
};

/**
 * @Api
 * @Path("/api/action/timer_restart")
 */
void onApiActionTimerRestart(AsyncWebServerRequest *request)
{
  setTimerActive(true);
  request->send(200);
}

/**
 * @Api
 * @Path("/api/full_refresh")
 */
void onApiFullRefresh(AsyncWebServerRequest *request)
{
  forceFullRefresh();
  std::array<String, NUM_SCREENS> newEpdContent = getCurrentEpdContent();

  setEpdContent(newEpdContent, true);

  request->send(200);
}

/**
 * @Api
 * @Path("/api/show/screen")
 */
void onApiShowScreen(AsyncWebServerRequest *request)
{
  if (request->hasParam("s"))
  {
    const AsyncWebParameter *p = request->getParam("s");
    uint currentScreen = p->value().toInt();
    setCurrentScreen(currentScreen);
  }
  request->send(200);
}

void onApiShowText(AsyncWebServerRequest *request)
{
  if (request->hasParam("t"))
  {
    const AsyncWebParameter *p = request->getParam("t");
    String t = p->value();
    t.toUpperCase(); // This is needed as long as lowercase letters are glitchy

    std::array<String, NUM_SCREENS> textEpdContent;
    for (uint i = 0; i < NUM_SCREENS; i++)
    {
      textEpdContent[i] = t[i];
    }

    setEpdContent(textEpdContent);
  }
  setCurrentScreen(SCREEN_CUSTOM);
  request->send(200);
}

void onApiShowTextAdvanced(AsyncWebServerRequest *request, JsonVariant &json)
{
  JsonArray screens = json.as<JsonArray>();

  std::array<String, NUM_SCREENS> epdContent;
  int i = 0;
  for (JsonVariant s : screens)
  {
    epdContent[i] = s.as<String>();
    i++;
  }

  setEpdContent(epdContent);

  setCurrentScreen(SCREEN_CUSTOM);
  request->send(200);
}

void onApiSettingsPatch(AsyncWebServerRequest *request, JsonVariant &json)
{
  JsonObject settings = json.as<JsonObject>();

  bool settingsChanged = true;

  if (settings.containsKey("fgColor"))
  {
    String fgColor = settings["fgColor"].as<String>();
    preferences.putUInt("fgColor", strtol(fgColor.c_str(), NULL, 16));
    setFgColor(int(strtol(fgColor.c_str(), NULL, 16)));
    Serial.print(F("Setting foreground color to "));
    Serial.println(strtol(fgColor.c_str(), NULL, 16));
    settingsChanged = true;
  }
  if (settings.containsKey("bgColor"))
  {
    String bgColor = settings["bgColor"].as<String>();

    preferences.putUInt("bgColor", strtol(bgColor.c_str(), NULL, 16));
    setBgColor(int(strtol(bgColor.c_str(), NULL, 16)));
    Serial.print(F("Setting background color to "));
    Serial.println(bgColor.c_str());
    settingsChanged = true;
  }

  if (settings.containsKey("timePerScreen"))
  {
    preferences.putUInt("timerSeconds",
                        settings["timePerScreen"].as<uint>() * 60);
  }

  String strSettings[] = {"hostnamePrefix", "mempoolInstance", "nostrPubKey", "nostrRelay", "bitaxeHostname"};

  for (String setting : strSettings)
  {
    if (settings.containsKey(setting))
    {
      preferences.putString(setting.c_str(), settings[setting].as<String>());
      Serial.printf("Setting %s to %s\r\n", setting.c_str(),
                    settings[setting].as<String>());
    }
  }

  String uintSettings[] = {"minSecPriceUpd", "fullRefreshMin", "ledBrightness", "flMaxBrightness", "flEffectDelay", "luxLightToggle", "wpTimeout"};

  for (String setting : uintSettings)
  {
    if (settings.containsKey(setting))
    {
      preferences.putUInt(setting.c_str(), settings[setting].as<uint>());
      Serial.printf("Setting %s to %d\r\n", setting.c_str(),
                    settings[setting].as<uint>());
    }
  }

  if (settings.containsKey("tzOffset"))
  {
    int gmtOffset = settings["tzOffset"].as<int>() * 60;
    size_t written = preferences.putInt("gmtOffset", gmtOffset);
    Serial.printf("Setting %s to %d (%d minutes, written %d)\r\n", "gmtOffset",
                  gmtOffset, settings["tzOffset"].as<int>(), written);
  }

  String boolSettings[] = {"fetchEurPrice", "ledTestOnPower", "ledFlashOnUpd",
                           "mdnsEnabled", "otaEnabled", "stealFocus",
                           "mcapBigChar", "useSatsSymbol", "useBlkCountdown",
                           "suffixPrice", "disableLeds", "ownDataSource", "flAlwaysOn", "flFlashOnUpd", "mempoolSecure", "useNostr", "bitaxeEnabled"};

  for (String setting : boolSettings)
  {
    if (settings.containsKey(setting))
    {
      preferences.putBool(setting.c_str(), settings[setting].as<boolean>());
      Serial.printf("Setting %s to %d\r\n", setting.c_str(),
                    settings[setting].as<boolean>());
    }
  }

  if (settings.containsKey("screens"))
  {
    for (JsonVariant screen : settings["screens"].as<JsonArray>())
    {
      JsonObject s = screen.as<JsonObject>();
      uint id = s["id"].as<uint>();
      String key = "screen[" + String(id) + "]";
      String prefKey = "screen" + String(id) + "Visible";
      bool visible = s["enabled"].as<boolean>();
      preferences.putBool(prefKey.c_str(), visible);
    }
  }

  if (settings.containsKey("txPower"))
  {
    int txPower = settings["txPower"].as<int>();

    if (txPower == 80)
    {
      preferences.remove("txPower");
      if (WiFi.getTxPower() != 80)
      {
        ESP.restart();
      }
    }
    else if (static_cast<int>(wifi_power_t::WIFI_POWER_MINUS_1dBm) <=
                 txPower &&
             txPower <= static_cast<int>(wifi_power_t::WIFI_POWER_19_5dBm))
    {
      // is valid value

      if (WiFi.setTxPower(static_cast<wifi_power_t>(txPower)))
      {
        Serial.printf("Set WiFi Tx power to: %d\n", txPower);
        preferences.putInt("txPower", txPower);
        settingsChanged = true;
      }
    }
  }

  request->send(200);
  if (settingsChanged)
  {
    queueLedEffect(LED_FLASH_SUCCESS);
  }
}

void onApiRestart(AsyncWebServerRequest *request)
{
  request->send(200);

  if (events.count())
    events.send("closing");

  delay(500);

  esp_restart();
}

void onApiIdentify(AsyncWebServerRequest *request)
{
  queueLedEffect(LED_FLASH_IDENTIFY);

  request->send(200);
}

/**
 * @Api
 * @Method GET
 * @Path("/api/settings")
 */
void onApiSettingsGet(AsyncWebServerRequest *request)
{
  JsonDocument root;
  root["numScreens"] = NUM_SCREENS;
  root["fgColor"] = getFgColor();
  root["bgColor"] = getBgColor();
  root["timerSeconds"] = getTimerSeconds();
  root["timerRunning"] = isTimerActive();
  root["minSecPriceUpd"] = preferences.getUInt(
      "minSecPriceUpd", DEFAULT_SECONDS_BETWEEN_PRICE_UPDATE);
  root["fullRefreshMin"] =
      preferences.getUInt("fullRefreshMin", DEFAULT_MINUTES_FULL_REFRESH);
  root["wpTimeout"] = preferences.getUInt("wpTimeout", DEFAULT_WP_TIMEOUT);
  root["tzOffset"] = preferences.getInt("gmtOffset", DEFAULT_TIME_OFFSET_SECONDS) / 60;
  root["mempoolInstance"] =
      preferences.getString("mempoolInstance", DEFAULT_MEMPOOL_INSTANCE);
  root["mempoolSecure"] = preferences.getBool("mempoolSecure", DEFAULT_MEMPOOL_SECURE);
  root["useNostr"] = preferences.getBool("useNostr", DEFAULT_USE_NOSTR);
  root["ledTestOnPower"] = preferences.getBool("ledTestOnPower", DEFAULT_LED_TEST_ON_POWER);
  root["ledFlashOnUpd"] = preferences.getBool("ledFlashOnUpd", DEFAULT_LED_FLASH_ON_UPD);
  root["ledBrightness"] = preferences.getUInt("ledBrightness", DEFAULT_LED_BRIGHTNESS);
  root["stealFocus"] = preferences.getBool("stealFocus", DEFAULT_STEAL_FOCUS);
  root["mcapBigChar"] = preferences.getBool("mcapBigChar", DEFAULT_MCAP_BIG_CHAR);
  root["mdnsEnabled"] = preferences.getBool("mdnsEnabled", DEFAULT_MDNS_ENABLED);
  root["otaEnabled"] = preferences.getBool("otaEnabled", DEFAULT_OTA_ENABLED);
  root["fetchEurPrice"] = preferences.getBool("fetchEurPrice", DEFAULT_FETCH_EUR_PRICE);
  root["useSatsSymbol"] = preferences.getBool("useSatsSymbol", DEFAULT_USE_SATS_SYMBOL);
  root["useBlkCountdown"] = preferences.getBool("useBlkCountdown", DEFAULT_USE_BLOCK_COUNTDOWN);
  root["suffixPrice"] = preferences.getBool("suffixPrice", DEFAULT_SUFFIX_PRICE);
  root["disableLeds"] = preferences.getBool("disableLeds", DEFAULT_DISABLE_LEDS);

  root["hostnamePrefix"] = preferences.getString("hostnamePrefix", DEFAULT_HOSTNAME_PREFIX);
  root["hostname"] = getMyHostname();
  root["ip"] = WiFi.localIP();
  root["txPower"] = WiFi.getTxPower();
  root["ownDataSource"] = preferences.getBool("ownDataSource", DEFAULT_OWN_DATA_SOURCE);

  root["nostrPubKey"] = preferences.getString("nostrPubKey", DEFAULT_NOSTR_NPUB);
  root["nostrRelay"] = preferences.getString("nostrRelay", DEFAULT_NOSTR_RELAY);

  root["bitaxeEnabled"] = preferences.getBool("bitaxeEnabled", DEFAULT_BITAXE_ENABLED);
  root["bitaxeHostname"] = preferences.getString("bitaxeHostname", DEFAULT_BITAXE_HOSTNAME);

#ifdef HAS_FRONTLIGHT
  root["hasFrontlight"] = true;
  root["flMaxBrightness"] = preferences.getUInt("flMaxBrightness", DEFAULT_FL_MAX_BRIGHTNESS);
  root["flAlwaysOn"] = preferences.getBool("flAlwaysOn", DEFAULT_FL_ALWAYS_ON);
  root["flEffectDelay"] = preferences.getUInt("flEffectDelay", DEFAULT_FL_EFFECT_DELAY);
  root["flFlashOnUpd"] = preferences.getBool("flFlashOnUpd", DEFAULT_FL_FLASH_ON_UPDATE);
  root["hasLightLevel"] = hasLightLevel();
  root["luxLightToggle"] = preferences.getUInt("luxLightToggle", DEFAULT_LUX_LIGHT_TOGGLE);
#else
  root["hasFrontlight"] = false;
  root["hasLightLevel"] = false;
#endif

  root["hwRev"] = getHwRev();
  root["fsRev"] = getFsRev();

#ifdef GIT_REV
  root["gitRev"] = String(GIT_REV);
#endif
#ifdef GIT_TAG
  root["gitTag"] = String(GIT_TAG);
#endif

#ifdef LAST_BUILD_TIME
  root["lastBuildTime"] = String(LAST_BUILD_TIME);
#endif
  JsonArray screens = root["screens"].to<JsonArray>();

  std::vector<ScreenMapping> screenNameMap = getScreenNameMap();

  for (int i = 0; i < screenNameMap.size(); i++)
  {
    JsonObject o = screens.add<JsonObject>();
    String key = "screen" + String(screenNameMap.at(i).value) + "Visible";
    o["id"] = screenNameMap.at(i).value;
    o["name"] = screenNameMap.at(i).name;
    o["enabled"] = preferences.getBool(key.c_str(), true);
  }

  AsyncResponseStream *response =
      request->beginResponseStream("application/json");
  serializeJson(root, *response);

  request->send(response);
}

bool processEpdColorSettings(AsyncWebServerRequest *request)
{
  bool settingsChanged = false;
  if (request->hasParam("fgColor", true))
  {
    const AsyncWebParameter *fgColor = request->getParam("fgColor", true);
    preferences.putUInt("fgColor", strtol(fgColor->value().c_str(), NULL, 16));
    setFgColor(int(strtol(fgColor->value().c_str(), NULL, 16)));
    // Serial.print(F("Setting foreground color to "));
    // Serial.println(fgColor->value().c_str());
    settingsChanged = true;
  }
  if (request->hasParam("bgColor", true))
  {
    const AsyncWebParameter *bgColor = request->getParam("bgColor", true);

    preferences.putUInt("bgColor", strtol(bgColor->value().c_str(), NULL, 16));
    setBgColor(int(strtol(bgColor->value().c_str(), NULL, 16)));
    // Serial.print(F("Setting background color to "));
    // Serial.println(bgColor->value().c_str());
    settingsChanged = true;
  }

  return settingsChanged;
}

void onApiSettingsPost(AsyncWebServerRequest *request)
{
  // bool settingsChanged = false;

  // settingsChanged = processEpdColorSettings(request);

  // int headers = request->headers();
  // int i;
  // for (i = 0; i < headers; i++)
  // {
  //   AsyncWebHeader *h = request->getHeader(i);
  //   Serial.printf("HEADER[%s]: %s\n", h->name().c_str(), h->value().c_str());
  // }

  // int params = request->params();
  // for (int i = 0; i < params; i++)
  // {
  //   const AsyncWebParameter *p = request->getParam(i);
  //   if (p->isFile())
  //   { // p->isPost() is also true
  //     Serial.printf("FILE[%s]: %s, size: %u\n", p->name().c_str(),
  //                   p->value().c_str(), p->size());
  //   }
  //   else if (p->isPost())
  //   {
  //     Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
  //   }
  //   else
  //   {
  //     Serial.printf("GET[%s]: %s\n", p->name().c_str(), p->value().c_str());
  //   }
  // }

  // if (request->hasParam("fetchEurPrice", true))
  // {
  //   const AsyncWebParameter *fetchEurPrice = request->getParam("fetchEurPrice", true);

  //   preferences.putBool("fetchEurPrice", fetchEurPrice->value().toInt());
  //   settingsChanged = true;
  // }
  // else
  // {
  //   preferences.putBool("fetchEurPrice", 0);
  //   settingsChanged = true;
  // }

  // if (request->hasParam("ledTestOnPower", true))
  // {
  //   const AsyncWebParameter *ledTestOnPower =
  //       request->getParam("ledTestOnPower", true);

  //   preferences.putBool("ledTestOnPower", ledTestOnPower->value().toInt());
  //   settingsChanged = true;
  // }
  // else
  // {
  //   preferences.putBool("ledTestOnPower", 0);
  //   settingsChanged = true;
  // }

  // if (request->hasParam("ledFlashOnUpd", true))
  // {
  //   const AsyncWebParameter *ledFlashOnUpdate =
  //       request->getParam("ledFlashOnUpd", true);

  //   preferences.putBool("ledFlashOnUpd", ledFlashOnUpdate->value().toInt());
  //   settingsChanged = true;
  // }
  // else
  // {
  //   preferences.putBool("ledFlashOnUpd", 0);
  //   settingsChanged = true;
  // }

  // if (request->hasParam("mdnsEnabled", true))
  // {
  //   const AsyncWebParameter *mdnsEnabled = request->getParam("mdnsEnabled", true);

  //   preferences.putBool("mdnsEnabled", mdnsEnabled->value().toInt());
  //   settingsChanged = true;
  // }
  // else
  // {
  //   preferences.putBool("mdnsEnabled", 0);
  //   settingsChanged = true;
  // }

  // if (request->hasParam("otaEnabled", true))
  // {
  //   const AsyncWebParameter *otaEnabled = request->getParam("otaEnabled", true);

  //   preferences.putBool("otaEnabled", otaEnabled->value().toInt());
  //   settingsChanged = true;
  // }
  // else
  // {
  //   preferences.putBool("otaEnabled", 0);
  //   settingsChanged = true;
  // }

  // if (request->hasParam("stealFocus", false))
  // {
  //   const AsyncWebParameter *stealFocusOnBlock =
  //       request->getParam("stealFocus", false);

  //   preferences.putBool("stealFocus", stealFocusOnBlock->value().toInt());
  //   settingsChanged = true;
  // }
  // else
  // {
  //   preferences.putBool("stealFocus", 0);
  //   settingsChanged = true;
  // }

  // if (request->hasParam("mcapBigChar", true))
  // {
  //   const AsyncWebParameter *mcapBigChar = request->getParam("mcapBigChar", true);

  //   preferences.putBool("mcapBigChar", mcapBigChar->value().toInt());
  //   settingsChanged = true;
  // }
  // else
  // {
  //   preferences.putBool("mcapBigChar", 0);
  //   settingsChanged = true;
  // }

  // if (request->hasParam("mempoolInstance", true))
  // {
  //   const AsyncWebParameter *mempoolInstance =
  //       request->getParam("mempoolInstance", true);

  //   preferences.putString("mempoolInstance", mempoolInstance->value().c_str());
  //   settingsChanged = true;
  // }

  // if (request->hasParam("hostnamePrefix", true))
  // {
  //   const AsyncWebParameter *hostnamePrefix =
  //       request->getParam("hostnamePrefix", true);

  //   preferences.putString("hostnamePrefix", hostnamePrefix->value().c_str());
  //   settingsChanged = true;
  // }

  // if (request->hasParam("ledBrightness", true))
  // {
  //   const AsyncWebParameter *ledBrightness = request->getParam("ledBrightness", true);

  //   preferences.putUInt("ledBrightness", ledBrightness->value().toInt());
  //   settingsChanged = true;
  // }

  // if (request->hasParam("fullRefreshMin", true))
  // {
  //   const AsyncWebParameter *fullRefreshMin =
  //       request->getParam("fullRefreshMin", true);

  //   preferences.putUInt("fullRefreshMin", fullRefreshMin->value().toInt());
  //   settingsChanged = true;
  // }

  // if (request->hasParam("wpTimeout", true))
  // {
  //   const AsyncWebParameter *wpTimeout = request->getParam("wpTimeout", true);

  //   preferences.putUInt("wpTimeout", wpTimeout->value().toInt());
  //   settingsChanged = true;
  // }

  // std::vector<std::string> screenNameMap = getScreenNameMap();

  // if (request->hasParam("screens"))
  // {
  //   const AsyncWebParameter *screenParam = request->getParam("screens", true);

  //   Serial.printf(screenParam->value().c_str());
  // }

  // for (int i = 0; i < screenNameMap.size(); i++)
  // {
  //   String key = "screen[" + String(i) + "]";
  //   String prefKey = "screen" + String(i) + "Visible";
  //   bool visible = false;
  //   if (request->hasParam(key, true))
  //   {
  //     const AsyncWebParameter *screenParam = request->getParam(key, true);
  //     visible = screenParam->value().toInt();
  //   }

  //   preferences.putBool(prefKey.c_str(), visible);
  // }

  // if (request->hasParam("tzOffset", true))
  // {
  //   const AsyncWebParameter *p = request->getParam("tzOffset", true);
  //   int tzOffsetSeconds = p->value().toInt() * 60;
  //   preferences.putInt("gmtOffset", tzOffsetSeconds);
  //   settingsChanged = true;
  // }

  // if (request->hasParam("minSecPriceUpd", true))
  // {
  //   const AsyncWebParameter *p = request->getParam("minSecPriceUpd", true);
  //   int minSecPriceUpd = p->value().toInt();
  //   preferences.putUInt("minSecPriceUpd", minSecPriceUpd);
  //   settingsChanged = true;
  // }

  // if (request->hasParam("timePerScreen", true))
  // {
  //   const AsyncWebParameter *p = request->getParam("timePerScreen", true);
  //   uint timerSeconds = p->value().toInt() * 60;
  //   preferences.putUInt("timerSeconds", timerSeconds);
  //   settingsChanged = true;
  // }

  // request->send(200);
  // if (settingsChanged)
  // {
  //   queueLedEffect(LED_FLASH_SUCCESS);
  // }
}

void onApiSystemStatus(AsyncWebServerRequest *request)
{
  AsyncResponseStream *response =
      request->beginResponseStream("application/json");

  JsonDocument root;

  root["espFreeHeap"] = ESP.getFreeHeap();
  root["espHeapSize"] = ESP.getHeapSize();
  root["espFreePsram"] = ESP.getFreePsram();
  root["espPsramSize"] = ESP.getPsramSize();
  root["rssi"] = WiFi.RSSI();
  root["txPower"] = WiFi.getTxPower();

  serializeJson(root, *response);

  request->send(response);
}

#define STRINGIFY(x) #x
#define ENUM_TO_STRING(x) STRINGIFY(x)

void onApiSetWifiTxPower(AsyncWebServerRequest *request)
{
  if (request->hasParam("txPower"))
  {
    const AsyncWebParameter *txPowerParam = request->getParam("txPower");
    int txPower = txPowerParam->value().toInt();
    if (static_cast<int>(wifi_power_t::WIFI_POWER_MINUS_1dBm) <= txPower &&
        txPower <= static_cast<int>(wifi_power_t::WIFI_POWER_19_5dBm))
    {
      // is valid value
      String txPowerName =
          std::to_string(
              static_cast<std::underlying_type_t<wifi_power_t>>(txPower))
              .c_str();

      Serial.printf("Set WiFi Tx power to: %s\n", txPowerName);

      if (WiFi.setTxPower(static_cast<wifi_power_t>(txPower)))
      {
        preferences.putInt("txPower", txPower);
        request->send(200, "application/json", "{\"setTxPower\": \"ok\"}");
        return;
      }
    }
  }

  return request->send(400);
}

void onApiLightsStatus(AsyncWebServerRequest *request)
{
  AsyncResponseStream *response =
      request->beginResponseStream("application/json");

  serializeJson(getLedStatusObject()["data"], *response);

  request->send(response);
}

void onApiStopDataSources(AsyncWebServerRequest *request)
{
  AsyncResponseStream *response =
      request->beginResponseStream("application/json");

  stopPriceNotify();
  stopBlockNotify();

  request->send(response);
}

void onApiRestartDataSources(AsyncWebServerRequest *request)
{
  AsyncResponseStream *response =
      request->beginResponseStream("application/json");

  restartPriceNotify();
  restartBlockNotify();
  //  setupPriceNotify();
  //  setupBlockNotify();

  request->send(response);
}

void onApiLightsOff(AsyncWebServerRequest *request)
{
  setLights(0, 0, 0);
  request->send(200);
}

void onApiLightsSetColor(AsyncWebServerRequest *request)
{
  if (request->hasParam("c"))
  {
    AsyncResponseStream *response =
        request->beginResponseStream("application/json");

    String rgbColor = request->getParam("c")->value();

    if (rgbColor.compareTo("off") == 0)
    {
      setLights(0, 0, 0);
    }
    else
    {
      uint r, g, b;
      sscanf(rgbColor.c_str(), "%02x%02x%02x", &r, &g, &b);
      setLights(r, g, b);
    }

    JsonDocument doc;
    doc["result"] = rgbColor;

    serializeJson(getLedStatusObject()["data"], *response);

    request->send(response);
  }
  else
  {
    request->send(400);
  }
}

void onApiLightsSetJson(AsyncWebServerRequest *request, JsonVariant &json)
{
  JsonArray lights = json.as<JsonArray>();

  if (lights.size() != pixels.numPixels())
  {
    if (!lights.size())
    {
      // if empty, assume off request
      return onApiLightsOff(request);
    }

    Serial.printf("Invalid values for LED set %d\n", lights.size());
    request->send(400);
    return;
  }

  for (uint i = 0; i < pixels.numPixels(); i++)
  {
    unsigned int red, green, blue;

    if (lights[i].containsKey("red") && lights[i].containsKey("green") &&
        lights[i].containsKey("blue"))
    {
      red = lights[i]["red"].as<uint>();
      green = lights[i]["green"].as<uint>();
      blue = lights[i]["blue"].as<uint>();
    }
    else if (lights[i].containsKey("hex"))
    {
      if (!sscanf(lights[i]["hex"].as<String>().c_str(), "#%02X%02X%02X", &red,
                  &green, &blue) == 3)
      {
        Serial.printf("Invalid hex for LED %d\n", i);
        request->send(400);
        return;
      }
    }
    else
    {
      Serial.printf("No valid color for LED %d\n", i);
      request->send(400);
      return;
    }

    pixels.setPixelColor((pixels.numPixels() - i - 1),
                         pixels.Color(red, green, blue));
  }

  pixels.show();
  saveLedState();

  request->send(200);
}

void onIndex(AsyncWebServerRequest *request)
{
  request->send(LittleFS, "/index.html", String(), false);
}

void onNotFound(AsyncWebServerRequest *request)
{
  // Serial.printf("NotFound, URL[%s]\n", request->url());

  // Serial.printf("NotFound, METHOD[%s]\n", request->methodToString());

  // int headers = request->headers();
  // int i;
  // for (i = 0; i < headers; i++)
  // {
  //     AsyncWebHeader *h = request->getHeader(i);
  //     Serial.printf("NotFound HEADER[%s]: %s\n", h->name().c_str(),
  //     h->value().c_str());
  // }

  // int params = request->params();
  // for (int i = 0; i < params; i++)
  // {
  //     const AsyncWebParameter *p = request->getParam(i);
  //     if (p->isFile())
  //     { // p->isPost() is also true
  //         Serial.printf("NotFound FILE[%s]: %s, size: %u\n",
  //         p->name().c_str(), p->value().c_str(), p->size());
  //     }
  //     else if (p->isPost())
  //     {
  //         Serial.printf("NotFound POST[%s]: %s\n", p->name().c_str(),
  //         p->value().c_str());
  //     }
  //     else
  //     {
  //         Serial.printf("NotFound GET[%s]: %s\n", p->name().c_str(),
  //         p->value().c_str());
  //     }
  // }

  // Access-Control-Request-Method == POST might be better

  if (request->method() == HTTP_OPTIONS ||
      request->hasHeader("Sec-Fetch-Mode"))
  {
    // Serial.printf("NotFound, Return[%d]\n", 200);

    request->send(200);
  }
  else
  {
    // Serial.printf("NotFound, Return[%d]\n", 404);
    request->send(404);
  }
};

void eventSourceTask(void *pvParameters)
{
  for (;;)
  {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    eventSourceUpdate();
  }
}

#ifdef HAS_FRONTLIGHT
void onApiFrontlightOn(AsyncWebServerRequest *request)
{
  frontlightFadeInAll();

  request->send(200);
}

void onApiFrontlightStatus(AsyncWebServerRequest *request)
{
  AsyncResponseStream *response =
      request->beginResponseStream("application/json");

  JsonDocument root;

  std::vector<uint16_t> statuses = frontlightGetStatus();
  uint16_t arr[NUM_SCREENS];
  std::copy(statuses.begin(), statuses.end(), arr);

  JsonArray data = root["flStatus"].to<JsonArray>();
  copyArray(arr, data);
  serializeJson(root, *response);

  request->send(response);
}

void onApiFrontlightFlash(AsyncWebServerRequest *request)
{
  frontlightFlash(preferences.getUInt("flEffectDelay"));

  request->send(200);
}

void onApiFrontlightSetBrightness(AsyncWebServerRequest *request)
{
  if (request->hasParam("b"))
  {
    frontlightSetBrightness(request->getParam("b")->value().toInt());
    request->send(200);
  }
  else
  {
    request->send(400);
  }
}

void onApiFrontlightOff(AsyncWebServerRequest *request)
{
  frontlightFadeOutAll();

  request->send(200);
}
#endif