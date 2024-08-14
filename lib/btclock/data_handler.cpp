#include "data_handler.hpp"
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/bind.h>
#endif

char getCurrencySymbol(char input)
{
    switch (input)
    {
    case CURRENCY_EUR:
        return '[';
        break;
    case CURRENCY_GBP:
        return '\\';
        break;
    case CURRENCY_JPY:
        return ']';
        break;
    case CURRENCY_AUD:
    case CURRENCY_CAD:
    case CURRENCY_USD:
        return '$';
        break;
    default:
        return input;
    }
}

std::string getCurrencyCode(char input)
{
    switch (input)
    {
    case CURRENCY_EUR:
        return "EUR";
        break;
    case CURRENCY_GBP:
        return "GBP";
        break;
    case CURRENCY_JPY:
        return "YEN";
        break;
    case CURRENCY_AUD:
        return "AUD";
        break;
    case CURRENCY_CHF:
        return "CHF";
        break;
    case CURRENCY_CAD:
        return "CAD";
        break;
    default:
        return "USD";
    }
}

std::array<std::string, NUM_SCREENS> parsePriceData(std::uint32_t price, char currencySymbol, bool useSuffixFormat)
{
    std::array<std::string, NUM_SCREENS> ret;
    std::string priceString;
    if (std::to_string(price).length() >= NUM_SCREENS || useSuffixFormat)
    {
        priceString = getCurrencySymbol(currencySymbol) + formatNumberWithSuffix(price, NUM_SCREENS - 2);
    }
    else
    {
        priceString = getCurrencySymbol(currencySymbol) + std::to_string(price);
    }
    std::uint32_t firstIndex = 0;
    if (priceString.length() < (NUM_SCREENS))
    {
        priceString.insert(priceString.begin(), NUM_SCREENS - priceString.length(), ' ');

        ret[0] = "BTC/" + getCurrencyCode(currencySymbol);


        firstIndex = 1;
    }

    for (std::uint32_t i = firstIndex; i < NUM_SCREENS; i++)
    {
        ret[i] = priceString[i];
    }

    return ret;
}

std::array<std::string, NUM_SCREENS> parseSatsPerCurrency(std::uint32_t price,char currencySymbol, bool withSatsSymbol)
{
    std::array<std::string, NUM_SCREENS> ret;
    std::string priceString = std::to_string(int(round(1 / float(price) * 10e7)));
    std::uint32_t firstIndex = 0;
    std::uint8_t insertSatSymbol = NUM_SCREENS - priceString.length() - 1;

    if (priceString.length() < (NUM_SCREENS))
    {
        priceString.insert(priceString.begin(), NUM_SCREENS - priceString.length(), ' ');

        if (currencySymbol != CURRENCY_USD)
            ret[0] = "SATS/" + getCurrencyCode(currencySymbol);
        else 
            ret[0] = "MSCW/TIME";

        firstIndex = 1;

        for (std::uint32_t i = firstIndex; i < NUM_SCREENS; i++)
        {
            ret[i] = priceString[i];
        }

        if (withSatsSymbol)
        {
            ret[insertSatSymbol] = "STS";
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

std::array<std::string, NUM_SCREENS> parseBlockFees(std::uint16_t blockFees)
{
    std::array<std::string, NUM_SCREENS> ret;
    std::string blockFeesString = std::to_string(blockFees);
    std::uint32_t firstIndex = 0;

    if (blockFeesString.length() < NUM_SCREENS)
    {
        blockFeesString.insert(blockFeesString.begin(), NUM_SCREENS - blockFeesString.length() - 1, ' ');
        ret[0] = "FEE/RATE";
        firstIndex = 1;
    }

    for (std::uint8_t i = firstIndex; i < NUM_SCREENS - 1; i++)
    {
        ret[i] = blockFeesString[i];
    }

    ret[NUM_SCREENS - 1] = "sat/vB";

    return ret;
}

std::array<std::string, NUM_SCREENS> parseHalvingCountdown(std::uint32_t blockHeight, bool asBlocks)
{
    std::array<std::string, NUM_SCREENS> ret;
    const std::uint32_t nextHalvingBlock = 210000 - (blockHeight % 210000);
    const std::uint32_t minutesToHalving = nextHalvingBlock * 10;

    if (asBlocks)
    {
        std::string blockNrString = std::to_string(nextHalvingBlock);
        std::uint32_t firstIndex = 0;

        if (blockNrString.length() < NUM_SCREENS)
        {
            blockNrString.insert(blockNrString.begin(), NUM_SCREENS - blockNrString.length(), ' ');
            ret[0] = "HAL/VING";
            firstIndex = 1;
        }

        for (std::uint32_t i = firstIndex; i < NUM_SCREENS; i++)
        {
            ret[i] = blockNrString[i];
        }
    }
    else
    {

        const int years = floor(minutesToHalving / 525600);
        const int days = floor((minutesToHalving - (years * 525600)) / (24 * 60));
        const int hours = floor((minutesToHalving - (years * 525600) - (days * (24 * 60))) / 60);
        const int mins = floor(minutesToHalving - (years * 525600) - (days * (24 * 60)) - (hours * 60));
        ret[0] = "BIT/COIN";
        ret[1] = "HAL/VING";
        ret[(NUM_SCREENS - 5)] = std::to_string(years) + "/YRS";
        ret[(NUM_SCREENS - 4)] = std::to_string(days) + "/DAYS";
        ret[(NUM_SCREENS - 3)] = std::to_string(hours) + "/HRS";
        ret[(NUM_SCREENS - 2)] = std::to_string(mins) + "/MINS";
        ret[(NUM_SCREENS - 1)] = "TO/GO";
    }

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
        // Serial.print("Market cap: ");
        // Serial.println(marketCap);
        std::string priceString = currencySymbol + formatNumberWithSuffix(marketCap, (NUM_SCREENS - 2));
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

#ifdef __EMSCRIPTEN__
emscripten::val arrayToStringArray(const std::array<std::string, NUM_SCREENS> &arr)
{
    emscripten::val jsArray = emscripten::val::array();
    for (const auto &str : arr)
    {
        jsArray.call<void>("push", str);
    }
    return jsArray;
}

emscripten::val vectorToStringArray(const std::vector<std::string> &vec)
{
    emscripten::val jsArray = emscripten::val::array();
    for (size_t i = 0; i < vec.size(); ++i)
    {
        jsArray.set(i, vec[i]);
    }
    return jsArray;
}

emscripten::val parseBlockHeightArray(std::uint32_t blockHeight)
{
    return arrayToStringArray(parseBlockHeight(blockHeight));
}

emscripten::val parsePriceDataArray(std::uint32_t price, const std::string &currencySymbol, bool useSuffixFormat = false)
{
    return arrayToStringArray(parsePriceData(price, currencySymbol[0], useSuffixFormat));
}

emscripten::val parseHalvingCountdownArray(std::uint32_t blockHeight, bool asBlocks)
{
    return arrayToStringArray(parseHalvingCountdown(blockHeight, asBlocks));
}

emscripten::val parseMarketCapArray(std::uint32_t blockHeight, std::uint32_t price, const std::string &currencySymbol, bool bigChars)
{
    return arrayToStringArray(parseMarketCap(blockHeight, price, currencySymbol[0], bigChars));
}

emscripten::val parseBlockFeesArray(std::uint16_t blockFees)
{
    return arrayToStringArray(parseBlockFees(blockFees));
}

emscripten::val parseSatsPerCurrencyArray(std::uint32_t price, const std::string &currencySymbol, bool withSatsSymbol)
{
    return arrayToStringArray(parseSatsPerCurrency(price, currencySymbol[0], withSatsSymbol));
}

EMSCRIPTEN_BINDINGS(my_module)
{
    //    emscripten::register_vector<std::string>("StringList");

    emscripten::function("parseBlockHeight", &parseBlockHeightArray);
    emscripten::function("parseHalvingCountdown", &parseHalvingCountdownArray);
    emscripten::function("parseMarketCap", &parseMarketCapArray);
    emscripten::function("parseBlockFees", &parseBlockFeesArray);
    emscripten::function("parseSatsPerCurrency", &parseSatsPerCurrencyArray);
    emscripten::function("parsePriceData", &parsePriceDataArray);

    emscripten::function("arrayToStringArray", &arrayToStringArray);
    emscripten::function("vectorToStringArray", &vectorToStringArray);
}
#endif