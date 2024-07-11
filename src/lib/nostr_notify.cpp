#include "nostr_notify.hpp"

std::vector<nostr::NostrPool *> pools;
nostr::Transport *transport;
TaskHandle_t nostrTaskHandle = NULL;

void setupNostrNotify()
{
    nostr::esp32::ESP32Platform::initNostr(false);

    

    try
    {
        transport = nostr::esp32::ESP32Platform::getTransport();
        nostr::NostrPool *pool = new nostr::NostrPool(transport);
        String relay = preferences.getString("nostrRelay");
        String pubKey = preferences.getString("nostrPubKey");
        pools.push_back(pool);
        // Lets subscribe to the relay
        String subId = pool->subscribeMany(
            {relay},
            {{// we set the filters here (see
              // https://github.com/nostr-protocol/nips/blob/master/01.md#from-client-to-relay-sending-events-and-creating-subscriptions)
              {"kinds", {"1"}},
              {"authors", {pubKey}}}},
            [&](const String &subId, nostr::SignedNostrEvent *event)
            {
                // Received events callback, we can access the event content with
                // event->getContent() Here you should handle the event, for this
                // test we will just serialize it and print to console
                JsonDocument doc;
                JsonArray arr = doc["data"].to<JsonArray>();
                event->toSendableEvent(arr);
                // Access the second element which is the object
                JsonObject obj = arr[1].as<JsonObject>();

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
            },
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