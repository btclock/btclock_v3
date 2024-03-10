#include <array>
#include <string>
#include <cmath>
#include <cstdint>

#include "utils.hpp"

std::array<std::string, NUM_SCREENS> parsePriceData(std::uint32_t price, char currencySymbol);
std::array<std::string, NUM_SCREENS> parseSatsPerCurrency(std::uint32_t price, char currencySymbol, bool withSatsSymbol);
std::array<std::string, NUM_SCREENS> parseBlockHeight(std::uint32_t blockHeight);
std::array<std::string, NUM_SCREENS> parseHalvingCountdown(std::uint32_t blockHeight, bool asBlocks);
std::array<std::string, NUM_SCREENS> parseMarketCap(std::uint32_t blockHeight, std::uint32_t price, char currencySymbol, bool bigChars);
std::array<std::string, NUM_SCREENS> parseBlockFees(std::uint16_t blockFees);
