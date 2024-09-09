#include <data_handler.hpp>
#include <unity.h>

void setUp(void)
{
    // set stuff up here
}

void tearDown(void)
{
    // clean stuff up here
}

void test_CorrectSatsPerDollarConversion(void)
{
    std::array<std::string, NUM_SCREENS> output = parseSatsPerCurrency(37253, CURRENCY_USD, false);
    TEST_ASSERT_EQUAL_STRING("MSCW/TIME", output[0].c_str());
    TEST_ASSERT_EQUAL_STRING("2", output[NUM_SCREENS - 4].c_str());
    TEST_ASSERT_EQUAL_STRING("6", output[NUM_SCREENS - 3].c_str());
    TEST_ASSERT_EQUAL_STRING("8", output[NUM_SCREENS - 2].c_str());
    TEST_ASSERT_EQUAL_STRING("4", output[NUM_SCREENS - 1].c_str());
}

void test_CorrectSatsPerPoundConversion(void)
{
    std::array<std::string, NUM_SCREENS> output = parseSatsPerCurrency(37253, CURRENCY_GBP, false);
    TEST_ASSERT_EQUAL_STRING("SATS/GBP", output[0].c_str());
    TEST_ASSERT_EQUAL_STRING("2", output[NUM_SCREENS - 4].c_str());
    TEST_ASSERT_EQUAL_STRING("6", output[NUM_SCREENS - 3].c_str());
    TEST_ASSERT_EQUAL_STRING("8", output[NUM_SCREENS - 2].c_str());
    TEST_ASSERT_EQUAL_STRING("4", output[NUM_SCREENS - 1].c_str());
}

void test_SixCharacterBlockHeight(void)
{
    std::array<std::string, NUM_SCREENS> output = parseBlockHeight(999999);
    TEST_ASSERT_EQUAL_STRING("BLOCK/HEIGHT", output[0].c_str());
    TEST_ASSERT_EQUAL_STRING("9", output[1].c_str());
}

void test_SevenCharacterBlockHeight(void)
{
    std::array<std::string, NUM_SCREENS> output = parseBlockHeight(1000000);
    TEST_ASSERT_EQUAL_STRING("1", output[0].c_str());
    TEST_ASSERT_EQUAL_STRING("0", output[1].c_str());
}

void test_FeeRateDisplay(void)
{
    uint testValue = 21;
    std::array<std::string, NUM_SCREENS> output = parseBlockFees(static_cast<std::uint16_t>(testValue));
    TEST_ASSERT_EQUAL_STRING("FEE/RATE", output[0].c_str());
    TEST_ASSERT_EQUAL_STRING("2", output[NUM_SCREENS - 3].c_str());
    TEST_ASSERT_EQUAL_STRING("1", output[NUM_SCREENS - 2].c_str());
    TEST_ASSERT_EQUAL_STRING("sat/vB", output[NUM_SCREENS - 1].c_str());
}

void test_PriceOf100kusd(void)
{
    std::array<std::string, NUM_SCREENS> output = parsePriceData(100000, '$');
    TEST_ASSERT_EQUAL_STRING("$", output[0].c_str());
    TEST_ASSERT_EQUAL_STRING("1", output[1].c_str());
}

void test_PriceOf1MillionUsd(void)
{
    std::array<std::string, NUM_SCREENS> output = parsePriceData(1000000, '$');
    TEST_ASSERT_EQUAL_STRING("BTC/USD", output[0].c_str());

    TEST_ASSERT_EQUAL_STRING("1", output[NUM_SCREENS - 5].c_str());
    TEST_ASSERT_EQUAL_STRING(".", output[NUM_SCREENS - 4].c_str());
    TEST_ASSERT_EQUAL_STRING("0", output[NUM_SCREENS - 3].c_str());
    TEST_ASSERT_EQUAL_STRING("0", output[NUM_SCREENS - 2].c_str());
    TEST_ASSERT_EQUAL_STRING("M", output[NUM_SCREENS - 1].c_str());
}

void test_McapLowerUsd(void)
{
    std::array<std::string, NUM_SCREENS> output = parseMarketCap(810000, 26000, '$', true);
    TEST_ASSERT_EQUAL_STRING("USD/MCAP", output[0].c_str());

    //    TEST_ASSERT_EQUAL_STRING("$", output[NUM_SCREENS-6].c_str());
    TEST_ASSERT_EQUAL_STRING("$", output[NUM_SCREENS - 5].c_str());
    TEST_ASSERT_EQUAL_STRING("5", output[NUM_SCREENS - 4].c_str());
    TEST_ASSERT_EQUAL_STRING("0", output[NUM_SCREENS - 3].c_str());
    TEST_ASSERT_EQUAL_STRING("7", output[NUM_SCREENS - 2].c_str());
    TEST_ASSERT_EQUAL_STRING("B", output[NUM_SCREENS - 1].c_str());
}

void test_Mcap1TrillionUsd(void)
{
    std::array<std::string, NUM_SCREENS> output = parseMarketCap(831000, 52000, '$', true);
    TEST_ASSERT_EQUAL_STRING("USD/MCAP", output[0].c_str());

    TEST_ASSERT_EQUAL_STRING("$", output[NUM_SCREENS - 6].c_str());
    TEST_ASSERT_EQUAL_STRING("1", output[NUM_SCREENS - 5].c_str());
    TEST_ASSERT_EQUAL_STRING(".", output[NUM_SCREENS - 4].c_str());
    TEST_ASSERT_EQUAL_STRING("0", output[NUM_SCREENS - 3].c_str());
    TEST_ASSERT_EQUAL_STRING("2", output[NUM_SCREENS - 2].c_str());
    TEST_ASSERT_EQUAL_STRING("T", output[NUM_SCREENS - 1].c_str());
}

void test_Mcap1TrillionEur(void)
{
    std::array<std::string, NUM_SCREENS> output = parseMarketCap(831000, 52000, CURRENCY_EUR, true);
    TEST_ASSERT_EQUAL_STRING("EUR/MCAP", output[0].c_str());
    TEST_ASSERT_TRUE(CURRENCY_EUR == output[NUM_SCREENS - 6].c_str()[0]);
    TEST_ASSERT_EQUAL_STRING("1", output[NUM_SCREENS - 5].c_str());
    TEST_ASSERT_EQUAL_STRING(".", output[NUM_SCREENS - 4].c_str());
    TEST_ASSERT_EQUAL_STRING("0", output[NUM_SCREENS - 3].c_str());
    TEST_ASSERT_EQUAL_STRING("2", output[NUM_SCREENS - 2].c_str());
    TEST_ASSERT_EQUAL_STRING("T", output[NUM_SCREENS - 1].c_str());
}

void test_Mcap1TrillionJpy(void)
{
    std::array<std::string, NUM_SCREENS> output = parseMarketCap(831000, 52000, CURRENCY_JPY, true);
    TEST_ASSERT_EQUAL_STRING("JPY/MCAP", output[0].c_str());
    TEST_ASSERT_TRUE(CURRENCY_JPY == output[NUM_SCREENS - 6].c_str()[0]);
    TEST_ASSERT_EQUAL_STRING("1", output[NUM_SCREENS - 5].c_str());
    TEST_ASSERT_EQUAL_STRING(".", output[NUM_SCREENS - 4].c_str());
    TEST_ASSERT_EQUAL_STRING("0", output[NUM_SCREENS - 3].c_str());
    TEST_ASSERT_EQUAL_STRING("2", output[NUM_SCREENS - 2].c_str());
    TEST_ASSERT_EQUAL_STRING("T", output[NUM_SCREENS - 1].c_str());
}

// not needed when using generate_test_runner.rb
int runUnityTests(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_CorrectSatsPerDollarConversion);
    RUN_TEST(test_CorrectSatsPerPoundConversion);
    RUN_TEST(test_SixCharacterBlockHeight);
    RUN_TEST(test_SevenCharacterBlockHeight);
    RUN_TEST(test_FeeRateDisplay);
    RUN_TEST(test_PriceOf100kusd);
    RUN_TEST(test_McapLowerUsd);
    RUN_TEST(test_Mcap1TrillionUsd);
    RUN_TEST(test_Mcap1TrillionEur);
    RUN_TEST(test_Mcap1TrillionJpy);

    return UNITY_END();
}

int main(void)
{
    return runUnityTests();
}

extern "C" void app_main()
{
    runUnityTests();
}
