#include <doctest/doctest.h>
#include <rtlog/rtlog.h>

namespace rtlog
{

namespace test
{

std::atomic<std::size_t> gSequenceNumber{ 0 };

constexpr auto MAX_LOG_MESSAGE_LENGTH = 256;
constexpr auto MAX_NUM_LOG_MESSAGES = 100;

enum class ExampleLogLevel
{
    Debug,
    Info,
    Warning,
    Error
};

const char* to_string(ExampleLogLevel level)
{
    switch (level)
    {
    case ExampleLogLevel::Debug:
        return "Debug";
    case ExampleLogLevel::Info:
        return "Info";
    case ExampleLogLevel::Warning:
        return "Warning";
    case ExampleLogLevel::Error:
        return "Error";
    default:
        return "Unknown";
    }
}

enum class ExampleLogRegion
{
    Engine,
    Game,
    Network,
    Audio
};

const char* to_string(ExampleLogRegion region)
{
    switch (region)
    {
    case ExampleLogRegion::Engine:
        return "Engine";
    case ExampleLogRegion::Game:
        return "Game";
    case ExampleLogRegion::Network:
        return "Network";
    case ExampleLogRegion::Audio:
        return "Audio";
    default:
        return "Unknown";
    }
}

struct ExampleLogData
{
    ExampleLogLevel level;
    ExampleLogRegion region;
};


void ExamplePrintMessage(const ExampleLogData& data, size_t sequenceNumber, const char* message)
{
    printf("[%s] (%s) {%lu}: %s\n", 
        rtlog::test::to_string(data.level), 
        rtlog::test::to_string(data.region), 
        sequenceNumber, 
        message);
}

}
}

using namespace rtlog::test;


TEST_CASE("Dummy test")
{
    CHECK(true);
}

TEST_CASE("Test rtlog")
{
    rtlog::Logger<ExampleLogData, MAX_NUM_LOG_MESSAGES, MAX_LOG_MESSAGE_LENGTH, gSequenceNumber> logger;
    logger.Log({ExampleLogLevel::Debug, ExampleLogRegion::Engine}, "Hello, world!");
    logger.Log({ExampleLogLevel::Info, ExampleLogRegion::Game}, "Hello, world!");
    logger.Log({ExampleLogLevel::Warning, ExampleLogRegion::Network}, "Hello, world!");
    logger.Log({ExampleLogLevel::Error, ExampleLogRegion::Audio}, "Hello, world!");

    logger.ProcessLog(ExamplePrintMessage);
    logger.ProcessLog(ExamplePrintMessage);
    logger.ProcessLog(ExamplePrintMessage);
    logger.ProcessLog(ExamplePrintMessage);
}
