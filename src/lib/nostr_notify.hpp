#pragma once

#include "shared.hpp"

#include <ArduinoJson.h>
#include <nostrdisplay_handler.hpp>
#include <string>

#include "esp32/ESP32Platform.h"
#include "NostrEvent.h"
#include "NostrPool.h"

#include "price_notify.hpp"
#include "block_notify.hpp"
#include "lib/timers.hpp"

void setupNostrNotify(bool asDatasource, bool zapNotify);

void nostrTask(void *pvParameters);
void setupNostrTask();

boolean nostrConnected();
void handleNostrEventCallback(const String &subId, nostr::SignedNostrEvent *event);
void handleNostrZapCallback(const String &subId, nostr::SignedNostrEvent *event);

void onNostrSubscriptionClosed(const String &subId, const String &reason);
void onNostrSubscriptionEose(const String &subId);

time_t getMinutesAgo(int min);
void subscribeZaps(nostr::NostrPool *pool, const String &relay, int minutesAgo);