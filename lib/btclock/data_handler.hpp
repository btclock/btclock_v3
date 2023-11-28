#include <array>
#include <string>
#include <cmath>
#include "utils.hpp"

std::array<std::string, NUM_SCREENS> parsePriceData(uint price, char currencySymbol);
std::array<std::string, NUM_SCREENS> parseSatsPerCurrency(uint price, char currencySymbol);
std::array<std::string, NUM_SCREENS> parseBlockHeight(uint blockHeight);
std::array<std::string, NUM_SCREENS> parseHalvingCountdown(uint blockHeight);
std::array<std::string, NUM_SCREENS> parseMarketCap(uint blockHeight, uint price, char currencySymbol, bool bigChars);