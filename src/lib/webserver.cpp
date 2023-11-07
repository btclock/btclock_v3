#include "webserver.hpp"

AsyncWebServer server(80);

void setupWebserver()
{
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(200, "text/plain", "Hello, world");
    });

    server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request){
        AsyncResponseStream *response = request->beginResponseStream("application/json");
        StaticJsonDocument<128> root;
        
        root["currentPrice"] = getPrice();
        root["currentBlockHeight"] = getBlockHeight();
        root["espFreeHeap"] = ESP.getFreeHeap();
        root["espHeapSize"] = ESP.getHeapSize();
        root["espFreePsram"] = ESP.getFreePsram();
        root["espPsramSize"] = ESP.getPsramSize();
        
        serializeJson(root, *response);


        request->send(response);
    });

    server.onNotFound(onNotFound);

    server.begin();
}

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
