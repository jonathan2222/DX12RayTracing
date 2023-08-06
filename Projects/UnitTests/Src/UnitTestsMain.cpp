#include <iostream>

#define RS_THROW_INSTEAD_OF_ASSERT

#include "RSEngine.h"
#include "Catch2/catch_amalgamated.hpp"

void AssertTest()
{
    RS_ASSERT(false, "Error: {} {} {} [{}]", "This", "should", "throw", 256);
}

void AssertTestNoMessage()
{
    RS_ASSERT_NO_MSG(false, "Error: {} {} {} [{}]", "This", "should", "throw", 256);
}

TEST_CASE("Assert testing", "[RS_ASSERT]")
{
    REQUIRE_THROWS_WITH(AssertTest(), "Error: This should throw [256]");
    REQUIRE_THROWS_WITH(AssertTestNoMessage(), "Error");
}

TEST_CASE("Splitting strings", "[Utils::Split]")
{
    CHECK(RS::Utils::Split("Test.Hej.alla", '.') == std::vector<std::string>{"Test", "Hej", "alla"});
    CHECK(RS::Utils::Split("Test.Hej.alla.", '.') == std::vector<std::string>{"Test", "Hej", "alla"});
    CHECK(RS::Utils::Split("TestHejalla", '.') == std::vector<std::string>{"TestHejalla"});
    CHECK(RS::Utils::Split("..", '.') == std::vector<std::string>{});
    CHECK(RS::Utils::Split("hej..", '.') == std::vector<std::string>{"hej"});
    CHECK(RS::Utils::Split("..San", '.') == std::vector<std::string>{"San"});
    CHECK(RS::Utils::Split("hej..San", '.') == std::vector<std::string>{"hej", "San"});
    CHECK(RS::Utils::Split("", '.') == std::vector<std::string>{});
    CHECK(RS::Utils::Split("c", '.') == std::vector<std::string>{"c"});
    CHECK(RS::Utils::Split("ac", '.') == std::vector<std::string>{"ac"});
    CHECK(RS::Utils::Split(".a", '.') == std::vector<std::string>{"a"});
    CHECK(RS::Utils::Split("b.", '.') == std::vector<std::string>{"b"});

    CHECK(RS::Utils::Split("I would like this to be correct!", ' ') == std::vector<std::string>{"I", "would", "like", "this", "to", "be", "correct!"});
}

TEST_CASE("String comparisons", "[StringUtils.h]")
{
    SECTION("EndsWith")
    {
        CHECK(RS::Utils::EndsWith("", "") == true);
        CHECK(RS::Utils::EndsWith("./test/this/thing.png", ".png") == true);
        CHECK(RS::Utils::EndsWith("./test/this/thing.png", "thing.png") == true);

        CHECK(RS::Utils::EndsWith("", "san") == false);
        CHECK(RS::Utils::EndsWith("Hejsan", "") == false);
        CHECK(RS::Utils::EndsWith("Hejsan", "santamaria") == false);
        CHECK(RS::Utils::EndsWith("./test/this/thing.png", "thing") == false);
    }

    SECTION("StartsWith")
    {
        CHECK(RS::Utils::StartsWith("", "") == true);
        CHECK(RS::Utils::StartsWith("./test/this/thing.png", ".") == true);
        CHECK(RS::Utils::StartsWith("./test/this/thing.png", "./test") == true);

        CHECK(RS::Utils::StartsWith("", "san") == false);
        CHECK(RS::Utils::StartsWith("Hejsan", "") == false);
        CHECK(RS::Utils::StartsWith("Hejsan", "santamaria") == false);
        CHECK(RS::Utils::StartsWith("./test/this/thing.png", "thing") == false);
    }
}
