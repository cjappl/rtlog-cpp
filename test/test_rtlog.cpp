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
    Critical
};

const char* to_string(ExampleLogLevel level)
{
    switch (level)
    {
    case ExampleLogLevel::Debug:
        return "DEBG";
    case ExampleLogLevel::Info:
        return "INFO";
    case ExampleLogLevel::Warning:
        return "WARN";
    case ExampleLogLevel::Critical:
        return "CRIT";
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
        return "ENGIN";
    case ExampleLogRegion::Game:
        return "GAME ";
    case ExampleLogRegion::Network:
        return "NETWK";
    case ExampleLogRegion::Audio:
        return "AUDIO";
    default:
        return "UNKWN";
    }
}

struct ExampleLogData
{
    ExampleLogLevel level;
    ExampleLogRegion region;
};


void ExamplePrintMessage(const ExampleLogData& data, size_t sequenceNumber, const char* message)
{
    printf("{%lu} [%s] (%s): %s\n", 
        sequenceNumber, 
        rtlog::test::to_string(data.level), 
        rtlog::test::to_string(data.region), 
        message);
}

}
}

using namespace rtlog::test;


TEST_CASE("Dummy test")
{
    CHECK(true);
}

TEST_CASE("Test rtlog basic construction")
{
    rtlog::Logger<ExampleLogData, MAX_NUM_LOG_MESSAGES, MAX_LOG_MESSAGE_LENGTH, gSequenceNumber> logger;
    logger.Log({ExampleLogLevel::Debug, ExampleLogRegion::Engine}, "Hello, world!");
    logger.Log({ExampleLogLevel::Info, ExampleLogRegion::Game}, "Hello, world!");
    logger.Log({ExampleLogLevel::Warning, ExampleLogRegion::Network}, "Hello, world!");
    logger.Log({ExampleLogLevel::Critical, ExampleLogRegion::Audio}, "Hello, world!");

    logger.ProcessLog(ExamplePrintMessage);
    logger.ProcessLog(ExamplePrintMessage);
    logger.ProcessLog(ExamplePrintMessage);
    logger.ProcessLog(ExamplePrintMessage);
}

TEST_CASE("va_args works as intended")
{
    rtlog::Logger<ExampleLogData, MAX_NUM_LOG_MESSAGES, MAX_LOG_MESSAGE_LENGTH, gSequenceNumber> logger;

    logger.Log({ExampleLogLevel::Debug, ExampleLogRegion::Engine}, "Hello, %lu!", 123l); 
    logger.Log({ExampleLogLevel::Info, ExampleLogRegion::Game}, "Hello, %f!", 123.0f);
    logger.Log({ExampleLogLevel::Warning, ExampleLogRegion::Network}, "Hello, %lf!", 123.0);
    logger.Log({ExampleLogLevel::Critical, ExampleLogRegion::Audio}, "Hello, %p!", (void*)123);
    logger.Log({ExampleLogLevel::Debug, ExampleLogRegion::Engine}, "Hello, %d!", 123);
    logger.Log({ExampleLogLevel::Critical, ExampleLogRegion::Audio}, "Hello, %s!", "world");

    logger.ProcessLog(ExamplePrintMessage);
    logger.ProcessLog(ExamplePrintMessage);
    logger.ProcessLog(ExamplePrintMessage);
    logger.ProcessLog(ExamplePrintMessage);
    logger.ProcessLog(ExamplePrintMessage);
}
