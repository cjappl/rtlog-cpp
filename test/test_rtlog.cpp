#include <doctest/doctest.h>
#include <rtlog/rtlog.h>

namespace rtlog::test
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



static auto PrintMessage = [](const ExampleLogData& data, size_t sequenceNumber, const char* fstring, ...) __attribute__ ((format (printf, 4, 5)))
{
    std::array<char, MAX_LOG_MESSAGE_LENGTH> buffer;
    // print fstring and the varargs into a std::string
    va_list args;
    va_start(args, fstring);
    vsnprintf(buffer.data(), buffer.size(), fstring, args);
    va_end(args);

    printf("{%lu} [%s] (%s): %s\n", 
        sequenceNumber, 
        rtlog::test::to_string(data.level), 
        rtlog::test::to_string(data.region), 
        buffer.data());
};

} // namespace rtlog::test

using namespace rtlog::test;

TEST_CASE("Test rtlog basic construction")
{
    rtlog::Logger<ExampleLogData, MAX_NUM_LOG_MESSAGES, MAX_LOG_MESSAGE_LENGTH, gSequenceNumber> logger;
    logger.Log({ExampleLogLevel::Debug, ExampleLogRegion::Engine}, "Hello, world!");
    logger.Log({ExampleLogLevel::Info, ExampleLogRegion::Game}, "Hello, world!");
    logger.Log({ExampleLogLevel::Warning, ExampleLogRegion::Network}, "Hello, world!");
    logger.Log({ExampleLogLevel::Critical, ExampleLogRegion::Audio}, "Hello, world!");

    CHECK(logger.PrintAndClearLogQueue(PrintMessage) == 4);
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

    CHECK(logger.PrintAndClearLogQueue(PrintMessage) == 6);
}

void vaArgsTest(rtlog::Logger<ExampleLogData, MAX_NUM_LOG_MESSAGES, MAX_LOG_MESSAGE_LENGTH, gSequenceNumber>& logger, ExampleLogData&& data, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    logger.Logv(std::move(data), format, args);
    va_end(args);
}

TEST_CASE("Logv version works as well")
{
    rtlog::Logger<ExampleLogData, MAX_NUM_LOG_MESSAGES, MAX_LOG_MESSAGE_LENGTH, gSequenceNumber> logger;

    vaArgsTest(logger, {ExampleLogLevel::Debug, ExampleLogRegion::Engine}, "Hello, %lu!", 123l);
    vaArgsTest(logger, {ExampleLogLevel::Info, ExampleLogRegion::Game}, "Hello, %f!", 123.0f);
    vaArgsTest(logger, {ExampleLogLevel::Warning, ExampleLogRegion::Network}, "Hello, %lf!", 123.0);
    vaArgsTest(logger, {ExampleLogLevel::Critical, ExampleLogRegion::Audio}, "Hello, %p!", (void*)123);
    vaArgsTest(logger, {ExampleLogLevel::Debug, ExampleLogRegion::Engine}, "Hello, %d!", 123);
    vaArgsTest(logger, {ExampleLogLevel::Critical, ExampleLogRegion::Audio}, "Hello, %s!", "world");

    CHECK(logger.PrintAndClearLogQueue(PrintMessage) == 6);
}

TEST_CASE("LoggerThread does it's job")
{
    rtlog::Logger<ExampleLogData, MAX_NUM_LOG_MESSAGES, MAX_LOG_MESSAGE_LENGTH, gSequenceNumber> logger;

    rtlog::LogProcessingThread thread(logger, PrintMessage, std::chrono::milliseconds(10));

    logger.Log({ExampleLogLevel::Debug, ExampleLogRegion::Engine}, "Hello, %lu!", 123l); 
    logger.Log({ExampleLogLevel::Info, ExampleLogRegion::Game}, "Hello, %f!", 123.0f);
    logger.Log({ExampleLogLevel::Warning, ExampleLogRegion::Network}, "Hello, %lf!", 123.0);
    logger.Log({ExampleLogLevel::Critical, ExampleLogRegion::Audio}, "Hello, %p!", (void*)123);
    logger.Log({ExampleLogLevel::Debug, ExampleLogRegion::Engine}, "Hello, %d!", 123);
    logger.Log({ExampleLogLevel::Critical, ExampleLogRegion::Audio}, "Hello, %s!", "world");

    thread.Stop();
}

TEST_CASE("Errors are returned from Log")
{
    SUBCASE("Success on normal enqueue")
    {
        rtlog::Logger<ExampleLogData, MAX_NUM_LOG_MESSAGES, MAX_LOG_MESSAGE_LENGTH, gSequenceNumber> logger;

        CHECK(logger.Log({ExampleLogLevel::Debug, ExampleLogRegion::Engine}, "Hello, %lu!", 123l) == rtlog::Status::Success); 
    }

    SUBCASE("On a very long message, get truncation")
    {
        const auto maxMessageLength = 10;
        rtlog::Logger<ExampleLogData, MAX_NUM_LOG_MESSAGES, maxMessageLength, gSequenceNumber> logger;
        CHECK(logger.Log({ExampleLogLevel::Debug, ExampleLogRegion::Engine}, "Hello, %lu! xxxxxxxxxxx", 123l) == rtlog::Status::Error_MessageTruncated);

        SUBCASE("And you get a trunctated message out if you try to process")
        {
            auto InspectLogMessage = [](const ExampleLogData& data, size_t sequenceNumber, const char* fstring, ...)
            {
                (void)sequenceNumber;

                CHECK(data.level == ExampleLogLevel::Debug);
                CHECK(data.region == ExampleLogRegion::Engine);

                std::array<char, MAX_LOG_MESSAGE_LENGTH> buffer{};
                va_list args;
                va_start(args, fstring);
                vsnprintf(buffer.data(), buffer.size(), fstring, args);
                va_end(args);

                CHECK(buffer.data() == "Hello, 12");
                CHECK(strlen(buffer.data()) == maxMessageLength - 1);
            };

            CHECK(logger.PrintAndClearLogQueue(InspectLogMessage) == 1);
        }

    }

    SUBCASE("Enqueue more than capacity and get an error")
    {
        const auto maxNumMessages = 10;
        rtlog::Logger<ExampleLogData, maxNumMessages, MAX_LOG_MESSAGE_LENGTH, gSequenceNumber> logger;

        auto status = rtlog::Status::Success;
        auto numMessagesEnqueued = 0;
        while (status == rtlog::Status::Success)
        {
            status = logger.Log({ExampleLogLevel::Debug, ExampleLogRegion::Engine}, "Hello, %s!", "world");
            if (status == rtlog::Status::Success)
            {
                ++numMessagesEnqueued;
            }
        }

        // NOTE: This isn't a hard limit, it's up to moodycamel to decide what the exact block size is
        // on my machine setting a maxNumMessages of 10 results in a block size of 16!
        CHECK(status == rtlog::Status::Error_QueueFull);
    }
}

#ifdef RTLOG_USE_FMTLIB

TEST_CASE("Formatlib version works as intended")
{
    rtlog::Logger<ExampleLogData, MAX_NUM_LOG_MESSAGES, MAX_LOG_MESSAGE_LENGTH, gSequenceNumber> logger;

    logger.LogFmt({ExampleLogLevel::Debug, ExampleLogRegion::Engine}, FMT_STRING("Hello, {}!"), 123l); 
    logger.LogFmt({ExampleLogLevel::Info, ExampleLogRegion::Game}, FMT_STRING("Hello, {}!"), 123.0f);
    logger.LogFmt({ExampleLogLevel::Warning, ExampleLogRegion::Network}, FMT_STRING("Hello, {}!"), 123.0);
    logger.LogFmt({ExampleLogLevel::Critical, ExampleLogRegion::Audio}, FMT_STRING("Hello, {}!"), (void*)123);
    logger.LogFmt({ExampleLogLevel::Debug, ExampleLogRegion::Engine}, FMT_STRING("Hello, {}!"), 123);
    logger.LogFmt({ExampleLogLevel::Critical, ExampleLogRegion::Audio}, FMT_STRING("Hello, {}!"), "world");

    CHECK(logger.PrintAndClearLogQueue(PrintMessage) == 6);
}

TEST_CASE("Errors are returned from LogFmt")
{
    SUBCASE("Success on normal enqueue")
    {
        rtlog::Logger<ExampleLogData, MAX_NUM_LOG_MESSAGES, MAX_LOG_MESSAGE_LENGTH, gSequenceNumber> logger;

        CHECK(logger.LogFmt({ExampleLogLevel::Debug, ExampleLogRegion::Engine}, FMT_STRING("Hello, {}!"), 123l) == rtlog::Status::Success); 
    }

    SUBCASE("On a very long message, get truncation")
    {
        const auto maxMessageLength = 10;
        rtlog::Logger<ExampleLogData, MAX_NUM_LOG_MESSAGES, maxMessageLength, gSequenceNumber> logger;
        CHECK(logger.LogFmt({ExampleLogLevel::Debug, ExampleLogRegion::Engine}, FMT_STRING("Hello, {}! xxxxxxxxxxx"), 123l) == rtlog::Status::Error_MessageTruncated);

        SUBCASE("And you get a truncated message out if you try to process")
        {
            auto InspectLogMessage = [](const ExampleLogData& data, size_t sequenceNumber, const char* fstring, ...)
            {
                (void)sequenceNumber;

                CHECK(data.level == ExampleLogLevel::Debug);
                CHECK(data.region == ExampleLogRegion::Engine);

                std::array<char, MAX_LOG_MESSAGE_LENGTH> buffer{};
                va_list args;
                va_start(args, fstring);
                vsnprintf(buffer.data(), buffer.size(), fstring, args);
                va_end(args);

                CHECK(buffer.data() == "Hello, 12");
                CHECK(strlen(buffer.data()) == maxMessageLength - 1);
            };

            CHECK(logger.PrintAndClearLogQueue(InspectLogMessage) == 1);
        }

    }

    SUBCASE("Enqueue more than capacity and get an error")
    {
        const auto maxNumMessages = 10;
        rtlog::Logger<ExampleLogData, maxNumMessages, MAX_LOG_MESSAGE_LENGTH, gSequenceNumber> logger;

        auto status = rtlog::Status::Success;
        auto numMessagesEnqueued = 0;
        while (status == rtlog::Status::Success)
        {
            status = logger.LogFmt({ExampleLogLevel::Debug, ExampleLogRegion::Engine}, FMT_STRING("Hello, {}!"), "world");
            if (status == rtlog::Status::Success)
            {
                ++numMessagesEnqueued;
            }
        }

        // NOTE: This isn't a hard limit, it's up to moodycamel to decide what the exact block size is
        // on my machine setting a maxNumMessages of 10 results in a block size of 16!
        CHECK(status == rtlog::Status::Error_QueueFull);
    }
}

#endif // RTLOG_USE_FMTLIB
