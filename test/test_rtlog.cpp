#include <rtlog/rtlog.h>

#include <gtest/gtest.h>

namespace rtlog::test {

static std::atomic<std::size_t> gSequenceNumber{0};

constexpr auto MAX_LOG_MESSAGE_LENGTH = 256;
constexpr auto MAX_NUM_LOG_MESSAGES = 128;

enum class ExampleLogLevel { Debug, Info, Warning, Critical };

static const char *to_string(ExampleLogLevel level) {
  switch (level) {
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

enum class ExampleLogRegion { Engine, Game, Network, Audio };

static const char *to_string(ExampleLogRegion region) {
  switch (region) {
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

struct ExampleLogData {
  ExampleLogLevel level;
  ExampleLogRegion region;
};

static auto PrintMessage =
    [](const ExampleLogData &data, size_t sequenceNumber, const char *fstring,
       ...) __attribute__((format(printf, 4, 5))) {
  std::array<char, MAX_LOG_MESSAGE_LENGTH> buffer;
  // print fstring and the varargs into a std::string
  va_list args;
  va_start(args, fstring);
  vsnprintf(buffer.data(), buffer.size(), fstring, args);
  va_end(args);

  printf("{%lu} [%s] (%s): %s\n", sequenceNumber,
         rtlog::test::to_string(data.level),
         rtlog::test::to_string(data.region), buffer.data());
};

} // namespace rtlog::test

using namespace rtlog::test;

TEST(RtlogTest, BasicConstruction) {
  rtlog::Logger<ExampleLogData, MAX_NUM_LOG_MESSAGES, MAX_LOG_MESSAGE_LENGTH,
                gSequenceNumber>
      logger;
  logger.Log({ExampleLogLevel::Debug, ExampleLogRegion::Engine},
             "Hello, world!");
  logger.Log({ExampleLogLevel::Info, ExampleLogRegion::Game}, "Hello, world!");
  logger.Log({ExampleLogLevel::Warning, ExampleLogRegion::Network},
             "Hello, world!");
  logger.Log({ExampleLogLevel::Critical, ExampleLogRegion::Audio},
             "Hello, world!");

  EXPECT_EQ(logger.PrintAndClearLogQueue(PrintMessage), 4);
}

TEST(RtlogTest, MPSCWorksAsIntended) {
  rtlog::Logger<ExampleLogData, MAX_NUM_LOG_MESSAGES, MAX_LOG_MESSAGE_LENGTH,
                gSequenceNumber,
                rtlog::QueueConcurrency::Multi_Producer_Single_Consumer>
      logger;
  logger.Log({ExampleLogLevel::Debug, ExampleLogRegion::Engine},
             "Hello, world!");
  logger.Log({ExampleLogLevel::Info, ExampleLogRegion::Game}, "Hello, world!");
  logger.Log({ExampleLogLevel::Warning, ExampleLogRegion::Network},
             "Hello, world!");
  logger.Log({ExampleLogLevel::Critical, ExampleLogRegion::Audio},
             "Hello, world!");

  EXPECT_EQ(logger.PrintAndClearLogQueue(PrintMessage), 4);
}

TEST(RtlogTest, VaArgsWorksAsIntended) {
  rtlog::Logger<ExampleLogData, MAX_NUM_LOG_MESSAGES, MAX_LOG_MESSAGE_LENGTH,
                gSequenceNumber>
      logger;

  logger.Log({ExampleLogLevel::Debug, ExampleLogRegion::Engine}, "Hello, %lu!",
             123ul);
  logger.Log({ExampleLogLevel::Info, ExampleLogRegion::Game}, "Hello, %f!",
             123.0);
  logger.Log({ExampleLogLevel::Warning, ExampleLogRegion::Network},
             "Hello, %lf!", 123.0);
  logger.Log({ExampleLogLevel::Critical, ExampleLogRegion::Audio}, "Hello, %p!",
             (void *)123);
  logger.Log({ExampleLogLevel::Debug, ExampleLogRegion::Engine}, "Hello, %d!",
             123);
  logger.Log({ExampleLogLevel::Critical, ExampleLogRegion::Audio}, "Hello, %s!",
             "world");

  EXPECT_EQ(logger.PrintAndClearLogQueue(PrintMessage), 6);
}

void vaArgsTest(rtlog::Logger<ExampleLogData, MAX_NUM_LOG_MESSAGES,
                              MAX_LOG_MESSAGE_LENGTH, gSequenceNumber> &logger,
                ExampleLogData &&data, const char *format, ...) {
  va_list args;
  va_start(args, format);
  logger.Logv(std::move(data), format, args);
  va_end(args);
}

TEST(RtlogTest, LogvVersionWorks) {
  rtlog::Logger<ExampleLogData, MAX_NUM_LOG_MESSAGES, MAX_LOG_MESSAGE_LENGTH,
                gSequenceNumber>
      logger;

  vaArgsTest(logger, {ExampleLogLevel::Debug, ExampleLogRegion::Engine},
             "Hello, %lu!", 123ul);
  vaArgsTest(logger, {ExampleLogLevel::Info, ExampleLogRegion::Game},
             "Hello, %f!", 123.0);
  vaArgsTest(logger, {ExampleLogLevel::Warning, ExampleLogRegion::Network},
             "Hello, %lf!", 123.0);
  vaArgsTest(logger, {ExampleLogLevel::Critical, ExampleLogRegion::Audio},
             "Hello, %p!", (void *)123);
  vaArgsTest(logger, {ExampleLogLevel::Debug, ExampleLogRegion::Engine},
             "Hello, %d!", 123);
  vaArgsTest(logger, {ExampleLogLevel::Critical, ExampleLogRegion::Audio},
             "Hello, %s!", "world");

  EXPECT_EQ(logger.PrintAndClearLogQueue(PrintMessage), 6);
}

TEST(RtlogTest, LoggerThreadDoesItsJob) {
  rtlog::Logger<ExampleLogData, MAX_NUM_LOG_MESSAGES, MAX_LOG_MESSAGE_LENGTH,
                gSequenceNumber>
      logger;

  rtlog::LogProcessingThread thread(logger, PrintMessage,
                                    std::chrono::milliseconds(10));

  logger.Log({ExampleLogLevel::Debug, ExampleLogRegion::Engine}, "Hello, %lu!",
             123ul);
  logger.Log({ExampleLogLevel::Info, ExampleLogRegion::Game}, "Hello, %f!",
             123.0);
  logger.Log({ExampleLogLevel::Warning, ExampleLogRegion::Network},
             "Hello, %lf!", 123.0);
  logger.Log({ExampleLogLevel::Critical, ExampleLogRegion::Audio}, "Hello, %p!",
             (void *)123);
  logger.Log({ExampleLogLevel::Debug, ExampleLogRegion::Engine}, "Hello, %d!",
             123);
  logger.Log({ExampleLogLevel::Critical, ExampleLogRegion::Audio}, "Hello, %s!",
             "world");

  thread.Stop();
}

TEST(RtlogTest, ErrorsReturnedFromLog) {
  rtlog::Logger<ExampleLogData, MAX_NUM_LOG_MESSAGES, MAX_LOG_MESSAGE_LENGTH,
                gSequenceNumber>
      logger;

  EXPECT_EQ(logger.Log({ExampleLogLevel::Debug, ExampleLogRegion::Engine},
                       "Hello, %lu!", 123ul),
            rtlog::Status::Success);

  const auto maxMessageLength = 10;
  rtlog::Logger<ExampleLogData, MAX_NUM_LOG_MESSAGES, maxMessageLength,
                gSequenceNumber>
      truncatedLogger;
  EXPECT_EQ(
      truncatedLogger.Log({ExampleLogLevel::Debug, ExampleLogRegion::Engine},
                          "Hello, %lu! xxxxxxxxxxx", 123ul),
      rtlog::Status::Error_MessageTruncated);

  // Inspect truncated message
  auto InspectLogMessage = [](const ExampleLogData &data, size_t sequenceNumber,
                              const char *fstring, ...) {
    (void)sequenceNumber;
    EXPECT_EQ(data.level, ExampleLogLevel::Debug);
    EXPECT_EQ(data.region, ExampleLogRegion::Engine);

    std::array<char, MAX_LOG_MESSAGE_LENGTH> buffer{};
    va_list args;
    va_start(args, fstring);
    vsnprintf(buffer.data(), buffer.size(), fstring, args);
    va_end(args);

    EXPECT_STREQ(buffer.data(), "Hello, 12");
    EXPECT_EQ(strlen(buffer.data()), maxMessageLength - 1);
  };
  EXPECT_EQ(truncatedLogger.PrintAndClearLogQueue(InspectLogMessage), 1);
}

#ifdef RTLOG_USE_FMTLIB

TEST(LoggerTest, FormatLibVersionWorksAsIntended) {
  rtlog::Logger<ExampleLogData, MAX_NUM_LOG_MESSAGES, MAX_LOG_MESSAGE_LENGTH,
                gSequenceNumber>
      logger;

  logger.LogFmt({ExampleLogLevel::Debug, ExampleLogRegion::Engine},
                FMT_STRING("Hello, {}!"), 123l);
  logger.LogFmt({ExampleLogLevel::Info, ExampleLogRegion::Game},
                FMT_STRING("Hello, {}!"), 123.0f);
  logger.LogFmt({ExampleLogLevel::Warning, ExampleLogRegion::Network},
                FMT_STRING("Hello, {}!"), 123.0);
  logger.LogFmt({ExampleLogLevel::Critical, ExampleLogRegion::Audio},
                FMT_STRING("Hello, {}!"), (void *)123);
  logger.LogFmt({ExampleLogLevel::Debug, ExampleLogRegion::Engine},
                FMT_STRING("Hello, {}!"), 123);
  logger.LogFmt({ExampleLogLevel::Critical, ExampleLogRegion::Audio},
                FMT_STRING("Hello, {}!"), "world");

  EXPECT_EQ(logger.PrintAndClearLogQueue(PrintMessage), 6);
}

TEST(LoggerTest, LogFmtReturnsSuccessOnNormalEnqueue) {
  rtlog::Logger<ExampleLogData, MAX_NUM_LOG_MESSAGES, MAX_LOG_MESSAGE_LENGTH,
                gSequenceNumber>
      logger;
  EXPECT_EQ(logger.LogFmt({ExampleLogLevel::Debug, ExampleLogRegion::Engine},
                          FMT_STRING("Hello, {}!"), 123l),
            rtlog::Status::Success);
}

TEST(LoggerTest, LogFmtHandlesLongMessageTruncation) {
  const auto maxMessageLength = 10;
  rtlog::Logger<ExampleLogData, MAX_NUM_LOG_MESSAGES, maxMessageLength,
                gSequenceNumber>
      logger;

  EXPECT_EQ(logger.LogFmt({ExampleLogLevel::Debug, ExampleLogRegion::Engine},
                          FMT_STRING("Hello, {}! xxxxxxxxxxx"), 123l),
            rtlog::Status::Error_MessageTruncated);

  auto InspectLogMessage = [](const ExampleLogData &data, size_t sequenceNumber,
                              const char *fstring, ...) {
    (void)sequenceNumber;

    EXPECT_EQ(data.level, ExampleLogLevel::Debug);
    EXPECT_EQ(data.region, ExampleLogRegion::Engine);

    std::array<char, MAX_LOG_MESSAGE_LENGTH> buffer{};
    va_list args;
    va_start(args, fstring);
    vsnprintf(buffer.data(), buffer.size(), fstring, args);
    va_end(args);

    EXPECT_STREQ(buffer.data(), "Hello, 12");
    EXPECT_EQ(strlen(buffer.data()), maxMessageLength - 1);
  };

  EXPECT_EQ(logger.PrintAndClearLogQueue(InspectLogMessage), 1);
}

TEST(LoggerTest, LogFmtHandlesQueueFullError) {
  const auto maxNumMessages = 10;
  rtlog::Logger<ExampleLogData, maxNumMessages, MAX_LOG_MESSAGE_LENGTH,
                gSequenceNumber>
      logger;

  auto status = rtlog::Status::Success;

  while (status == rtlog::Status::Success) {
    status = logger.LogFmt({ExampleLogLevel::Debug, ExampleLogRegion::Engine},
                           FMT_STRING("Hello, {}!"), "world");
  }

  EXPECT_EQ(status, rtlog::Status::Error_QueueFull);
}

#endif // RTLOG_USE_FMTLIB
