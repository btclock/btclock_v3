#include "bitaxe_handler.hpp"

std::array<std::string, NUM_SCREENS> parseBitaxeHashRate(std::string text)
{
    std::array<std::string, NUM_SCREENS> ret;
    ret.fill(""); // Initialize all elements to empty strings

    std::size_t textLength = text.length();

    // Calculate the position where the digits should start
    // Account for the position of the "mdi:pickaxe" and the "GH/S" label
    std::size_t startIndex = NUM_SCREENS - 1 - textLength;

    // Insert the "mdi:pickaxe" icon just before the digits
    if (startIndex > 0)
    {
        ret[startIndex - 1] = "mdi:pickaxe";
    }

    // Place the digits
    for (std::size_t i = 0; i < textLength; ++i)
    {
        ret[startIndex + i] = text.substr(i, 1);
    }

    ret[NUM_SCREENS - 1] = "GH/S";
    ret[0] = "BIT/AXE";

    return ret;
}

std::array<std::string, NUM_SCREENS> parseBitaxeBestDiff(std::string text)
{
    std::array<std::string, NUM_SCREENS> ret;
    std::uint32_t firstIndex = 0;

    if (text.length() < NUM_SCREENS)
    {
        text.insert(text.begin(), NUM_SCREENS - text.length(), ' ');
        ret[0] = "BIT/AXE";
        ret[1] = "mdi:rocket";
        firstIndex = 2;
    }

    for (std::uint8_t i = firstIndex; i < NUM_SCREENS; i++)
    {
        ret[i] = text[i];
    }

    return ret;
}
