#pragma once

#include <string>
#include <cmath>
#include <cstdint>
#include <sstream>
#include <iomanip>

int modulo(int x,int N);

double getSupplyAtBlock(std::uint32_t blockNr);

std::string formatNumberWithSuffix(std::uint64_t num, int numCharacters = 4);
int64_t getAmountInSatoshis(std::string bolt11);