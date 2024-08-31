#include "nostrdisplay_handler.hpp"

std::array<std::string, NUM_SCREENS> parseZapNotify(std::uint16_t amount, bool withSatsSymbol)
{
    std::string text = std::to_string(amount);
    std::size_t textLength = text.length();
    std::size_t startIndex = NUM_SCREENS - textLength;

    std::array<std::string, NUM_SCREENS> textEpdContent = {"ZAP", "mdi-lnbolt", "", "", "", "", ""};

    // Insert the sats symbol just before the digits
    if (startIndex > 0 && withSatsSymbol)
    {
        textEpdContent[startIndex - 1] = "STS";
    }

    // Place the digits
    for (std::size_t i = 0; i < textLength; i++)
    {
        textEpdContent[startIndex + i] = text.substr(i, 1);
    }

    return textEpdContent;
}