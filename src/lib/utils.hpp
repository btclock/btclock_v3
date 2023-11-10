#pragma once

#include <WiFi.h>
#include "shared.hpp"

int modulo(int x,int N);

double getSupplyAtBlock(uint blockNr);

String getMyHostname();
std::string formatNumberWithSuffix(int64_t num);