#include "config.hpp"

#define MAX_ATTEMPTS_WIFI_CONNECTION 20

Preferences preferences;
Adafruit_MCP23X17 mcp;
std::vector<std::string> screenNameMap(SCREEN_COUNT);

void setup()
{
    setupPreferences();
    setupHardware();
    setupDisplays();
    if (preferences.getBool("ledTestOnPower", true))
    {
        queueLedEffect(LED_POWER_TEST);
    }
    if (mcp.digitalRead(3) == LOW)
    {
        preferences.putBool("wifiConfigured", false);

        WiFi.eraseAP();
        queueLedEffect(LED_EFFECT_WIFI_ERASE_SETTINGS);
    }

    tryImprovSetup();

    setupWebserver();

    // setupWifi();
    setupTime();
    finishSetup();

    setupTasks();
    setupTimers();

    xTaskCreate(setupWebsocketClients, "setupWebsocketClients", 4096, NULL, tskIDLE_PRIORITY, NULL);

    setupButtonTask();
    setupOTA();

    waitUntilNoneBusy();
    forceFullRefresh();
}

void tryImprovSetup()
{
    WiFi.onEvent(WiFiEvent);

    if (!preferences.getBool("wifiConfigured", false))
    {
        setFgColor(GxEPD_BLACK);
        setBgColor(GxEPD_WHITE);
        queueLedEffect(LED_EFFECT_WIFI_WAIT_FOR_CONFIG);

        uint8_t x_buffer[16];
        uint8_t x_position = 0;

        // Hold second button to start QR code wifi config
        if (mcp.digitalRead(2) == LOW)
        {
            WiFiManager wm;

            byte mac[6];
            WiFi.macAddress(mac);
            String softAP_SSID = String("BTClock" + String(mac[5], 16) + String(mac[1], 16));
            WiFi.setHostname(softAP_SSID.c_str());
            String softAP_password = base64::encode(String(mac[2], 16) + String(mac[4], 16) + String(mac[5], 16) + String(mac[1], 16)).substring(2, 10);

            wm.setConfigPortalTimeout(preferences.getUInt("wpTimeout", 600));
            wm.setWiFiAutoReconnect(true);
            wm.setAPCallback([&](WiFiManager *wifiManager)
                             {

                Serial.printf("Entered config mode:ip=%s, ssid='%s', pass='%s'\n", 
                WiFi.softAPIP().toString().c_str(), 
                wifiManager->getConfigPortalSSID().c_str(), 
                softAP_password.c_str()); 
                delay(6000);

                const String qrText = "qrWIFI:S:" + wifiManager->getConfigPortalSSID() + ";T:WPA;P:" + softAP_password.c_str() + ";;";
                const String explainText = "*SSID: *\r\n" + wifiManager->getConfigPortalSSID() + "\r\n\r\n*Password:*\r\n" + softAP_password;
                std::array<String, NUM_SCREENS> epdContent = {"Welcome!", "", "To setup\r\nscan QR or\r\nconnect\r\nmanually", "", explainText, "", qrText};
                setEpdContent(epdContent);
                 });

            wm.setSaveConfigCallback([]()
                                     {
                preferences.putBool("wifiConfigured", true);

                delay(1000);
                // just restart after succes
                ESP.restart(); });

            bool ac = wm.autoConnect(softAP_SSID.c_str(), softAP_password.c_str());
        }
        else
        {
            waitUntilNoneBusy();
            std::array<String, NUM_SCREENS> epdContent = {"Welcome!", "", "Use\r\nweb-interface\r\nto configure", "", "Or restart\r\nwhile\r\nholding\r\n2nd button\r\r\nto start\r\n QR-config", "", ""};
            setEpdContent(epdContent);
            esp_task_wdt_init(30, false);
            while (WiFi.status() != WL_CONNECTED)
            {
                if (Serial.available() > 0)
                {
                    uint8_t b = Serial.read();

                    if (parse_improv_serial_byte(x_position, b, x_buffer, onImprovCommandCallback, onImprovErrorCallback))
                    {
                        x_buffer[x_position++] = b;
                    }
                    else
                    {
                        x_position = 0;
                    }
                }
            }
            esp_task_wdt_deinit();
            esp_task_wdt_reset();
        }
        setFgColor(preferences.getUInt("fgColor", DEFAULT_FG_COLOR));
        setBgColor(preferences.getUInt("bgColor", DEFAULT_BG_COLOR));
    }
    else
    {
        WiFi.setAutoConnect(true);
        WiFi.setAutoReconnect(true);
        WiFi.begin();

        while (WiFi.status() != WL_CONNECTED)
        {
            vTaskDelay(pdMS_TO_TICKS(400));
        }
    }
   // queueLedEffect(LED_EFFECT_WIFI_CONNECT_SUCCESS);
}

void setupTime()
{
    configTime(preferences.getInt("gmtOffset", TIME_OFFSET_SECONDS), 0, NTP_SERVER);
    struct tm timeinfo;

    while (!getLocalTime(&timeinfo))
    {
        configTime(preferences.getInt("gmtOffset", TIME_OFFSET_SECONDS), 0, NTP_SERVER);
        delay(500);
        Serial.println(F("Retry set time"));
    }
}

void setupPreferences()
{
    preferences.begin("btclock", false);


    setFgColor(preferences.getUInt("fgColor", DEFAULT_FG_COLOR));
    setBgColor(preferences.getUInt("bgColor", DEFAULT_BG_COLOR));
    setBlockHeight(preferences.getUInt("blockHeight", 816000));
    setPrice(preferences.getUInt("lastPrice", 30000));

    screenNameMap[SCREEN_BLOCK_HEIGHT] = "Block Height";
    screenNameMap[SCREEN_MSCW_TIME] = "Sats per dollar";
    screenNameMap[SCREEN_BTC_TICKER] = "Ticker";
    screenNameMap[SCREEN_TIME] = "Time";
    screenNameMap[SCREEN_HALVING_COUNTDOWN] = "Halving countdown";
    screenNameMap[SCREEN_MARKET_CAP] = "Market Cap";
}

void setupWebsocketClients(void *pvParameters)
{
    setupBlockNotify();

    if (preferences.getBool("fetchEurPrice", false)) {
        setupPriceFetchTask();
    } else {
        setupPriceNotify();
    }

    vTaskDelete(NULL);
}

void setupTimers()
{
    xTaskCreate(setupTimeUpdateTimer, "setupTimeUpdateTimer", 2048, NULL, tskIDLE_PRIORITY, NULL);
    xTaskCreate(setupScreenRotateTimer, "setupScreenRotateTimer", 2048, NULL, tskIDLE_PRIORITY, NULL);
}

void finishSetup()
{

    if (preferences.getBool("ledStatus", false))
    {
        setLights(preferences.getUInt("ledColor", 0xFFCC00));
    }
    else
    {
        clearLeds();
    }
}

std::vector<std::string> getScreenNameMap()
{
    return screenNameMap;
}

void setupHardware()
{
    setupLeds();

    WiFi.setHostname(getMyHostname().c_str());
    ;
    if (!psramInit())
      {
        Serial.println(F("PSRAM not available"));
    }

    Wire.begin(35, 36, 400000);

    if (!mcp.begin_I2C(0x20))
    {
        Serial.println(F("Error MCP23017"));

        // while (1)
        //         ;
    }
    else
    {
        pinMode(MCP_INT_PIN, INPUT_PULLUP);
        mcp.setupInterrupts(false, false, LOW);

        for (int i = 0; i < 4; i++)
        {
            mcp.pinMode(i, INPUT_PULLUP);
            mcp.setupInterruptPin(i, LOW);
        }
        for (int i = 8; i <= 14; i++)
        {
            mcp.pinMode(i, OUTPUT);
        }
    }
}

void improvGetAvailableWifiNetworks()
{
    int networkNum = WiFi.scanNetworks();

    for (int id = 0; id < networkNum; ++id)
    {
        std::vector<uint8_t> data = improv::build_rpc_response(
            improv::GET_WIFI_NETWORKS, {WiFi.SSID(id), String(WiFi.RSSI(id)), (WiFi.encryptionType(id) == WIFI_AUTH_OPEN ? "NO" : "YES")}, false);
        improv_send_response(data);
    }
    // final response
    std::vector<uint8_t> data =
        improv::build_rpc_response(improv::GET_WIFI_NETWORKS, std::vector<std::string>{}, false);
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
    return {
        // URL where user can finish onboarding or use device
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
            std::vector<uint8_t> data = improv::build_rpc_response(improv::GET_CURRENT_STATE, getLocalUrl(), false);
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

            // std::array<String, NUM_SCREENS> epdContent = {"S", "U", "C", "C", "E", "S", "S"};
            // setEpdContent(epdContent);

            preferences.putBool("wifiConfigured", true);

            improv_set_state(improv::STATE_PROVISIONED);
            std::vector<uint8_t> data = improv::build_rpc_response(improv::WIFI_SETTINGS, getLocalUrl(), false);
            improv_send_response(data);
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
        std::vector<std::string> infos = {
            // Firmware name
            "BTClock",
            // Firmware version
            "1.0.0",
            // Hardware chip/variant
            "ESP32S3",
            // Device name
            "BTClock"};
        std::vector<uint8_t> data = improv::build_rpc_response(improv::GET_DEVICE_INFO, infos, false);
        improv_send_response(data);
        break;
    }

    case improv::Command::GET_WIFI_NETWORKS:
    {
        improvGetAvailableWifiNetworks();
        // std::array<String, NUM_SCREENS> epdContent = {"W", "E", "B", "W", "I", "F", "I"};
        // setEpdContent(epdContent);
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

void WiFiEvent(WiFiEvent_t event)
{
    Serial.printf("[WiFi-event] event: %d\n", event);

    switch (event) {
        case ARDUINO_EVENT_WIFI_READY: 
            Serial.println("WiFi interface ready");
            break;
        case ARDUINO_EVENT_WIFI_SCAN_DONE:
            Serial.println("Completed scan for access points");
            break;
        case ARDUINO_EVENT_WIFI_STA_START:
            Serial.println("WiFi client started");
            break;
        case ARDUINO_EVENT_WIFI_STA_STOP:
            Serial.println("WiFi clients stopped");
            break;
        case ARDUINO_EVENT_WIFI_STA_CONNECTED:
            Serial.println("Connected to access point");
            break;
        case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
            Serial.println("Disconnected from WiFi access point");
            queueLedEffect(LED_EFFECT_WIFI_CONNECT_ERROR);
            break;
        case ARDUINO_EVENT_WIFI_STA_AUTHMODE_CHANGE:
            Serial.println("Authentication mode of access point has changed");
            break;
        case ARDUINO_EVENT_WIFI_STA_GOT_IP:
            Serial.print("Obtained IP address: ");
            Serial.println(WiFi.localIP());
            break;
        case ARDUINO_EVENT_WIFI_STA_LOST_IP:
            Serial.println("Lost IP address and IP address is reset to 0");
            queueLedEffect(LED_EFFECT_WIFI_CONNECT_ERROR);
            break;
        case ARDUINO_EVENT_WIFI_AP_START:
            Serial.println("WiFi access point started");
            break;
        case ARDUINO_EVENT_WIFI_AP_STOP:
            Serial.println("WiFi access point  stopped");
            break;
        case ARDUINO_EVENT_WIFI_AP_STACONNECTED:
            Serial.println("Client connected");
            break;
        case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED:
            Serial.println("Client disconnected");
            break;
        case ARDUINO_EVENT_WIFI_AP_STAIPASSIGNED:
            Serial.println("Assigned IP address to client");
            break;
        case ARDUINO_EVENT_WIFI_AP_PROBEREQRECVED:
            Serial.println("Received probe request");
            break;
        case ARDUINO_EVENT_WIFI_AP_GOT_IP6:
            Serial.println("AP IPv6 is preferred");
            break;
        case ARDUINO_EVENT_WIFI_STA_GOT_IP6:
            Serial.println("STA IPv6 is preferred");
            break;
        default: break;
    }}