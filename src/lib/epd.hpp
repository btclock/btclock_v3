#pragma once

#include <GxEPD2_BW.h>
#include <native_pin.hpp>
#include <mcp23x17_pin.hpp>
#include "shared.hpp"
#include "config.hpp"
#include "fonts/fonts.hpp"

void setupDisplays();
void taskEpd(void *pvParameters);

void splitText(const uint dispNum, String top, String bottom, bool partial);
void showDigit(const uint dispNum, char chr, bool partial, const GFXfont *font);
extern "C" void updateDisplay(void *pvParameters) noexcept;

int getBgColor();
int getFgColor();
void setBgColor(int color);
void setFgColor(int color);

void setEpdContent(std::array<String, NUM_SCREENS> newEpdContent);
std::array<String, NUM_SCREENS> getCurrentEpdContent();
