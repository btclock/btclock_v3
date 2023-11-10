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

double getSupplyAtBlock(uint blockNr) {
    if (blockNr >= 33 * 210000) {
        return 20999999.9769;
    } 

    const int initialBlockReward = 50;  // Initial block reward
    const int halvingInterval = 210000;  // Number of blocks before halving

    int halvingCount = blockNr / halvingInterval;
    double totalBitcoinInCirculation = 0;

    for (int i = 0; i < halvingCount; ++i) {
        totalBitcoinInCirculation += halvingInterval * initialBlockReward * std::pow(0.5, i);
    }

    totalBitcoinInCirculation += (blockNr % halvingInterval) * initialBlockReward * std::pow(0.5, halvingCount);

    return totalBitcoinInCirculation;
}