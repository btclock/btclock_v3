#include "config.hpp"

#define MAX_ATTEMPTS_WIFI_CONNECTION 20

Preferences preferences;
Adafruit_MCP23X17 mcp1;
#ifdef IS_BTCLOCK_S3
Adafruit_MCP23X17 mcp2;
#endif

#ifdef HAS_FRONTLIGHT
PCA9685 flArray(PCA_I2C_ADDR);
BH1750 bh1750;
bool hasLuxSensor = false;
#endif

std::vector<ScreenMapping> screenMappings;
std::mutex mcpMutex;
uint lastTimeSync;

void addScreenMapping(int value, const char *name)
{
  screenMappings.push_back({value, name});
}

void setup()
{
  setupPreferences();
  setupHardware();

  setupDisplays();
  if (preferences.getBool("ledTestOnPower", DEFAULT_LED_TEST_ON_POWER))
  {
    queueLedEffect(LED_POWER_TEST);
  }
  {
    std::lock_guard<std::mutex> lockMcp(mcpMutex);
    if (mcp1.digitalRead(3) == LOW)
    {
      preferences.putBool("wifiConfigured", false);
      preferences.remove("txPower");

      WiFi.eraseAP();
      queueLedEffect(LED_EFFECT_WIFI_ERASE_SETTINGS);
    }
  }

  {
    if (mcp1.digitalRead(0) == LOW)
    {
      // Then loop forever to prevent anything else from writing to the screen
      while (true)
      {
        delay(1000);
      }
    }
    else if (mcp1.digitalRead(1) == LOW)
    {
      preferences.clear();
      queueLedEffect(LED_EFFECT_WIFI_ERASE_SETTINGS);
      nvs_flash_erase();
      delay(1000);

      ESP.restart();
    }
  }

  setupWifi();

  setupWebserver();

  syncTime();
  finishSetup();

  setupTasks();
  setupTimers();

  if (preferences.getBool("useNostr", DEFAULT_USE_NOSTR) || preferences.getBool("nostrZapNotify", DEFAULT_ZAP_NOTIFY_ENABLED))
  {
    setupNostrNotify(preferences.getBool("useNostr", DEFAULT_USE_NOSTR), preferences.getBool("nostrZapNotify", DEFAULT_ZAP_NOTIFY_ENABLED));
    setupNostrTask();
  }

  if (!preferences.getBool("useNostr", DEFAULT_USE_NOSTR))
  {
    xTaskCreate(setupWebsocketClients, "setupWebsocketClients", 8192, NULL,
                tskIDLE_PRIORITY, NULL);
  }

  if (preferences.getBool("bitaxeEnabled", DEFAULT_BITAXE_ENABLED))
  {
    setupBitaxeFetchTask();
  }

  setupButtonTask();
  setupOTA();

  waitUntilNoneBusy();

#ifdef HAS_FRONTLIGHT
  if (!preferences.getBool("flAlwaysOn", DEFAULT_FL_ALWAYS_ON))
  {
    frontlightFadeOutAll(preferences.getUInt("flEffectDelay"), true);
    flArray.allOFF();
  }
#endif

  forceFullRefresh();
}

void setupWifi()
{
  WiFi.onEvent(WiFiEvent);
  WiFi.setAutoConnect(true);
  WiFi.setAutoReconnect(true);
  WiFi.begin();
  if (preferences.getInt("txPower", DEFAULT_TX_POWER))
  {
    if (WiFi.setTxPower(
            static_cast<wifi_power_t>(preferences.getInt("txPower", DEFAULT_TX_POWER))))
    {
      Serial.printf("WiFi max tx power set to %d\n",
                    preferences.getInt("txPower", DEFAULT_TX_POWER));
    }
  }

  // if (!preferences.getBool("wifiConfigured", DEFAULT_WIFI_CONFIGURED)
  {

    queueLedEffect(LED_EFFECT_WIFI_WAIT_FOR_CONFIG);

    bool buttonPress = false;
    {
      std::lock_guard<std::mutex> lockMcp(mcpMutex);
      buttonPress = (mcp1.digitalRead(2) == LOW);
    }

    {
      WiFiManager wm;

      byte mac[6];
      WiFi.macAddress(mac);
      String softAP_SSID =
          String("BTClock" + String(mac[5], 16) + String(mac[1], 16));
      WiFi.setHostname(softAP_SSID.c_str());
      String softAP_password = replaceAmbiguousChars(
          base64::encode(String(mac[2], 16) + String(mac[4], 16) +
                         String(mac[5], 16) + String(mac[1], 16))
              .substring(2, 10));

      wm.setConfigPortalTimeout(preferences.getUInt("wpTimeout", DEFAULT_WP_TIMEOUT));
      wm.setWiFiAutoReconnect(false);
      wm.setDebugOutput(false);
      wm.setConfigPortalBlocking(true);

      wm.setAPCallback([&](WiFiManager *wifiManager)
                       {
        // Serial.printf("Entered config mode:ip=%s, ssid='%s', pass='%s'\n",
        // WiFi.softAPIP().toString().c_str(),
        // wifiManager->getConfigPortalSSID().c_str(),
        // softAP_password.c_str());
        // delay(6000);
        setFgColor(GxEPD_BLACK);
        setBgColor(GxEPD_WHITE);
        const String qrText = "qrWIFI:S:" + wifiManager->getConfigPortalSSID() +
                              ";T:WPA;P:" + softAP_password.c_str() + ";;";
        const String explainText = "*SSID: *\r\n" +
                                   wifiManager->getConfigPortalSSID() +
                                   "\r\n\r\n*Password:*\r\n" + softAP_password;
        // Set the UNIX timestamp
        time_t timestamp = LAST_BUILD_TIME; // Example timestamp: March 7, 2021 00:00:00 UTC

        // Convert the timestamp to a struct tm in UTC
        struct tm *timeinfo = gmtime(&timestamp);

        // Format the date
        char formattedDate[20];
        strftime(formattedDate, sizeof(formattedDate), "%y-%m-%d\r\n%H:%M:%S", timeinfo);
  
        std::array<String, NUM_SCREENS> epdContent = {
            "Welcome!",
            "Bienvenidos!",
            "To setup\r\nscan QR or\r\nconnect\r\nmanually",
            "Para\r\nconfigurar\r\nescanear QR\r\no conectar\r\nmanualmente",
            explainText,
            "*Hostname*:\r\n" + getMyHostname() + "\r\n\r\n" + "*FW build date:*\r\n" + formattedDate,
            qrText};
        setEpdContent(epdContent); });

      wm.setSaveConfigCallback([]()
                               {
        preferences.putBool("wifiConfigured", true);

        delay(1000);
        // just restart after succes
        ESP.restart(); });

      bool ac = wm.autoConnect(softAP_SSID.c_str(), softAP_password.c_str());

      // waitUntilNoneBusy();
      // std::array<String, NUM_SCREENS> epdContent = {"Welcome!",
      // "Bienvenidos!", "Use\r\nweb-interface\r\nto configure", "Use\r\nla
      // interfaz web\r\npara configurar", "Or
      // restart\r\nwhile\r\nholding\r\n2nd button\r\r\nto start\r\n QR-config",
      // "O reinicie\r\nmientras\r\n mantiene presionado\r\nel segundo
      // botón\r\r\npara iniciar\r\nQR-config", ""}; setEpdContent(epdContent);
      //  esp_task_wdt_init(30, false);
      //  uint count = 0;
      //  while (WiFi.status() != WL_CONNECTED)
      //  {
      //      if (Serial.available() > 0)
      //      {
      //          uint8_t b = Serial.read();

      //         if (parse_improv_serial_byte(x_position, b, x_buffer,
      //         onImprovCommandCallback, onImprovErrorCallback))
      //         {
      //             x_buffer[x_position++] = b;
      //         }
      //         else
      //         {
      //             x_position = 0;
      //         }
      //     }
      //     count++;

      //     if (count > 2000000) {
      //         queueLedEffect(LED_EFFECT_HEARTBEAT);
      //         count = 0;
      //     }
      // }
      // esp_task_wdt_deinit();
      // esp_task_wdt_reset();
    }



    setFgColor(preferences.getUInt("fgColor", isWhiteVersion() ? GxEPD_BLACK : GxEPD_WHITE));
    setBgColor(preferences.getUInt("bgColor", isWhiteVersion() ? GxEPD_WHITE : GxEPD_BLACK));
  }
  // else
  // {

  //   while (WiFi.status() != WL_CONNECTED)
  //   {
  //     vTaskDelay(pdMS_TO_TICKS(400));
  //   }
  // }
  // queueLedEffect(LED_EFFECT_WIFI_CONNECT_SUCCESS);
}

void syncTime()
{
  configTime(preferences.getInt("gmtOffset", DEFAULT_TIME_OFFSET_SECONDS), 0,
             NTP_SERVER);
  struct tm timeinfo;

  while (!getLocalTime(&timeinfo))
  {
    configTime(preferences.getInt("gmtOffset", DEFAULT_TIME_OFFSET_SECONDS), 0,
               NTP_SERVER);
    delay(500);
    Serial.println(F("Retry set time"));
  }

  lastTimeSync = esp_timer_get_time() / 1000000;
}

void setupPreferences()
{
  preferences.begin("btclock", false);

  setFgColor(preferences.getUInt("fgColor", DEFAULT_FG_COLOR));
  setBgColor(preferences.getUInt("bgColor", DEFAULT_BG_COLOR));
  setBlockHeight(preferences.getUInt("blockHeight", INITIAL_BLOCK_HEIGHT));
  setPrice(preferences.getUInt("lastPrice", INITIAL_LAST_PRICE), CURRENCY_USD);

  if (preferences.getBool("ownDataSource", DEFAULT_OWN_DATA_SOURCE))
    setCurrentCurrency(preferences.getUChar("lastCurrency", CURRENCY_USD));
  else
    setCurrentCurrency(CURRENCY_USD);

  if (!preferences.isKey("flDisable")) {
    preferences.putBool("flDisable", isWhiteVersion() ? false : true);
  }

  if (!preferences.isKey("fgColor")) {
    preferences.putUInt("fgColor", isWhiteVersion() ? GxEPD_BLACK : GxEPD_WHITE);
    preferences.putUInt("bgColor", isWhiteVersion() ? GxEPD_WHITE : GxEPD_BLACK);
  }
 

  addScreenMapping(SCREEN_BLOCK_HEIGHT, "Block Height");

  addScreenMapping(SCREEN_TIME, "Time");
  addScreenMapping(SCREEN_HALVING_COUNTDOWN, "Halving countdown");
  addScreenMapping(SCREEN_BLOCK_FEE_RATE, "Block Fee Rate");

  addScreenMapping(SCREEN_SATS_PER_CURRENCY, "Sats per dollar");
  addScreenMapping(SCREEN_BTC_TICKER, "Ticker");
  addScreenMapping(SCREEN_MARKET_CAP, "Market Cap");

  // addScreenMapping(SCREEN_SATS_PER_CURRENCY_USD, "Sats per USD");
  // addScreenMapping(SCREEN_BTC_TICKER_USD, "Ticker USD");
  // addScreenMapping(SCREEN_MARKET_CAP_USD, "Market Cap USD");

  // addScreenMapping(SCREEN_SATS_PER_CURRENCY_EUR, "Sats per EUR");
  // addScreenMapping(SCREEN_BTC_TICKER_EUR, "Ticker EUR");
  // addScreenMapping(SCREEN_MARKET_CAP_EUR, "Market Cap EUR");

  // screenNameMap[SCREEN_BLOCK_HEIGHT] = "Block Height";
  // screenNameMap[SCREEN_BLOCK_FEE_RATE] = "Block Fee Rate";
  // screenNameMap[SCREEN_SATS_PER_CURRENCY] = "Sats per dollar";
  // screenNameMap[SCREEN_BTC_TICKER] = "Ticker";
  // screenNameMap[SCREEN_TIME] = "Time";
  // screenNameMap[SCREEN_HALVING_COUNTDOWN] = "Halving countdown";
  // screenNameMap[SCREEN_MARKET_CAP] = "Market Cap";

  // addCurrencyMappings(getActiveCurrencies());

  if (preferences.getBool("bitaxeEnabled", DEFAULT_BITAXE_ENABLED))
  {
    addScreenMapping(SCREEN_BITAXE_HASHRATE, "BitAxe Hashrate");
    addScreenMapping(SCREEN_BITAXE_BESTDIFF, "BitAxe Best Difficulty");
  }
}

String replaceAmbiguousChars(String input)
{
  const char *ambiguous = "1IlO0";
  const char *replacements = "LKQM8";

  for (int i = 0; i < strlen(ambiguous); i++)
  {
    input.replace(ambiguous[i], replacements[i]);
  }

  return input;
}

// void addCurrencyMappings(const std::vector<std::string>& currencies)
// {
//     for (const auto& currency : currencies)
//     {
//         int satsPerCurrencyScreen;
//         int btcTickerScreen;
//         int marketCapScreen;

//         // Determine the corresponding screen IDs based on the currency code
//         if (currency == "USD")
//         {
//             satsPerCurrencyScreen = SCREEN_SATS_PER_CURRENCY_USD;
//             btcTickerScreen = SCREEN_BTC_TICKER_USD;
//             marketCapScreen = SCREEN_MARKET_CAP_USD;
//         }
//         else if (currency == "EUR")
//         {
//             satsPerCurrencyScreen = SCREEN_SATS_PER_CURRENCY_EUR;
//             btcTickerScreen = SCREEN_BTC_TICKER_EUR;
//             marketCapScreen = SCREEN_MARKET_CAP_EUR;
//         }
//         else if (currency == "GBP")
//         {
//             satsPerCurrencyScreen = SCREEN_SATS_PER_CURRENCY_GBP;
//             btcTickerScreen = SCREEN_BTC_TICKER_GBP;
//             marketCapScreen = SCREEN_MARKET_CAP_GBP;
//         }
//         else if (currency == "JPY")
//         {
//             satsPerCurrencyScreen = SCREEN_SATS_PER_CURRENCY_JPY;
//             btcTickerScreen = SCREEN_BTC_TICKER_JPY;
//             marketCapScreen = SCREEN_MARKET_CAP_JPY;
//         }
//         else if (currency == "AUD")
//         {
//             satsPerCurrencyScreen = SCREEN_SATS_PER_CURRENCY_AUD;
//             btcTickerScreen = SCREEN_BTC_TICKER_AUD;
//             marketCapScreen = SCREEN_MARKET_CAP_AUD;
//         }
//         else if (currency == "CAD")
//         {
//             satsPerCurrencyScreen = SCREEN_SATS_PER_CURRENCY_CAD;
//             btcTickerScreen = SCREEN_BTC_TICKER_CAD;
//             marketCapScreen = SCREEN_MARKET_CAP_CAD;
//         }
//         else
//         {
//             continue;  // Unknown currency, skip it
//         }

//         // Create the string locally to ensure it persists
//         std::string satsPerCurrencyString = "Sats per " + currency;
//         std::string btcTickerString = "Ticker " + currency;
//         std::string marketCapString = "Market Cap " + currency;

//         // Pass the c_str() to the function
//         addScreenMapping(satsPerCurrencyScreen, satsPerCurrencyString.c_str());
//         addScreenMapping(btcTickerScreen, btcTickerString.c_str());
//         addScreenMapping(marketCapScreen, marketCapString.c_str());
//     }
// }

void setupWebsocketClients(void *pvParameters)
{
  if (preferences.getBool("ownDataSource", DEFAULT_OWN_DATA_SOURCE))
  {
    setupV2Notify();
  }
  else
  {
    setupBlockNotify();
    setupPriceNotify();
  }

  vTaskDelete(NULL);
}

void setupTimers()
{
  xTaskCreate(setupTimeUpdateTimer, "setupTimeUpdateTimer", 2048, NULL,
              tskIDLE_PRIORITY, NULL);
  xTaskCreate(setupScreenRotateTimer, "setupScreenRotateTimer", 2048, NULL,
              tskIDLE_PRIORITY, NULL);
}

void finishSetup()
{
  if (preferences.getBool("ledStatus", DEFAULT_LED_STATUS))
  {
    restoreLedState();
  }
  else
  {
    clearLeds();
  }
}

std::vector<ScreenMapping> getScreenNameMap() { return screenMappings; }

void setupMcp()
{
#ifdef IS_BTCLOCK_S3
  const int mcp1AddrPins[] = {MCP1_A0_PIN, MCP1_A1_PIN, MCP1_A2_PIN};
  const int mcp1AddrValues[] = {LOW, LOW, LOW};

  const int mcp2AddrPins[] = {MCP2_A0_PIN, MCP2_A1_PIN, MCP2_A2_PIN};
  const int mcp2AddrValues[] = {LOW, LOW, HIGH};

  pinMode(MCP_RESET_PIN, OUTPUT);
  digitalWrite(MCP_RESET_PIN, HIGH);

  for (int i = 0; i < 3; ++i)
  {
    pinMode(mcp1AddrPins[i], OUTPUT);
    digitalWrite(mcp1AddrPins[i], mcp1AddrValues[i]);

    pinMode(mcp2AddrPins[i], OUTPUT);
    digitalWrite(mcp2AddrPins[i], mcp2AddrValues[i]);
  }

  digitalWrite(MCP_RESET_PIN, LOW);
  delay(30);
  digitalWrite(MCP_RESET_PIN, HIGH);
#endif
}

void setupHardware()
{
  if (!LittleFS.begin(true))
  {
    Serial.println(F("An Error has occurred while mounting LittleFS"));
  }

  if (HW_REV == "REV_B_EPD_2_13" && !isWhiteVersion()) {
    Serial.println(F("Black Rev B"));
  }

  if (!LittleFS.open("/index.html.gz", "r"))
  {
    Serial.println(F("Error loading WebUI"));
  }

  // if (!LittleFS.exists("/qr.txt"))
  // {
  //   File f = LittleFS.open("/qr.txt", "w");

  //   if(f) {

  //   } else {
  //     Serial.println(F("Can't write QR to FS"));
  //   }
  // }

  setupLeds();

  WiFi.setHostname(getMyHostname().c_str());
  if (!psramInit())
  {
    Serial.println(F("PSRAM not available"));
  }

  setupMcp();

  Wire.begin(I2C_SDA_PIN, I2C_SCK_PIN, 400000);

  if (!mcp1.begin_I2C(0x20))
  {
    Serial.println(F("Error MCP23017"));

    // while (1)
    //         ;
  }
  else
  {
    pinMode(MCP_INT_PIN, INPUT_PULLUP);
    mcp1.setupInterrupts(false, false, LOW);

    for (int i = 0; i < 4; i++)
    {
      mcp1.pinMode(i, INPUT_PULLUP);
      mcp1.setupInterruptPin(i, LOW);
    }
#ifndef IS_BTCLOCK_S3
    for (int i = 8; i <= 14; i++)
    {
      mcp1.pinMode(i, OUTPUT);
    }
#endif
  }

#ifdef IS_HW_REV_B
  pinMode(39, INPUT_PULLDOWN);

#endif

#ifdef IS_BTCLOCK_S3
  if (!mcp2.begin_I2C(0x21))
  {
    Serial.println(F("Error MCP23017"));

    // while (1)
    //         ;
  }
#endif

#ifdef HAS_FRONTLIGHT
  setupFrontlight();

  Wire.beginTransmission(0x5C);
  byte error = Wire.endTransmission();

  if (error == 0)
  {
    Serial.println(F("Found BH1750"));
    hasLuxSensor = true;
    bh1750.begin(BH1750::CONTINUOUS_LOW_RES_MODE, 0x5C);
  }
  else
  {
    Serial.println(F("BH1750 Not found"));
    hasLuxSensor = false;
  }
#endif
}

void WiFiEvent(WiFiEvent_t event, WiFiEventInfo_t info)
{
  static bool first_connect = true;

  Serial.printf("[WiFi-event] event: %d\n", event);

  switch (event)
  {
  case ARDUINO_EVENT_WIFI_READY:
    Serial.println(F("WiFi interface ready"));
    break;
  case ARDUINO_EVENT_WIFI_SCAN_DONE:
    Serial.println(F("Completed scan for access points"));
    break;
  case ARDUINO_EVENT_WIFI_STA_START:
    Serial.println(F("WiFi client started"));
    break;
  case ARDUINO_EVENT_WIFI_STA_STOP:
    Serial.println(F("WiFi clients stopped"));
    break;
  case ARDUINO_EVENT_WIFI_STA_CONNECTED:
    Serial.println(F("Connected to access point"));
    break;
  case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
  {
    if (!first_connect)
    {
      Serial.println(F("Disconnected from WiFi access point"));
      queueLedEffect(LED_EFFECT_WIFI_CONNECT_ERROR);
      uint8_t reason = info.wifi_sta_disconnected.reason;
      if (reason)
        Serial.printf("Disconnect reason: %s, ",
                      WiFi.disconnectReasonName((wifi_err_reason_t)reason));
    }
    break;
  }
  case ARDUINO_EVENT_WIFI_STA_AUTHMODE_CHANGE:
    Serial.println(F("Authentication mode of access point has changed"));
    break;
  case ARDUINO_EVENT_WIFI_STA_GOT_IP:
  {
    Serial.print("Obtained IP address: ");
    Serial.println(WiFi.localIP());
    if (!first_connect)
      queueLedEffect(LED_EFFECT_WIFI_CONNECT_SUCCESS);
    first_connect = false;
    break;
  }
  case ARDUINO_EVENT_WIFI_STA_LOST_IP:
    Serial.println(F("Lost IP address and IP address is reset to 0"));
    queueLedEffect(LED_EFFECT_WIFI_CONNECT_ERROR);
    WiFi.reconnect();
    break;
  case ARDUINO_EVENT_WIFI_AP_START:
    Serial.println(F("WiFi access point started"));
    break;
  case ARDUINO_EVENT_WIFI_AP_STOP:
    Serial.println(F("WiFi access point  stopped"));
    break;
  case ARDUINO_EVENT_WIFI_AP_STACONNECTED:
    Serial.println(F("Client connected"));
    break;
  case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED:
    Serial.println(F("Client disconnected"));
    break;
  case ARDUINO_EVENT_WIFI_AP_STAIPASSIGNED:
    Serial.println(F("Assigned IP address to client"));
    break;
  case ARDUINO_EVENT_WIFI_AP_PROBEREQRECVED:
    Serial.println(F("Received probe request"));
    break;
  case ARDUINO_EVENT_WIFI_AP_GOT_IP6:
    Serial.println(F("AP IPv6 is preferred"));
    break;
  case ARDUINO_EVENT_WIFI_STA_GOT_IP6:
    Serial.println(F("STA IPv6 is preferred"));
    break;
  default:
    break;
  }
}

String getMyHostname()
{
  uint8_t mac[6];
  // WiFi.macAddress(mac);
  esp_efuse_mac_get_default(mac);
  char hostname[15];
  String hostnamePrefix = preferences.getString("hostnamePrefix", DEFAULT_HOSTNAME_PREFIX);
  snprintf(hostname, sizeof(hostname), "%s-%02x%02x%02x", hostnamePrefix,
           mac[3], mac[4], mac[5]);
  return hostname;
}

uint getLastTimeSync()
{
  return lastTimeSync;
}

#ifdef HAS_FRONTLIGHT
void setupFrontlight()
{
  if (!flArray.begin(PCA9685_MODE1_AUTOINCR | PCA9685_MODE1_ALLCALL, PCA9685_MODE2_TOTEMPOLE))
  {
    Serial.println(F("FL driver error"));
    return;
  }
  Serial.println(F("FL driver active"));

  if (!preferences.isKey("flMaxBrightness"))
  {
    preferences.putUInt("flMaxBrightness", DEFAULT_FL_MAX_BRIGHTNESS);
  }
  if (!preferences.isKey("flEffectDelay"))
  {
    preferences.putUInt("flEffectDelay", DEFAULT_FL_EFFECT_DELAY);
  }

  if (!preferences.isKey("flFlashOnUpd"))
  {
    preferences.putBool("flFlashOnUpd", DEFAULT_FL_FLASH_ON_UPDATE);
  }
}

float getLightLevel()
{
  return bh1750.readLightLevel();
}

bool hasLightLevel()
{
  return hasLuxSensor;
}
#endif

String getHwRev()
{
#ifndef HW_REV
  return "REV_0";
#else
  return HW_REV;
#endif
}

bool isWhiteVersion()
{
#ifdef IS_HW_REV_B
  pinMode(39, INPUT_PULLDOWN);
  return digitalRead(39);
#else
  return false;
#endif
}

String getFsRev()
{
  File fsHash = LittleFS.open("/fs_hash.txt", "r");
  if (!fsHash)
  {
    Serial.println(F("Error loading WebUI"));
  }

  String ret = fsHash.readString();
  fsHash.close();
  return ret;
}

int findScreenIndexByValue(int value)
{
  for (int i = 0; i < screenMappings.size(); i++)
  {
    if (screenMappings[i].value == value)
    {
      return i;
    }
  }
  return -1; // Return -1 if value is not found
}

std::vector<std::string> getAvailableCurrencies()
{
  return {CURRENCY_CODE_USD, CURRENCY_CODE_EUR, CURRENCY_CODE_GBP, CURRENCY_CODE_JPY, CURRENCY_CODE_AUD, CURRENCY_CODE_CAD};
}

std::vector<std::string> getActiveCurrencies()
{
  std::vector<std::string> result;

  // Convert Arduino String to std::string
  std::string stdString = preferences.getString("actCurrencies", DEFAULT_ACTIVE_CURRENCIES).c_str();

  // Use a stringstream to split the string
  std::stringstream ss(stdString);
  std::string item;

  // Split the string by comma and add each part to the vector
  while (std::getline(ss, item, ','))
  {
    result.push_back(item);
  }
  return result;
}

bool isActiveCurrency(std::string &currency)
{
  std::vector<std::string> ac = getActiveCurrencies();
  if (std::find(ac.begin(), ac.end(), currency) != ac.end())
  {
    return true;
  }
  return false;
}

const char* getFirmwareFilename() {
    if (HW_REV == "REV_B_EPD_2_13") {
        return "btclock_rev_b_213epd_firmware.bin";
    } else if (HW_REV == "REV_A_EPD_2_13") {
        return "lolin_s3_mini_213epd_firmware.bin";
    } else if (HW_REV == "REV_A_EPD_2_9") {
        return "lolin_s3_mini_29epd_firmware.bin";
    } else {
        return "";
    }
}