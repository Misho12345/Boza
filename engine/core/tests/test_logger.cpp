#include <gtest/gtest.h>
#include <boza/core/Logger.hpp>

TEST(Logger, LoggerDoesNotThrow)
{
    EXPECT_NO_THROW({
        boza::Logger::info("Test info");
        boza::Logger::warn("Test warn");
        boza::Logger::error("Test error");
    });
}