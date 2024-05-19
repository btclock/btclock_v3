#include "config.hpp"

#define MAX_ATTEMPTS_WIFI_CONNECTION 20

Preferences preferences;
Adafruit_MCP23X17 mcp1;
#ifdef IS_BTCLOCK_S3
Adafruit_MCP23X17 mcp2;
#endif

#ifdef HAS_FRONTLIGHT
PCA9685 flArray(PCA_I2C_ADDR);
#endif

std::vector<std::string> screenNameMap(SCREEN_COUNT);
std::mutex mcpMutex;
uint lastTimeSync;

void setup()
{
  setupHardware();

  setupPreferences();
  setupDisplays();
  if (preferences.getBool("ledTestOnPower", true))
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
  }

  tryImprovSetup();

  setupWebserver();

  // setupWifi();
  syncTime();
  finishSetup();

  setupTasks();
  setupTimers();

  xTaskCreate(setupWebsocketClients, "setupWebsocketClients", 4096, NULL,
              tskIDLE_PRIORITY, NULL);

  setupButtonTask();
  setupOTA();

  waitUntilNoneBusy();
  forceFullRefresh();
}

void tryImprovSetup()
{
  WiFi.onEvent(WiFiEvent);
  WiFi.setAutoConnect(true);
  WiFi.setAutoReconnect(true);
  WiFi.begin();
  if (preferences.getInt("txPower", 0))
  {
    if (WiFi.setTxPower(
            static_cast<wifi_power_t>(preferences.getInt("txPower", 0))))
    {
      Serial.printf("WiFi max tx power set to %d\n",
                    preferences.getInt("txPower", 0));
    }
  }

  // if (!preferences.getBool("wifiConfigured", false))
  {

    queueLedEffect(LED_EFFECT_WIFI_WAIT_FOR_CONFIG);

    uint8_t x_buffer[16];
    uint8_t x_position = 0;

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
      String softAP_password =
          base64::encode(String(mac[2], 16) + String(mac[4], 16) +
                         String(mac[5], 16) + String(mac[1], 16))
              .substring(2, 10);

      // wm.setConfigPortalTimeout(preferences.getUInt("wpTimeout", 600));
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
  configTime(preferences.getInt("gmtOffset", TIME_OFFSET_SECONDS), 0,
             NTP_SERVER);
  struct tm timeinfo;

  while (!getLocalTime(&timeinfo))
  {
    configTime(preferences.getInt("gmtOffset", TIME_OFFSET_SECONDS), 0,
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
  setBlockHeight(preferences.getUInt("blockHeight", 816000));
  setPrice(preferences.getUInt("lastPrice", 30000));

  screenNameMap[SCREEN_BLOCK_HEIGHT] = "Block Height";
  screenNameMap[SCREEN_BLOCK_FEE_RATE] = "Block Fee Rate";
  screenNameMap[SCREEN_MSCW_TIME] = "Sats per dollar";
  screenNameMap[SCREEN_BTC_TICKER] = "Ticker";
  screenNameMap[SCREEN_TIME] = "Time";
  screenNameMap[SCREEN_HALVING_COUNTDOWN] = "Halving countdown";
  screenNameMap[SCREEN_MARKET_CAP] = "Market Cap";
}

void setupWebsocketClients(void *pvParameters)
{
  setupBlockNotify();

  if (preferences.getBool("fetchEurPrice", false))
  {
    setupPriceFetchTask();
  }
  else
  {
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
  if (preferences.getBool("ledStatus", false))
  {
    restoreLedState();
  }
  else
  {
    clearLeds();
  }
}

std::vector<std::string> getScreenNameMap() { return screenNameMap; }

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
pinMode(39, INPUT_PULLUP);

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
#endif
}

void improvGetAvailableWifiNetworks()
{
  int networkNum = WiFi.scanNetworks();

  for (int id = 0; id < networkNum; ++id)
  {
    std::vector<uint8_t> data = improv::build_rpc_response(
        improv::GET_WIFI_NETWORKS,
        {WiFi.SSID(id), String(WiFi.RSSI(id)),
         (WiFi.encryptionType(id) == WIFI_AUTH_OPEN ? "NO" : "YES")},
        false);
    improv_send_response(data);
  }
  // final response
  std::vector<uint8_t> data = improv::build_rpc_response(
      improv::GET_WIFI_NETWORKS, std::vector<std::string>{}, false);
  improv_send_response(data);
}

bool improv_connectWifi(std::string ssid, std::string password)
{
  uint8_t count = 0;

  WiFi.begin(ssid.c_str(), password.c_str());

  while (WiFi.status() != WL_CONNECTED)
  {
    blinkDelay(500, 2);

    if (count > MAX_ATTEMPTS_WIFI_CONNECTION)
    {
      WiFi.disconnect();
      return false;
    }
    count++;
  }

  return true;
}

void onImprovErrorCallback(improv::Error err)
{
  blinkDelayColor(100, 1, 255, 0, 0);
  // pixels.setPixelColor(0, pixels.Color(255, 0, 0));
  // pixels.setPixelColor(1, pixels.Color(255, 0, 0));
  // pixels.setPixelColor(2, pixels.Color(255, 0, 0));
  // pixels.setPixelColor(3, pixels.Color(255, 0, 0));
  // pixels.show();
  // vTaskDelay(pdMS_TO_TICKS(100));

  // pixels.clear();
  // pixels.show();
  // vTaskDelay(pdMS_TO_TICKS(100));
}

std::vector<std::string> getLocalUrl()
{
  return {// URL where user can finish onboarding or use device
          // Recommended to use website hosted by device
          String("http://" + WiFi.localIP().toString()).c_str()};
}

bool onImprovCommandCallback(improv::ImprovCommand cmd)
{
  switch (cmd.command)
  {
  case improv::Command::GET_CURRENT_STATE:
  {
    if ((WiFi.status() == WL_CONNECTED))
    {
      improv_set_state(improv::State::STATE_PROVISIONED);
      std::vector<uint8_t> data = improv::build_rpc_response(
          improv::GET_CURRENT_STATE, getLocalUrl(), false);
      improv_send_response(data);
    }
    else
    {
      improv_set_state(improv::State::STATE_AUTHORIZED);
    }

    break;
  }

  case improv::Command::WIFI_SETTINGS:
  {
    if (cmd.ssid.length() == 0)
    {
      improv_set_error(improv::Error::ERROR_INVALID_RPC);
      break;
    }

    improv_set_state(improv::STATE_PROVISIONING);
    queueLedEffect(LED_EFFECT_WIFI_CONNECTING);

    if (improv_connectWifi(cmd.ssid, cmd.password))
    {
      queueLedEffect(LED_EFFECT_WIFI_CONNECT_SUCCESS);

      // std::array<String, NUM_SCREENS> epdContent = {"S", "U", "C", "C",
      // "E", "S", "S"}; setEpdContent(epdContent);

      preferences.putBool("wifiConfigured", true);

      improv_set_state(improv::STATE_PROVISIONED);
      std::vector<uint8_t> data = improv::build_rpc_response(
          improv::WIFI_SETTINGS, getLocalUrl(), false);
      improv_send_response(data);

      delay(2500);
      ESP.restart();
      setupWebserver();
    }
    else
    {
      queueLedEffect(LED_EFFECT_WIFI_CONNECT_ERROR);

      improv_set_state(improv::STATE_STOPPED);
      improv_set_error(improv::Error::ERROR_UNABLE_TO_CONNECT);
    }

    break;
  }

  case improv::Command::GET_DEVICE_INFO:
  {
    std::vector<std::string> infos = {// Firmware name
                                      "BTClock",
                                      // Firmware version
                                      "1.0.0",
                                      // Hardware chip/variant
                                      "ESP32S3",
                                      // Device name
                                      "BTClock"};
    std::vector<uint8_t> data =
        improv::build_rpc_response(improv::GET_DEVICE_INFO, infos, false);
    improv_send_response(data);
    break;
  }

  case improv::Command::GET_WIFI_NETWORKS:
  {
    improvGetAvailableWifiNetworks();
    // std::array<String, NUM_SCREENS> epdContent = {"W", "E", "B", "W", "I",
    // "F", "I"}; setEpdContent(epdContent);
    break;
  }

  default:
  {
    improv_set_error(improv::ERROR_UNKNOWN_RPC);
    return false;
  }
  }

  return true;
}

void improv_set_state(improv::State state)
{
  std::vector<uint8_t> data = {'I', 'M', 'P', 'R', 'O', 'V'};
  data.resize(11);
  data[6] = improv::IMPROV_SERIAL_VERSION;
  data[7] = improv::TYPE_CURRENT_STATE;
  data[8] = 1;
  data[9] = state;

  uint8_t checksum = 0x00;
  for (uint8_t d : data)
    checksum += d;
  data[10] = checksum;

  Serial.write(data.data(), data.size());
}

void improv_send_response(std::vector<uint8_t> &response)
{
  std::vector<uint8_t> data = {'I', 'M', 'P', 'R', 'O', 'V'};
  data.resize(9);
  data[6] = improv::IMPROV_SERIAL_VERSION;
  data[7] = improv::TYPE_RPC_RESPONSE;
  data[8] = response.size();
  data.insert(data.end(), response.begin(), response.end());

  uint8_t checksum = 0x00;
  for (uint8_t d : data)
    checksum += d;
  data.push_back(checksum);

  Serial.write(data.data(), data.size());
}

void improv_set_error(improv::Error error)
{
  std::vector<uint8_t> data = {'I', 'M', 'P', 'R', 'O', 'V'};
  data.resize(11);
  data[6] = improv::IMPROV_SERIAL_VERSION;
  data[7] = improv::TYPE_ERROR_STATE;
  data[8] = 1;
  data[9] = error;

  uint8_t checksum = 0x00;
  for (uint8_t d : data)
    checksum += d;
  data[10] = checksum;

  Serial.write(data.data(), data.size());
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
  String hostnamePrefix = preferences.getString("hostnamePrefix", "btclock");
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
  if (!flArray.begin(PCA9685_MODE1_AUTOINCR, PCA9685_MODE2_INVERT))
  {
    Serial.println(F("FL driver error"));
    return;
  }
  Serial.println(F("FL driver active"));
  flArray.setFrequency(1000);
  flArray.setOutputEnablePin(PCA_OE_PIN);
  flArray.setOutputEnable(true);
  delay(1000);
  flArray.setOutputEnable(false);
  if (!preferences.isKey("flMaxBrightness"))
  {
    preferences.putUInt("flMaxBrightness", 4095);
  }
  // Initialize all LEDs to off
  // for (int ledPin = 0; ledPin < NUM_SCREENS; ledPin++) {
  //   flArray.setPWM(ledPin, 0, 0); // Turn off LED
  // }
  flArray.allOFF();
}
#endif

String getHwRev() {
  #ifndef HW_REV
    return "REV_0";
  #else
    return HW_REV;
  #endif
}

bool isWhiteVersion() {
  #ifdef IS_HW_REV_B
  return digitalRead(39);
  #else
  return false;
  #endif
}