#include "webserver.hpp"

AsyncWebServer server(80);
AsyncEventSource events("/events");
TaskHandle_t eventSourceTaskHandle;

void setupWebserver()
{
    if (!LittleFS.begin(true))
    {
        Serial.println(F("An Error has occurred while mounting LittleFS"));
        return;
    }

    events.onConnect([](AsyncEventSourceClient *client)
                     {
                         if (client->lastId())
                         {
                             Serial.printf("Client reconnected! Last message ID that it gat is: %u\n", client->lastId());
                         }
                         // send event with message "hello!", id current millis
                         //  and set reconnect delay to 1 second
                        client->send("welcome",NULL,millis(),1000);
                     });
    server.addHandler(&events);

    server.serveStatic("/css", LittleFS, "/css/");
    server.serveStatic("/js", LittleFS, "/js/");
    server.serveStatic("/font", LittleFS, "/font/");

    server.on("/", HTTP_GET, onIndex);

    server.on("/api/status", HTTP_GET, onApiStatus);
    server.on("/api/system_status", HTTP_GET, onApiSystemStatus);

    server.on("/api/action/pause", HTTP_GET, onApiActionPause);
    server.on("/api/action/timer_restart", HTTP_GET, onApiActionTimerRestart);

    server.on("/api/settings", HTTP_GET, onApiSettingsGet);
    server.on("/api/settings", HTTP_POST, onApiSettingsPost);

    server.on("/api/show/screen", HTTP_GET, onApiShowScreen);
    server.on("/api/show/text", HTTP_GET, onApiShowText);

    server.on("/api/lights/off", HTTP_GET, onApiLightsOff);
    server.on("/api/lights/color", HTTP_GET, onApiLightsSetColor);

    // server.on("^\\/api\\/lights\\/([A-Fa-f0-9]{6}|[A-Fa-f0-9]{3})$", HTTP_GET, onApiLightsSetColor);

    server.on("/api/restart", HTTP_GET, onApiRestart);
    server.addRewrite(new OneParamRewrite("/api/lights/{color}", "/api/lights/color?c={color}"));
    server.addRewrite(new OneParamRewrite("/api/show/screen/{s}", "/api/show/screen?s={s}"));
    server.addRewrite(new OneParamRewrite("/api/show/text/{text}", "/api/show/text?t={text}"));
    server.addRewrite(new OneParamRewrite("/api/show/number/{number}", "/api/show/text?t={text}"));

    server.onNotFound(onNotFound);

    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");

    server.begin();
    // if (!MDNS.begin(getMyHostname()))
    // {
    //     Serial.println(F("Error setting up MDNS responder!"));
    //     while (1)
    //     {
    //         delay(1000);
    //     }
    // }
    // MDNS.addService("http", "tcp", 80);

    xTaskCreate(eventSourceTask, "eventSourceTask", 4096, NULL, tskIDLE_PRIORITY, &eventSourceTaskHandle);
}

StaticJsonDocument<768> getStatusObject()
{
    StaticJsonDocument<768> root;

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

    JsonObject conStatus = root.createNestedObject("connectionStatus");
    conStatus["price"] = isPriceNotifyConnected();
    conStatus["blocks"] = isBlockNotifyConnected();

    return root;
}

void eventSourceUpdate()
{
    if (!events.count()) return;
    StaticJsonDocument<768> root = getStatusObject();
    JsonArray data = root.createNestedArray("data");
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
    AsyncResponseStream *response = request->beginResponseStream("application/json");

    StaticJsonDocument<768> root = getStatusObject();
    JsonArray data = root.createNestedArray("data");
    JsonArray rendered = root.createNestedArray("rendered");
    String epdContent[NUM_SCREENS];

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
    Serial.println(F("Update timer paused"));

    request->send(200);
};

/**
 * @Api
 * @Path("/api/action/timer_restart")
 */
void onApiActionTimerRestart(AsyncWebServerRequest *request)
{
    // moment = millis();
    setTimerActive(true);
    Serial.println(F("Update timer restarted"));

    request->send(200);
}

void onApiShowScreen(AsyncWebServerRequest *request)
{
    if (request->hasParam("s"))
    {
        AsyncWebParameter *p = request->getParam("s");
        uint currentScreen = p->value().toInt();
        setCurrentScreen(currentScreen);
    }
    request->send(200);
}

void onApiShowText(AsyncWebServerRequest *request)
{
    if (request->hasParam("t"))
    {
        AsyncWebParameter *p = request->getParam("t");
        String t = p->value();
        t.toUpperCase(); // This is needed as long as lowercase letters are glitchy

        std::array<String, NUM_SCREENS> textEpdContent;
        for (uint i = 0; i < NUM_SCREENS; i++)
        {
            textEpdContent[i] = t[i];
        }

        setEpdContent(textEpdContent);
    }
    // setCurrentScreen(SCREEN_CUSTOM);
    request->send(200);
}

void onApiRestart(AsyncWebServerRequest *request)
{
    request->send(200);
    esp_restart();
}

/**
 * @Api
 * @Method GET
 * @Path("/api/settings")
 */
void onApiSettingsGet(AsyncWebServerRequest *request)
{
    StaticJsonDocument<768> root;
    root["numScreens"] = NUM_SCREENS;
    root["fgColor"] = getFgColor();
    root["bgColor"] = getBgColor();
    root["timerSeconds"] = getTimerSeconds();
    root["timerRunning"] = isTimerActive();
    root["minSecPriceUpd"] = preferences.getUInt("minSecPriceUpd", DEFAULT_SECONDS_BETWEEN_PRICE_UPDATE);
    root["fullRefreshMin"] = preferences.getUInt("fullRefreshMin", 30);
    root["wpTimeout"] = preferences.getUInt("wpTimeout", 600);
    root["tzOffset"] = preferences.getInt("gmtOffset", TIME_OFFSET_SECONDS) / 60;
    root["useBitcoinNode"] = preferences.getBool("useNode", false);
    root["rpcPort"] = preferences.getUInt("rpcPort", BITCOIND_PORT);
    root["rpcUser"] = preferences.getString("rpcUser", BITCOIND_RPC_USER);
    root["rpcHost"] = preferences.getString("rpcHost", BITCOIND_HOST);
    root["mempoolInstance"] = preferences.getString("mempoolInstance", DEFAULT_MEMPOOL_INSTANCE);

    root["ledFlashOnUpdate"] = preferences.getBool("ledFlashOnUpd", false);
    root["ledBrightness"] = preferences.getUInt("ledBrightness", 128);

#ifdef GIT_REV
    root["gitRev"] = String(GIT_REV);
#endif
#ifdef LAST_BUILD_TIME
    root["lastBuildTime"] = String(LAST_BUILD_TIME);
#endif
    JsonArray screens = root.createNestedArray("screens");

    std::vector<std::string> screenNameMap = getScreenNameMap();

    for (int i = 0; i < screenNameMap.size(); i++)
    {
        JsonObject o = screens.createNestedObject();
        String key = "screen" + String(i) + "Visible";
        o["id"] = i;
        o["name"] = screenNameMap[i];
        o["enabled"] = preferences.getBool(key.c_str(), true);
    }

    AsyncResponseStream *response = request->beginResponseStream("application/json");
    serializeJson(root, *response);

    request->send(response);
}

bool processEpdColorSettings(AsyncWebServerRequest *request)
{
    bool settingsChanged = false;
    if (request->hasParam("fgColor", true))
    {
        AsyncWebParameter *fgColor = request->getParam("fgColor", true);
        preferences.putUInt("fgColor", strtol(fgColor->value().c_str(), NULL, 16));
        setFgColor(int(strtol(fgColor->value().c_str(), NULL, 16)));
        Serial.print(F("Setting foreground color to "));
        Serial.println(fgColor->value().c_str());
        settingsChanged = true;
    }
    if (request->hasParam("bgColor", true))
    {
        AsyncWebParameter *bgColor = request->getParam("bgColor", true);

        preferences.putUInt("bgColor", strtol(bgColor->value().c_str(), NULL, 16));
        setBgColor(int(strtol(bgColor->value().c_str(), NULL, 16)));
        Serial.print(F("Setting background color to "));
        Serial.println(bgColor->value().c_str());
        settingsChanged = true;
    }

    return settingsChanged;
}

void onApiSettingsPost(AsyncWebServerRequest *request)
{
    bool settingsChanged = false;

    settingsChanged = processEpdColorSettings(request);

    if (request->hasParam("ledFlashOnUpd", true))
    {
        AsyncWebParameter *ledFlashOnUpdate = request->getParam("ledFlashOnUpd", true);

        preferences.putBool("ledFlashOnUpd", ledFlashOnUpdate->value().toInt());
        Serial.print("Setting led flash on update to ");
        Serial.println(ledFlashOnUpdate->value().c_str());
        settingsChanged = true;
    }
    else
    {
        preferences.putBool("ledFlashOnUpd", 0);
        Serial.print("Setting led flash on update to false");
        settingsChanged = true;
    }

    if (request->hasParam("mempoolInstance", true))
    {
        AsyncWebParameter *mempoolInstance = request->getParam("mempoolInstance", true);

        preferences.putString("mempoolInstance", mempoolInstance->value().c_str());
        Serial.printf("Setting mempool instance to %s\r\n", mempoolInstance->value().c_str());
        settingsChanged = true;
    }

    if (request->hasParam("ledBrightness", true))
    {
        AsyncWebParameter *ledBrightness = request->getParam("ledBrightness", true);

        preferences.putUInt("ledBrightness", ledBrightness->value().toInt());
        Serial.printf("Setting brightness to %d\r\n", ledBrightness->value().toInt());
        settingsChanged = true;
    }

    if (request->hasParam("fullRefreshMin", true))
    {
        AsyncWebParameter *fullRefreshMin = request->getParam("fullRefreshMin", true);

        preferences.putUInt("fullRefreshMin", fullRefreshMin->value().toInt());
        Serial.printf("Set full refresh minutes to %d\r\n",fullRefreshMin->value().toInt());
        settingsChanged = true;
    }

    if (request->hasParam("wpTimeout", true))
    {
        AsyncWebParameter *wpTimeout = request->getParam("wpTimeout", true);

        preferences.putUInt("wpTimeout", wpTimeout->value().toInt());
        Serial.printf("Set WiFi portal timeout seconds to %d\r\n", wpTimeout->value().toInt());
        settingsChanged = true;
    }

    std::vector<std::string> screenNameMap = getScreenNameMap();

    for (int i = 0; i < screenNameMap.size(); i++)
    {
        String key = "screen[" + String(i) + "]";
        String prefKey = "screen" + String(i) + "Visible";
        bool visible = false;
        if (request->hasParam(key, true))
        {
            AsyncWebParameter *screenParam = request->getParam(key, true);
            visible = screenParam->value().toInt();
        }
        Serial.printf("Setting screen %d to %d\r\n", i, visible);

        preferences.putBool(prefKey.c_str(), visible);
    }

    if (request->hasParam("tzOffset", true))
    {
        AsyncWebParameter *p = request->getParam("tzOffset", true);
        int tzOffsetSeconds = p->value().toInt() * 60;
        preferences.putInt("gmtOffset", tzOffsetSeconds);
        Serial.printf("Setting tz offset to %d\r\n", tzOffsetSeconds);
        settingsChanged = true;
    }

     if (request->hasParam("minSecPriceUpd", true))
    {
        AsyncWebParameter *p = request->getParam("minSecPriceUpd", true);
        int minSecPriceUpd = p->value().toInt();
        preferences.putUInt("minSecPriceUpd", minSecPriceUpd);
        Serial.printf("Setting minSecPriceUpd to %d\r\n", minSecPriceUpd);
        settingsChanged = true;
    }

    if (request->hasParam("timePerScreen", true))
    {
        AsyncWebParameter *p = request->getParam("timePerScreen", true);
        uint timerSeconds = p->value().toInt() * 60;
        preferences.putUInt("timerSeconds", timerSeconds);
        settingsChanged = true;
    }

    if (request->hasParam("useBitcoinNode", true))
    {
        AsyncWebParameter *p = request->getParam("useBitcoinNode", true);
        bool useBitcoinNode = p->value().toInt();
        preferences.putBool("useNode", useBitcoinNode);
        settingsChanged = true;

        String rpcVars[] = {"rpcHost", "rpcPort", "rpcUser", "rpcPass"};

        for (String v : rpcVars)
        {
            if (request->hasParam(v, true))
            {
                AsyncWebParameter *pv = request->getParam(v, true);
                // Don't store an empty password, probably new settings save
                if (!(v.equals("rpcPass") && pv->value().length() == 0))
                {
                    preferences.putString(v.c_str(), pv->value().c_str());
                }
            }
        }
    }
    else
    {
        preferences.putBool("useNode", false);
        settingsChanged = true;
    }

    request->send(200);
    if (settingsChanged)
    {
        queueLedEffect(LED_FLASH_SUCCESS);

        Serial.println(F("Settings changed"));
    }
}

void onApiSystemStatus(AsyncWebServerRequest *request)
{
    AsyncResponseStream *response = request->beginResponseStream("application/json");

    StaticJsonDocument<128> root;

    root["espFreeHeap"] = ESP.getFreeHeap();
    root["espHeapSize"] = ESP.getHeapSize();
    root["espFreePsram"] = ESP.getFreePsram();
    root["espPsramSize"] = ESP.getPsramSize();

    serializeJson(root, *response);

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
        String rgbColor = request->getParam("c")->value();
        uint r, g, b;
        sscanf(rgbColor.c_str(), "%02x%02x%02x", &r, &g, &b);
        setLights(r, g, b);
        request->send(200, "text/plain", rgbColor);
    }
}

void onIndex(AsyncWebServerRequest *request) { request->send(LittleFS, "/index.html", String(), false); }

void onNotFound(AsyncWebServerRequest *request)
{
    if (request->method() == HTTP_OPTIONS)
    {
        request->send(200);
    }
    else
    {
        request->send(404);
    }
};

void eventSourceTask(void *pvParameters) {
    for (;;)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        eventSourceUpdate();
    }
}