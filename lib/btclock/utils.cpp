#include "utils.hpp"

int modulo(int x, int N)
{
    return (x % N + N) % N;
}

double getSupplyAtBlock(std::uint32_t blockNr)
{
    if (blockNr >= 33 * 210000)
    {
        return 20999999.9769;
    }

    const int initialBlockReward = 50;  // Initial block reward
    const int halvingInterval = 210000; // Number of blocks before halving

    int halvingCount = blockNr / halvingInterval;
    double totalBitcoinInCirculation = 0;

    for (int i = 0; i < halvingCount; ++i)
    {
        totalBitcoinInCirculation += halvingInterval * initialBlockReward * std::pow(0.5, i);
    }

    totalBitcoinInCirculation += (blockNr % halvingInterval) * initialBlockReward * std::pow(0.5, halvingCount);

    return totalBitcoinInCirculation;
}

std::string formatNumberWithSuffix(std::uint64_t num, int numCharacters)
{
    static char result[20]; // Adjust size as needed
    const long long quadrillion = 1000000000000000LL;
    const long long trillion = 1000000000000LL;
    const long long billion = 1000000000;
    const long long million = 1000000;
    const long long thousand = 1000;

    double numDouble = (double)num;
    int numDigits = (int)log10(num) + 1;
    char suffix;

    if (num >= quadrillion || numDigits > 15)
    {
        numDouble /= quadrillion;
        suffix = 'Q';
    }
    else if (num >= trillion || numDigits > 12)
    {
        numDouble /= trillion;
        suffix = 'T';
    }
    else if (num >= billion || numDigits > 9)
    {
        numDouble /= billion;
        suffix = 'B';
    }
    else if (num >= million || numDigits > 6)
    {
        numDouble /= million;
        suffix = 'M';
    }
    else if (num >= thousand || numDigits > 3)
    {
        numDouble /= thousand;
        suffix = 'K';
    }
    else
    {
        snprintf(result, sizeof(result), "%llu", (unsigned long long)num);
//        sprintf(result, "%llu", (unsigned long long)num);
        return result;
    }

    // Add suffix
    int len = snprintf(result, sizeof(result), "%.0f%c", numDouble, suffix);

    // If there's room, add decimal places
    if (len < numCharacters)
    {
        snprintf(result, sizeof(result), "%.*f%c", numCharacters - len - 1, numDouble, suffix);
    }

    return result;
}

/**
 * Get sat amount from a bolt11 invoice 
 * 
 * Based on https://github.com/lnbits/nostr-zap-lamp/blob/main/nostrZapLamp/nostrZapLamp.ino
 */
int64_t getAmountInSatoshis(std::string bolt11) {
    int64_t number = -1;
    char multiplier = ' ';

    for (unsigned int i = 0; i < bolt11.length(); ++i) {
        if (isdigit(bolt11[i])) {
            number = 0;
            while (isdigit(bolt11[i])) {
                number = number * 10 + (bolt11[i] - '0');
                ++i;
            }
            for (unsigned int j = i; j < bolt11.length(); ++j) {
                if (isalpha(bolt11[j])) {
                    multiplier = bolt11[j];
                    break;
                }
            }
            break;
        }
    }

    if (number == -1 || multiplier == ' ') {
        return -1;
    }

    int64_t satoshis = number;

    switch (multiplier) {
        case 'm':
            satoshis *= 100000; // 0.001 * 100,000,000
            break;
        case 'u':
            satoshis *= 100; // 0.000001 * 100,000,000
            break;
        case 'n':
            satoshis /= 10; // 0.000000001 * 100,000,000
            break;
        case 'p':
            satoshis /= 10000; // 0.000000000001 * 100,000,000
            break;
        default:
            return -1;
    }

    return satoshis;
}
