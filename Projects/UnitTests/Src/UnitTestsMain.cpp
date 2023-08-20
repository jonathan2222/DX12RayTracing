#include <iostream>

#define RS_THROW_INSTEAD_OF_ASSERT

#include "RSEngine.h"
#include "Maths/RSVector.h"
#include "Maths/RSMatrix.h"
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

TEST_CASE("Vector", "[RSVector]")
{
    RS::Vec2i vecA;
    CHECK(vecA.x == 0);
    CHECK(vecA.y == 0);
    CHECK(vecA == RS::Vec2i::ZERO);
    vecA = {1, 2};
    CHECK(vecA.x == 1);
    CHECK(vecA.y == 2);
    CHECK(vecA.r == 1);
    CHECK(vecA.g == 2);
    CHECK(vecA.u == 1);
    CHECK(vecA.v == 2);
    CHECK(vecA == std::initializer_list<int32>{1, 2});
    vecA.x = vecA.y = 0;
    CHECK(vecA == RS::Vec2i::ZERO);

    RS::Vec2i vecB(2, 1);
    CHECK(vecB.x == 2);
    CHECK(vecB.y == 1);
    vecA += vecB;
    CHECK(vecA == RS::Vec2i{2, 1});
    vecA *= 2;
    CHECK(vecA == RS::Vec2i{ 4, 2 });
    vecA /= 2;
    CHECK(vecA == RS::Vec2i{ 2, 1 });
    vecA -= vecB;
    CHECK(vecA == RS::Vec2i::ZERO);

    vecA = { 2 };
    CHECK(vecA == RS::Vec2i{ 2, 2 });

    RS::Vec4i vecC(4, 3, 2, 1);
    CHECK(vecC == RS::Vec4i{ 4, 3, 2, 1 });

    RS::Vec4i vecD(vecA, 1);
    CHECK(vecD == RS::Vec4i{ 2, 2, 1, 1 });

    RS::Vec4i vecE(vecA, 1, 3);
    CHECK(vecE == RS::Vec4i{ 2, 2, 1, 3 });

    vecA.x = 1;
    RS::Vec4i vecF(vecA);
    CHECK(vecF == RS::Vec4i{ 1, 2, 2, 2 });

    std::string str = vecF;
    CHECK(str == "[1, 2, 2, 2]");
}

TEST_CASE("Matrix", "[RSMatrix]")
{
    RS::Mat2i matA;
    CHECK(matA.At(0, 0) == 0);
    CHECK(matA.At(1, 0) == 0);
    CHECK(matA.At(0, 1) == 0);
    CHECK(matA.At(1, 1) == 0);

    matA = RS::Mat2i::IDENTITY;
    CHECK(matA.At(0, 0) == 1);
    CHECK(matA.At(1, 0) == 0);
    CHECK(matA.At(0, 1) == 0);
    CHECK(matA.At(1, 1) == 1);
    RS::Mat2i identity =
        {
            1, 0,
            0, 1
        };
    CHECK(matA == identity);
    std::string str = matA.ToString(true);
    CHECK(str == "[ 1  0 ;  0  1 ]");

    RS::Mat3i matB = RS::Mat3i::IDENTITY;
    matB[0][2] = 1;
    RS::Mat4i matC(matB);
    RS::Mat4i matD =
    {
        1, 0, 0, 0,
        0, 1, 0, 0,
        1, 0, 1, 0,
        0, 0, 0, 0
    };
    CHECK(matC == matD);

    RS::Mat4i matDT = RS::Transpose(matD);
    RS::Mat4i matDTCopy =
    {
        1, 0, 1, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 0
    };
    CHECK(matDT == matDTCopy);

    RS::Mat23i matE =
    {
        2, 3, 4,
        1, 0, 0
    };
    RS::Mat32i matF =
    {
        0, 1000,
        1, 100,
        0, 10
    };
    RS::Mat2i matEF = matE * matF;
    RS::Mat2i matEFRes =
    {
       3, 2340,
       0, 1000
    };
    CHECK(matEF == matEFRes);

    RS::Mat2i mat2x2 =
    {
        3, 7,
        1, -4
    };
    int32 detOfMat2x2 = RS::Determinant(mat2x2);
    CHECK(detOfMat2x2 == -19);

    RS::Mat4i matG =
    {
        1, 1, 1, -1,
        1, 1, -1, 1,
        1, -1, 1, 1,
        -1, 1, 1, 1
    };

    RS::Mat3i matGMinor00 = RS::SubMatrix(matG, 0, 0);
    RS::Mat3i matGMinor00Res =
    {
        1, -1, 1,
        -1, 1, 1,
        1, 1, 1
    };
    CHECK(matGMinor00 == matGMinor00Res);

    int32 detOfMatGMinor00 = RS::Determinant(matGMinor00);
    CHECK(detOfMatGMinor00 == -4);

    int32 detOfMatG = RS::Determinant(matG);
    CHECK(detOfMatG == -16);

    RS::Mat2 mat2x2F =
    {
        -1.0f, 3.0f/2.0f,
        1.0f, -1.0f
    };
    RS::Mat2 mat2x2FInverse = RS::Inverse(mat2x2F);
    RS::Mat2 mat2x2FInverseRes =
    {
        2.0f, 3.0f,
        2.0f, 2.0f
    };
    CHECK(mat2x2FInverse == mat2x2FInverseRes);

    RS::Mat4 matGF = matG;
    RS::Mat4 matGInverse = RS::Inverse(matGF);
    RS::Mat4 matGInverseRes = matGF * 0.25f;
    CHECK(matGInverseRes == matGInverse);

    RS::Mat3 matH =
    {
        1.0f, -1.0f, 1.0f,
        -1.0f, 1.0f, 1.0f,
        1.0f, 2.0f, 1.0f
    };
    RS::Mat3 matHInverseRes =
    {
        1.0f/6.0f, -1.0f/2.0f, 1.0f/3.0f,
        -1.0f/3.0f, 0.0f, 1.0f/3.0f,
        1.0f/2.0f, 1.0f/2.0f, 0.0f
    };
    RS::Mat3 matHInverse = RS::Inverse(matH);
    CHECK(matHInverse == matHInverseRes);
}
