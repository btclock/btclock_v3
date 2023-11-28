#include <data_handler.hpp>
#include <unity.h>

void setUp(void) {
    // set stuff up here
}

void tearDown(void) {
    // clean stuff up here
}

void test_CorrectSatsPerDollarConversion(void) {
    std::array<std::string, NUM_SCREENS> output = parseSatsPerCurrency(37253, '$');
    TEST_ASSERT_EQUAL_STRING("MSCW/TIME", output[0].c_str());
    TEST_ASSERT_EQUAL_STRING("2", output[NUM_SCREENS-4].c_str());
    TEST_ASSERT_EQUAL_STRING("6", output[NUM_SCREENS-3].c_str());
    TEST_ASSERT_EQUAL_STRING("8", output[NUM_SCREENS-2].c_str());
    TEST_ASSERT_EQUAL_STRING("4", output[NUM_SCREENS-1].c_str());
}


void test_SixCharacterBlockHeight(void) {
    std::array<std::string, NUM_SCREENS> output = parseBlockHeight(999999);
    TEST_ASSERT_EQUAL_STRING("BLOCK/HEIGHT", output[0].c_str());
    TEST_ASSERT_EQUAL_STRING("9", output[1].c_str());
}

void test_SevenCharacterBlockHeight(void) {
    std::array<std::string, NUM_SCREENS> output = parseBlockHeight(1000000);
    TEST_ASSERT_EQUAL_STRING("1", output[0].c_str());
    TEST_ASSERT_EQUAL_STRING("0", output[1].c_str());
}

void test_PriceOf100kusd(void) {
    std::array<std::string, NUM_SCREENS> output = parsePriceData(100000, '$');
    TEST_ASSERT_EQUAL_STRING("$", output[0].c_str());
    TEST_ASSERT_EQUAL_STRING("1", output[1].c_str());
}

void test_PriceOf1MillionUsd(void) {
    std::array<std::string, NUM_SCREENS> output = parsePriceData(1000000, '$');
    TEST_ASSERT_EQUAL_STRING("BTC/USD", output[0].c_str());
    for (int i = 1; i <= NUM_SCREENS-3; i++) {
        TEST_ASSERT_EQUAL_STRING(" ", output[i].c_str());
    }
    TEST_ASSERT_EQUAL_STRING("1", output[NUM_SCREENS-2].c_str());
    TEST_ASSERT_EQUAL_STRING("M", output[NUM_SCREENS-1].c_str());
}

// not needed when using generate_test_runner.rb
int runUnityTests(void) {
    UNITY_BEGIN();
    RUN_TEST(test_CorrectSatsPerDollarConversion);
    RUN_TEST(test_SixCharacterBlockHeight);
    RUN_TEST(test_SevenCharacterBlockHeight);
    RUN_TEST(test_PriceOf100kusd);
    RUN_TEST(test_PriceOf1MillionUsd);

    return UNITY_END();
}

int main(void) {
  return runUnityTests();
}

extern "C" void app_main() {
  runUnityTests();
}
