#include "nostr_notify.hpp"

std::vector<nostr::NostrPool *> pools;
nostr::Transport *transport;
TaskHandle_t nostrTaskHandle = NULL;
boolean nostrIsConnected = false;

void setupNostrNotify()
{
    nostr::esp32::ESP32Platform::initNostr(false);
    time_t now;
    time(&now);
    struct tm *utcTimeInfo;
    utcTimeInfo = gmtime(&now);
    time_t utcNow = mktime(utcTimeInfo);
    time_t timestamp60MinutesAgo = utcNow - 3600;

    try
    {
        transport = nostr::esp32::ESP32Platform::getTransport();
        nostr::NostrPool *pool = new nostr::NostrPool(transport);
        String relay = preferences.getString("nostrRelay");
        String pubKey = preferences.getString("nostrPubKey");
        pools.push_back(pool);

        // JsonArray filter = {
        //       {"kinds", {"1"}},
        //       {"since", {String(timestamp60MinutesAgo)}},
        //       {"authors", {pubKey}},
        //       };

        //                 { "kinds", {"9735"}, {"limit",{"1"},
        //  {"#p", {"b5127a08cf33616274800a4387881a9f98e04b9c37116e92de5250498635c422"} },

        // Lets subscribe to the relay
        String subId = pool->subscribeMany(
            {relay},
            {// First filter
             {
                 {"kinds", {"9735"}},
                 {"limit", {"1"}},
                 {"since", {String(timestamp60MinutesAgo)}},
                 {"#p", {preferences.getString("nostrZapPubkey", DEFAULT_ZAP_NOTIFY_PUBKEY)}},
             },
             // Second filter
             {
                 {"kinds", {"1"}},
                 {"since", {String(timestamp60MinutesAgo)}},
                 {"authors", {pubKey}},
             }},
            handleNostrEventCallback,
            [&](const String &subId, const String &reason)
            {
                // This is the callback that will be called when the subscription is
                // closed
                Serial.println("Subscription closed: " + reason);
            },
            [&](const String &subId)
            {
                // This is the callback that will be called when the subscription is
                // EOSE
                Serial.println("Subscription EOSE: " + subId);
            });

        std::vector<nostr::NostrRelay *> *relays = pool->getConnectedRelays();
        for (nostr::NostrRelay *relay : *relays)
        {
            Serial.println("Registering to connection events of: " + relay->getUrl());
            relay->getConnection()->addConnectionStatusListener([&](const nostr::ConnectionStatus &status)
                                                                { 
                String sstatus="UNKNOWN";
                if(status==nostr::ConnectionStatus::CONNECTED){
                    nostrIsConnected = true;
                    sstatus="CONNECTED";
                }else if(status==nostr::ConnectionStatus::DISCONNECTED){
                    nostrIsConnected = false;
                    sstatus="DISCONNECTED";
                }else if(status==nostr::ConnectionStatus::ERROR){
                    sstatus = "ERROR";
                }
                Serial.println("Connection status changed: " + sstatus); });
        }
    }
    catch (const std::exception &e)
    {
        Serial.println("Error: " + String(e.what()));
    }
}

void nostrTask(void *pvParameters)
{
    int blockFetch = getBlockFetch();
    processNewBlock(blockFetch);

    while (1)
    {
        for (nostr::NostrPool *pool : pools)
        {
            // Run internal loop: refresh relays, complete pending connections, send
            // pending messages
            pool->loop();
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void setupNostrTask()
{
    xTaskCreate(nostrTask, "nostrTask", 16384, NULL, 10, &nostrTaskHandle);
}

boolean nostrConnected()
{
    return nostrIsConnected;
}

void handleNostrEventCallback(const String &subId, nostr::SignedNostrEvent *event)
{
    // Received events callback, we can access the event content with
    // event->getContent() Here you should handle the event, for this
    // test we will just serialize it and print to console
    JsonDocument doc;
    JsonArray arr = doc["data"].to<JsonArray>();
    event->toSendableEvent(arr);
    // Access the second element which is the object
    JsonObject obj = arr[1].as<JsonObject>();
    String json;

    // Serial.println(obj["kind"].as<String>());

    // Access the "tags" array
    JsonArray tags = obj["tags"].as<JsonArray>();

    // Flag to check if the tag was found
    bool tagFound = false;
    uint medianFee = 0;
    String typeValue;

    // Iterate over the tags array
    for (JsonArray tag : tags)
    {
        // Check if the tag is an array with two elements
        if (tag.size() == 2)
        {
            const char *key = tag[0];
            const char *value = tag[1];

            // Check if the key is "type" and the value is "priceUsd"
            if (strcmp(key, "type") == 0 && (strcmp(value, "priceUsd") == 0 || strcmp(value, "blockHeight") == 0))
            {
                typeValue = value;
                tagFound = true;
            }
            else if (strcmp(key, "medianFee") == 0)
            {
                medianFee = tag[1].as<uint>();
            }

            if (strcmp(key, "bolt11") == 0)
            {

                String text = String(getAmountInSatoshis(std::string(value)));

                std::size_t textLength = text.length();

                // Calculate the position where the digits should start
                // Account for the position of the "mdi:pickaxe" and the "GH/S" label
                std::size_t startIndex = NUM_SCREENS - textLength;

                std::array<String, NUM_SCREENS> textEpdContent = {"ZAP", "mdi-lnbolt", "", "", "", "", ""};

                // Insert the "mdi:pickaxe" icon just before the digits
                if (startIndex > 0 && preferences.getBool("useSatsSymbol", DEFAULT_USE_SATS_SYMBOL))
                {
                    textEpdContent[startIndex - 1] = "STS";
                }

                // Place the digits
                for (std::size_t i = 0; i < textLength; i++)
                {
                    textEpdContent[startIndex + i] = text.substring(i, i + 1);
                }

                uint64_t timerPeriod = 0;
                if (isTimerActive())
                {
                    // store timer periode before making inactive to prevent artifacts
                    timerPeriod = getTimerSeconds();
                    esp_timer_stop(screenRotateTimer);
                }
                setCurrentScreen(SCREEN_CUSTOM);

                setEpdContent(textEpdContent);
                vTaskDelay(pdMS_TO_TICKS(315 * NUM_SCREENS) + pdMS_TO_TICKS(250));
                queueLedEffect(LED_EFFECT_NOSTR_ZAP);
                if (timerPeriod > 0)
                {
                    esp_timer_start_periodic(screenRotateTimer,
                                             timerPeriod * usPerSecond);
                }
            }
        }
    }
    if (tagFound)
    {
        if (typeValue.equals("priceUsd"))
        {
            processNewPrice(obj["content"].as<uint>());
        }
        else if (typeValue.equals("blockHeight"))
        {
            processNewBlock(obj["content"].as<uint>());
        }

        if (medianFee != 0)
        {
            processNewBlockFee(medianFee);
        }
    }
}