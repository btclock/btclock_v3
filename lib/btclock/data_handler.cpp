#include "data_handler.hpp"

std::array<std::string, NUM_SCREENS> parsePriceData(std::uint32_t price, char currencySymbol)
{
    std::array<std::string, NUM_SCREENS> ret;
    std::string priceString;
    if (std::to_string(price).length() >= NUM_SCREENS) {
        priceString = formatNumberWithSuffix(price);
    } else {
        priceString = currencySymbol + std::to_string(price);
    }
    std::uint32_t firstIndex = 0;
    if (priceString.length() < (NUM_SCREENS))
    {
        priceString.insert(priceString.begin(), NUM_SCREENS - priceString.length(), ' ');
        if (currencySymbol == '[')
        {
            ret[0] = "BTC/EUR";
        }
        else
        {
            ret[0] = "BTC/USD";
        }
        firstIndex = 1;
    }

    for (std::uint32_t i = firstIndex; i < NUM_SCREENS; i++)
    {
        ret[i] = priceString[i];
    }

    return ret;
}

std::array<std::string, NUM_SCREENS> parseSatsPerCurrency(std::uint32_t price, char currencySymbol)
{
    std::array<std::string, NUM_SCREENS> ret;
    std::string priceString = std::to_string(int(round(1 / float(price) * 10e7)));
    std::uint32_t firstIndex = 0;

    if (priceString.length() < (NUM_SCREENS))
    {
        priceString.insert(priceString.begin(), NUM_SCREENS - priceString.length(), ' ');
        if (currencySymbol == '[')
        {
            ret[0] = "SATS/EUR";
        }
        else
        {
            ret[0] = "MSCW/TIME";
        }
        firstIndex = 1;

        for (std::uint32_t i = firstIndex; i < NUM_SCREENS; i++)
        {
            ret[i] = priceString[i];
        }
    }
    return ret;
}

std::array<std::string, NUM_SCREENS> parseBlockHeight(std::uint32_t blockHeight)
{
    std::array<std::string, NUM_SCREENS> ret;
    std::string blockNrString = std::to_string(blockHeight);
    std::uint32_t firstIndex = 0;

    if (blockNrString.length() < NUM_SCREENS)
    {
        blockNrString.insert(blockNrString.begin(), NUM_SCREENS - blockNrString.length(), ' ');
        ret[0] = "BLOCK/HEIGHT";
        firstIndex = 1;
    }

    for (std::uint32_t i = firstIndex; i < NUM_SCREENS; i++)
    {
        ret[i] = blockNrString[i];
    }

    return ret;
}

std::array<std::string, NUM_SCREENS> parseHalvingCountdown(std::uint32_t blockHeight)
{
    std::array<std::string, NUM_SCREENS> ret;

    const std::uint32_t nextHalvingBlock = 210000 - (blockHeight % 210000);
    const std::uint32_t minutesToHalving = nextHalvingBlock * 10;

    const int years = floor(minutesToHalving / 525600);
    const int days = floor((minutesToHalving - (years * 525600)) / (24 * 60));
    const int hours = floor((minutesToHalving - (years * 525600) - (days * (24 * 60))) / 60);
    const int mins = floor(minutesToHalving - (years * 525600) - (days * (24 * 60)) - (hours * 60));
    ret[0] = "BIT/COIN";
    ret[1] = "HALV/ING";
    ret[(NUM_SCREENS - 5)] = std::to_string(years) + "/YRS";
    ret[(NUM_SCREENS - 4)] = std::to_string(days) + "/DAYS";
    ret[(NUM_SCREENS - 3)] = std::to_string(hours) + "/HRS";
    ret[(NUM_SCREENS - 2)] = std::to_string(mins) + "/MINS";
    ret[(NUM_SCREENS - 1)] = "TO/GO";

    return ret;
}

std::array<std::string, NUM_SCREENS> parseMarketCap(std::uint32_t blockHeight, std::uint32_t price, char currencySymbol, bool bigChars)
{
    std::array<std::string, NUM_SCREENS> ret;
    std::uint32_t firstIndex = 0;
    double supply = getSupplyAtBlock(blockHeight);
    int64_t marketCap = static_cast<std::int64_t>(supply * double(price));
    if (currencySymbol == '[')
    {
        ret[0] = "EUR/MCAP";
    }
    else
    {
        ret[0] = "USD/MCAP";
    }

    if (bigChars)
    {
        firstIndex = 1;

        std::string priceString = currencySymbol + formatNumberWithSuffix(marketCap);
        priceString.insert(priceString.begin(), NUM_SCREENS - priceString.length(), ' ');

        for (std::uint32_t i = firstIndex; i < NUM_SCREENS; i++)
        {
            ret[i] = priceString[i];
        }
    }
    else
    {
        std::string stringValue = std::to_string(marketCap);
        size_t mcLength = stringValue.length();
        size_t leadingSpaces = (3 - mcLength % 3) % 3;
        stringValue = std::string(leadingSpaces, ' ') + stringValue;

        std::uint32_t groups = (mcLength + leadingSpaces) / 3;

        if (groups < NUM_SCREENS)
        {
            firstIndex = 1;
        }

        for (int i = firstIndex; i < NUM_SCREENS - groups - 1; i++)
        {
            ret[i] = "";
        }

        ret[NUM_SCREENS - groups - 1] = " $ ";
        for (std::uint32_t i = 0; i < groups; i++)
        {
            ret[(NUM_SCREENS - groups + i)] = stringValue.substr(i * 3, 3).c_str();
        }
    }

    return ret;
}