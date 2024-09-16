#include "nostr_notify.hpp"

std::vector<nostr::NostrPool *> pools;
nostr::Transport *transport;
TaskHandle_t nostrTaskHandle = NULL;
boolean nostrIsConnected = false;

void setupNostrNotify(bool asDatasource, bool zapNotify)
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

        std::vector<std::map<NostrString, std::initializer_list<NostrString>>> filters;

        if (zapNotify)
        {
            String subIdZap = pool->subscribeMany(
                {relay},
                {
                    {
                        {"kinds", {"9735"}},
                        {"limit", {"1"}},
                        {"since", {String(timestamp60MinutesAgo)}},
                        {"#p", {preferences.getString("nostrZapPubkey", DEFAULT_ZAP_NOTIFY_PUBKEY)}},
                    },
                },
                handleNostrZapCallback,
                onNostrSubscriptionClosed,
                onNostrSubscriptionEose);
            Serial.println("[ Nostr ] Subscribing to Zap Notifications");
        }

        if (asDatasource)
        {
            String subId = pool->subscribeMany(
                {relay},
                {// First filter
                 {
                     {"kinds", {"1"}},
                     {"since", {String(timestamp60MinutesAgo)}},
                     {"authors", {pubKey}},
                 }},
                handleNostrEventCallback,
                onNostrSubscriptionClosed,
                onNostrSubscriptionEose);

            Serial.println("[ Nostr ] Subscribing to Nostr Data Feed");
        }

        std::vector<nostr::NostrRelay *> *relays = pool->getConnectedRelays();
        for (nostr::NostrRelay *relay : *relays)
        {
            Serial.println("[ Nostr ] Registering to connection events of: " + relay->getUrl());
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
                //Serial.println("[ Nostr ] Connection status changed: " + sstatus); 
                });
        }
    }
    catch (const std::exception &e)
    {
        Serial.println("[ Nostr ] Error: " + String(e.what()));
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

void onNostrSubscriptionClosed(const String &subId, const String &reason)
{
    // This is the callback that will be called when the subscription is
    // closed
    Serial.println("[ Nostr ] Subscription closed: " + reason);
}

void onNostrSubscriptionEose(const String &subId)
{
    // This is the callback that will be called when the subscription is
    // EOSE
    Serial.println("[ Nostr ] Subscription EOSE: " + subId);
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
        }
    }
    if (tagFound)
    {
        if (typeValue.equals("priceUsd"))
        {
            processNewPrice(obj["content"].as<uint>(), CURRENCY_USD);
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

void handleNostrZapCallback(const String &subId, nostr::SignedNostrEvent *event) {
    // Received events callback, we can access the event content with
    // event->getContent() Here you should handle the event, for this
    // test we will just serialize it and print to console
    JsonDocument doc;
    JsonArray arr = doc["data"].to<JsonArray>();
    event->toSendableEvent(arr);
    // Access the second element which is the object
    JsonObject obj = arr[1].as<JsonObject>();
    JsonArray tags = obj["tags"].as<JsonArray>();

    // Iterate over the tags array
    for (JsonArray tag : tags)
    {
        // Check if the tag is an array with two elements
        if (tag.size() == 2)
        {
            const char *key = tag[0];
            const char *value = tag[1];

            if (strcmp(key, "bolt11") == 0)
            {
                Serial.print(F("Got a zap of "));
                
                int64_t satsAmount = getAmountInSatoshis(std::string(value));
                Serial.print(satsAmount);
                Serial.println(F(" sats"));

                std::array<std::string, NUM_SCREENS> textEpdContent = parseZapNotify(satsAmount, preferences.getBool("useSatsSymbol", DEFAULT_USE_SATS_SYMBOL));

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
}