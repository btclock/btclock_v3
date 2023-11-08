#include "utils.hpp"

int modulo(int x, int N)
{
    return (x % N + N) % N;
}

String getMyHostname() {
    byte mac[6];
    WiFi.macAddress(mac);
    return "btclock" + String(mac[4], 16) = String(mac[5], 16);
}