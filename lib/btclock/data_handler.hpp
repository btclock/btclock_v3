#include <array>
#include <string>
#include <cmath>
#include <cstdint>

#include "utils.hpp"

const char CURRENCY_USD = '$';
const char CURRENCY_EUR = '[';
const char CURRENCY_GBP = ']';
const char CURRENCY_JPY = '^';
const char CURRENCY_AUD = '_';
const char CURRENCY_CAD = '`';

std::array<std::string, NUM_SCREENS> parsePriceData(std::uint32_t price, char currency, bool useSuffixFormat = false);
std::array<std::string, NUM_SCREENS> parseSatsPerCurrency(std::uint32_t price, char currencySymbol, bool withSatsSymbol);
std::array<std::string, NUM_SCREENS> parseBlockHeight(std::uint32_t blockHeight);
std::array<std::string, NUM_SCREENS> parseHalvingCountdown(std::uint32_t blockHeight, bool asBlocks);
std::array<std::string, NUM_SCREENS> parseMarketCap(std::uint32_t blockHeight, std::uint32_t price, char currencySymbol, bool bigChars);
std::array<std::string, NUM_SCREENS> parseBlockFees(std::uint16_t blockFees);

char getCurrencySymbol(char input);
std::string getCurrencyCode(char input);