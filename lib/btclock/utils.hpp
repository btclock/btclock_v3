#pragma once

#include <string>
#include <cmath>
#include <cstdint>

int modulo(int x,int N);

double getSupplyAtBlock(std::uint32_t blockNr);

std::string formatNumberWithSuffix(std::uint64_t  num);