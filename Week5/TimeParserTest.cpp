#include <gtest/gtest.h>
#include "../TimeParser.h"
#include <cctype> 

// Testi: Merkkijono sisältää vain numeroita (hyvä ja huono tapaus)
TEST(TimeParserTest, OnlyDigits) {
    char valid_time[] = "123456";  // OK
    EXPECT_NE(time_parse(valid_time), TIME_VALUE_ERROR); // pitäisi olla OK

    char invalid_time[] = "12a456"; // sisältää 'a', ei hyväksytty
    EXPECT_EQ(time_parse(invalid_time), TIME_VALUE_ERROR);
}

// Testi: Null (tyhjä) merkkijono
TEST(TimeParserTest, NullString) {
    char* null_time = nullptr;
    EXPECT_EQ(time_parse(null_time), TIME_NULL_ERROR);
}

// Testi: Aika-arvo 0 sekuntia (esim. "000000") ei ole sallittu, koska ajastinkeskeytys olisi turha
TEST(TimeParserTest, ZeroTime) {
    char zero_time[] = "000000"; // 0 sekuntia
    EXPECT_EQ(time_parse(zero_time), TIME_ZERO_ERROR);
}

// Test suite: TimeParserTest
TEST(TimeParserTest, ValidTime) {
    char time[] = "000120";  // 1m 20s = 80
    EXPECT_EQ(time_parse(time), 80);
}

TEST(TimeParserTest, InvalidLength) {
    char time[] = "12345";
    EXPECT_EQ(time_parse(time), TIME_LEN_ERROR);
}

TEST(TimeParserTest, InvalidHour) {
    char time[] = "240000";
    EXPECT_EQ(time_parse(time), TIME_VALUE_ERROR);
}

TEST(TimeParserTest, InvalidMinute) {
    char time[] = "006000";
    EXPECT_EQ(time_parse(time), TIME_VALUE_ERROR);
}

TEST(TimeParserTest, InvalidSecond) {
    char time[] = "000060";
    EXPECT_EQ(time_parse(time), TIME_VALUE_ERROR);
}

TEST(TimeParserTest, BoundaryValid) {
    char time[] = "235959";  // 59m 59s = 3599
    EXPECT_EQ(time_parse(time), 3599);
}

// https://google.github.io/googletest/reference/testing.html
// https://google.github.io/googletest/reference/assertions.html
