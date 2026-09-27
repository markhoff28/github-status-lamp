#include <Arduino.h>
#include <unity.h>
#include "status_logic.h"

void setUp() {}
void tearDown() {}
void valid_statuses() {
    TEST_ASSERT_EQUAL(static_cast<int>(GitHubStatus::Operational), static_cast<int>(parseStatus("{\"status\":{\"indicator\":\"none\"}}")));
    TEST_ASSERT_EQUAL(static_cast<int>(GitHubStatus::Minor), static_cast<int>(parseStatus("{\"status\":{\"indicator\":\"minor\"}}")));
    TEST_ASSERT_EQUAL(static_cast<int>(GitHubStatus::Major), static_cast<int>(parseStatus("{\"status\":{\"indicator\":\"major\"}}")));
    TEST_ASSERT_EQUAL(static_cast<int>(GitHubStatus::Critical), static_cast<int>(parseStatus("{\"status\":{\"indicator\":\"critical\"}}")));
}
void bad_responses_are_unknown() {
    const char* cases[] = {"", "<html>error</html>", "{}", "null", "[]", "{\"status\":null}",
        "{\"status\":{\"indicator\":null}}", "{\"status\":{\"indicator\":12}}",
        "{\"status\":{\"indicator\":\"new-value\"}}", "{\"status\":{\"indicator\":\"none\"}"};
    for (const char* input : cases)
        TEST_ASSERT_EQUAL(static_cast<int>(GitHubStatus::Unknown), static_cast<int>(parseStatus(input)));
}
void led_count_boundaries() {
    uint16_t count = 42;
    TEST_ASSERT_TRUE(parseLedCount("1", count)); TEST_ASSERT_EQUAL(1, count);
    TEST_ASSERT_TRUE(parseLedCount("300", count)); TEST_ASSERT_EQUAL(300, count);
    const char* bad[] = {nullptr, "", "0", "301", "1000", "-1", "2.5", "30x", " 30", "999999999999"};
    for (const char* input : bad) {
        TEST_ASSERT_FALSE(parseLedCount(input, count)); TEST_ASSERT_EQUAL(300, count);
    }
}
void stale_and_boot_status() {
    TEST_ASSERT_FALSE(statusIsFresh(false, 0, 0));
    TEST_ASSERT_TRUE(statusIsFresh(true, 100, 100));
    TEST_ASSERT_TRUE(statusIsFresh(true, 100, 120099));
    TEST_ASSERT_FALSE(statusIsFresh(true, 100, 120100));
    TEST_ASSERT_TRUE(statusIsFresh(true, UINT32_MAX - 50, 50));
    TEST_ASSERT_FALSE(statusIsFresh(true, UINT32_MAX - 50, 120000));
}
void setup() {
    delay(2000); // Allow the USB test runner to attach; no animation is running.
    UNITY_BEGIN();
    RUN_TEST(valid_statuses);
    RUN_TEST(bad_responses_are_unknown);
    RUN_TEST(led_count_boundaries);
    RUN_TEST(stale_and_boot_status);
    UNITY_END();
}
void loop() {}
