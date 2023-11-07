#include "config.hpp"

#ifndef NEOPIXEL_PIN
#define NEOPIXEL_PIN 34
#endif
#ifndef NEOPIXEL_COUNT
#define NEOPIXEL_COUNT 4
#endif

#define MAX_ATTEMPTS_WIFI_CONNECTION 20

Preferences preferences;
Adafruit_MCP23X17 mcp;
Adafruit_NeoPixel pixels(NEOPIXEL_COUNT, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

void setup()
{
    setupHardware();
    if (mcp.digitalRead(3) == LOW) {
        WiFi.eraseAP();
        blinkDelay(100, 3);
    }

    tryImprovSetup();

    setupDisplays();
    setupPreferences();
    setupWebserver();

    // setupWifi();
    setupTime();
    finishSetup();

    setupTasks();
    setupTimers();
    setupWebsocketClients();

    setupButtonTask();
}

void tryImprovSetup()
{
    // if (mcp.digitalRead(3) == LOW)
    // {
    //     WiFi.persistent(false);
    //     blinkDelay(100, 3);
    // }
    // else
    // {
    //     WiFi.begin();
    // }

    uint8_t x_buffer[16];
    uint8_t x_position = 0;

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
        vTaskDelay(1);
    }
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

    Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
}

void setupPreferences()
{
    preferences.begin("btclock", false);
}

void setupWebsocketClients()
{
    setupBlockNotify();
    setupPriceNotify();
}

void setupTimers()
{
    xTaskCreate(setupTimeUpdateTimer, "setupTimeUpdateTimer", 4096, NULL, 1, NULL);
    xTaskCreate(setupScreenRotateTimer, "setupScreenRotateTimer", 4096, NULL, 1, NULL);
}

void finishSetup()
{
    pixels.clear();
    pixels.show();
}

void setupHardware()
{
    pixels.begin();
    pixels.setPixelColor(0, pixels.Color(255, 0, 0));
    pixels.setPixelColor(1, pixels.Color(0, 255, 0));
    pixels.setPixelColor(2, pixels.Color(0, 0, 255));
    pixels.setPixelColor(3, pixels.Color(255, 255, 255));
    pixels.show();

    if (psramInit())
    {
        Serial.println(F("PSRAM is correctly initialized"));
    }
    else
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

void blinkDelay(int d, int times)
{
    for (int j = 0; j < times; j++)
    {

        pixels.setPixelColor(0, pixels.Color(255, 0, 0));
        pixels.setPixelColor(1, pixels.Color(0, 255, 0));
        pixels.setPixelColor(2, pixels.Color(255, 0, 0));
        pixels.setPixelColor(3, pixels.Color(0, 255, 0));
        pixels.show();
        vTaskDelay(pdMS_TO_TICKS(d));

        pixels.setPixelColor(0, pixels.Color(255, 255, 0));
        pixels.setPixelColor(1, pixels.Color(0, 255, 255));
        pixels.setPixelColor(2, pixels.Color(255, 255, 0));
        pixels.setPixelColor(3, pixels.Color(0, 255, 255));
        pixels.show();
        vTaskDelay(pdMS_TO_TICKS(d));
    }
    pixels.clear();
    pixels.show();
}

void onImprovErrorCallback(improv::Error err)
{
    pixels.setPixelColor(0, pixels.Color(255, 0, 0));
    pixels.setPixelColor(1, pixels.Color(255, 0, 0));
    pixels.setPixelColor(2, pixels.Color(255, 0, 0));
    pixels.setPixelColor(3, pixels.Color(255, 0, 0));
    pixels.show();
    vTaskDelay(pdMS_TO_TICKS(100));

    pixels.clear();
    pixels.show();
    vTaskDelay(pdMS_TO_TICKS(100));
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

        if (improv_connectWifi(cmd.ssid, cmd.password))
        {
            
            blinkDelay(100, 3);
            // std::array<String, NUM_SCREENS> epdContent = {"S", "U", "C", "C", "E", "S", "S"};
            // setEpdContent(epdContent);

            improv_set_state(improv::STATE_PROVISIONED);
            std::vector<uint8_t> data = improv::build_rpc_response(improv::WIFI_SETTINGS, getLocalUrl(), false);
            improv_send_response(data);
            setupWebserver();
        }
        else
        {
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