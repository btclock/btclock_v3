#include <Arduino.h>
#include <NostrEvent.h>
#include <NostrRelayManager.h>

#include "lib/config.hpp"
#include "lib/shared.hpp"
#include "lib/block_notify.hpp"
extern TaskHandle_t nostrSubscribeTaskHandle;

void okEvent(const std::string& key, const char* payload);
void nip01Event(const std::string& key, const char* payload);
void setupNostrSubscribeTask();
void taskNostrSubscribe(void *pvParameters);

void storeStringVector(const char* key, const std::vector<String>& vec);
std::vector<String> retrieveStringVector(const char* key);