#include <data_handler.hpp>
#include <unity.h>

void setUp(void) {
    // set stuff up here
}

void tearDown(void) {
    // clean stuff up here
}

void test_sats_per_dollar(void) {
    std::array<std::string, NUM_SCREENS> output = parseSatsPerCurrency(37253, '$');
    TEST_ASSERT_EQUAL_STRING("MSCW/TIME", output[0].c_str());
    TEST_ASSERT_EQUAL_STRING("2", output[NUM_SCREENS-4].c_str());
    TEST_ASSERT_EQUAL_STRING("6", output[NUM_SCREENS-3].c_str());
    TEST_ASSERT_EQUAL_STRING("8", output[NUM_SCREENS-2].c_str());
    TEST_ASSERT_EQUAL_STRING("4", output[NUM_SCREENS-1].c_str());
}


void test_block_height_6screens(void) {
    std::array<std::string, NUM_SCREENS> output = parseBlockHeight(999999);
    TEST_ASSERT_EQUAL_STRING("BLOCK/HEIGHT", output[0].c_str());
    TEST_ASSERT_EQUAL_STRING("9", output[1].c_str());
}

void test_block_height_7screens(void) {
    std::array<std::string, NUM_SCREENS> output = parseBlockHeight(1000000);
    TEST_ASSERT_EQUAL_STRING("1", output[0].c_str());
    TEST_ASSERT_EQUAL_STRING("0", output[1].c_str());
}

void test_ticker_6screens(void) {
    std::array<std::string, NUM_SCREENS> output = parsePriceData(100000, '$');
    TEST_ASSERT_EQUAL_STRING("$", output[0].c_str());
    TEST_ASSERT_EQUAL_STRING("1", output[1].c_str());
}

void test_ticker_7screens(void) {
    std::array<std::string, NUM_SCREENS> output = parsePriceData(1000000, '$');
    TEST_ASSERT_EQUAL_STRING("1", output[0].c_str());
    TEST_ASSERT_EQUAL_STRING("0", output[1].c_str());
}

// not needed when using generate_test_runner.rb
int runUnityTests(void) {
    UNITY_BEGIN();
    RUN_TEST(test_sats_per_dollar);
    RUN_TEST(test_block_height_6screens);
    RUN_TEST(test_block_height_7screens);
    RUN_TEST(test_ticker_6screens);
    RUN_TEST(test_ticker_7screens);

    return UNITY_END();
}

extern "C" void app_main() {
  runUnityTests();
}
