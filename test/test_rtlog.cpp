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

static auto PrintMessage = [](const ExampleLogData &data, size_t sequenceNumber,
                              const char *fstring, ...) {
  std::array<char, MAX_LOG_MESSAGE_LENGTH> buffer;
  // print fstring and the varargs into a std::string
  va_list args;
  va_start(args, fstring);
  vsnprintf(buffer.data(), buffer.size(), fstring, args);
  va_end(args);

  printf("{%zu} [%s] (%s): %s\n", sequenceNumber,
         rtlog::test::to_string(data.level),
         rtlog::test::to_string(data.region), buffer.data());
};

} // namespace rtlog::test

using namespace rtlog::test;

using SingleWriterRtLoggerType =
    rtlog::Logger<ExampleLogData, MAX_NUM_LOG_MESSAGES, MAX_LOG_MESSAGE_LENGTH,
                  gSequenceNumber, rtlog::SingleRealtimeWriterQueueType>;
using MultiWriterRtLoggerType =
    rtlog::Logger<ExampleLogData, MAX_NUM_LOG_MESSAGES, MAX_LOG_MESSAGE_LENGTH,
                  gSequenceNumber, rtlog::MultiRealtimeWriterQueueType>;

template <typename LoggerType> class RtLogTest : public ::testing::Test {
protected:
  LoggerType logger;
};

typedef ::testing::Types<SingleWriterRtLoggerType, MultiWriterRtLoggerType>
    LoggerTypes;
TYPED_TEST_SUITE(RtLogTest, LoggerTypes);

using TruncatedSingleWriterRtLoggerType =
    rtlog::Logger<ExampleLogData, MAX_NUM_LOG_MESSAGES, 10, gSequenceNumber,
                  rtlog::SingleRealtimeWriterQueueType>;
using TruncatedMultiWriterRtLoggerType =
    rtlog::Logger<ExampleLogData, MAX_NUM_LOG_MESSAGES, 10, gSequenceNumber,
                  rtlog::MultiRealtimeWriterQueueType>;

template <typename LoggerType>
class TruncatedRtLogTest : public ::testing::Test {
protected:
  LoggerType logger;
  inline static const size_t maxMessageLength = 10;
};

typedef ::testing::Types<TruncatedSingleWriterRtLoggerType,
                         TruncatedMultiWriterRtLoggerType>
    TruncatedLoggerTypes;
TYPED_TEST_SUITE(TruncatedRtLogTest, TruncatedLoggerTypes);

#ifdef RTLOG_USE_STB

TYPED_TEST(RtLogTest, BasicConstruction) {
  auto &logger = this->logger;
  logger.Log({ExampleLogLevel::Debug, ExampleLogRegion::Engine},
             "Hello, world!");
  logger.Log({ExampleLogLevel::Info, ExampleLogRegion::Game}, "Hello, world!");
  logger.Log({ExampleLogLevel::Warning, ExampleLogRegion::Network},
             "Hello, world!");
  logger.Log({ExampleLogLevel::Critical, ExampleLogRegion::Audio},
             "Hello, world!");

  EXPECT_EQ(logger.PrintAndClearLogQueue(PrintMessage), 4);
}

TYPED_TEST(RtLogTest, VaArgsWorksAsIntended) {
  auto &logger = this->logger;
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

template <typename LoggerType>
void vaArgsTest(LoggerType &&logger, ExampleLogData &&data, const char *format,
                ...) {
  va_list args;
  va_start(args, format);
  logger.Logv(std::move(data), format, args);
  va_end(args);
}

TYPED_TEST(RtLogTest, LogvVersionWorks) {
  auto &logger = this->logger;
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

TYPED_TEST(RtLogTest, LoggerThreadDoesItsJob) {
  auto &logger = this->logger;
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

TYPED_TEST(TruncatedRtLogTest, ErrorsReturnedFromLog) {
  auto &logger = this->logger;
  EXPECT_EQ(logger.Log({ExampleLogLevel::Debug, ExampleLogRegion::Engine},
                       "Hello, %lu", 12ul),
            rtlog::Status::Success);
  EXPECT_EQ(logger.Log({ExampleLogLevel::Debug, ExampleLogRegion::Engine},
                       "Hello, %luxxxxxxxxxxxxxx", 123ul),
            rtlog::Status::Error_MessageTruncated);

  // Inspect truncated message
  auto InspectLogMessage = [=](const ExampleLogData &data,
                               size_t sequenceNumber, const char *fstring,
                               ...) {
    (void)sequenceNumber;
    EXPECT_EQ(data.level, ExampleLogLevel::Debug);
    EXPECT_EQ(data.region, ExampleLogRegion::Engine);

    std::array<char, MAX_LOG_MESSAGE_LENGTH> buffer{};
    va_list args;
    va_start(args, fstring);
    vsnprintf(buffer.data(), buffer.size(), fstring, args);
    va_end(args);

    EXPECT_STREQ(buffer.data(), "Hello, 12");
    EXPECT_EQ(strlen(buffer.data()), this->maxMessageLength - 1);
  };
  EXPECT_EQ(logger.PrintAndClearLogQueue(InspectLogMessage), 2);
}
#endif // RTLOG_USE_STB

#ifdef RTLOG_USE_FMTLIB

TYPED_TEST(RtLogTest, FormatLibVersionWorksAsIntended) {
  auto &logger = this->logger;
  logger.Log({ExampleLogLevel::Debug, ExampleLogRegion::Engine},
             FMT_STRING("Hello, {}!"), 123l);
  logger.Log({ExampleLogLevel::Info, ExampleLogRegion::Game},
             FMT_STRING("Hello, {}!"), 123.0f);
  logger.Log({ExampleLogLevel::Warning, ExampleLogRegion::Network},
             FMT_STRING("Hello, {}!"), 123.0);
  logger.Log({ExampleLogLevel::Critical, ExampleLogRegion::Audio},
             FMT_STRING("Hello, {}!"), (void *)123);
  logger.Log({ExampleLogLevel::Debug, ExampleLogRegion::Engine},
             FMT_STRING("Hello, {}!"), 123);
  logger.Log({ExampleLogLevel::Critical, ExampleLogRegion::Audio},
             FMT_STRING("Hello, {}!"), "world");

  EXPECT_EQ(logger.PrintAndClearLogQueue(PrintMessage), 6);
}

TYPED_TEST(RtLogTest, LogReturnsSuccessOnNormalEnqueue) {
  auto &logger = this->logger;
  EXPECT_EQ(logger.Log({ExampleLogLevel::Debug, ExampleLogRegion::Engine},
                       FMT_STRING("Hello, {}!"), 123l),
            rtlog::Status::Success);
}

TYPED_TEST(TruncatedRtLogTest, LogHandlesLongMessageTruncation) {
  auto &logger = this->logger;
  EXPECT_EQ(logger.Log({ExampleLogLevel::Debug, ExampleLogRegion::Engine},
                       FMT_STRING("Hello, {}"), 12ul),
            rtlog::Status::Success);
  EXPECT_EQ(logger.Log({ExampleLogLevel::Debug, ExampleLogRegion::Engine},
                       FMT_STRING("Hello, {}xxxxxxxxxxx"), 123ul),
            rtlog::Status::Error_MessageTruncated);

  auto InspectLogMessage = [=](const ExampleLogData &data,
                               size_t sequenceNumber, const char *fstring,
                               ...) {
    (void)sequenceNumber;

    EXPECT_EQ(data.level, ExampleLogLevel::Debug);
    EXPECT_EQ(data.region, ExampleLogRegion::Engine);

    std::array<char, MAX_LOG_MESSAGE_LENGTH> buffer{};
    va_list args;
    va_start(args, fstring);
    vsnprintf(buffer.data(), buffer.size(), fstring, args);
    va_end(args);

    EXPECT_STREQ(buffer.data(), "Hello, 12");
    EXPECT_EQ(strlen(buffer.data()), this->maxMessageLength - 1);
  };

  EXPECT_EQ(logger.PrintAndClearLogQueue(InspectLogMessage), 2);
}

TEST(LoggerTest, SingleWriterLogHandlesQueueFullError) {
  const auto maxNumMessages = 16;
  rtlog::Logger<ExampleLogData, maxNumMessages, MAX_LOG_MESSAGE_LENGTH,
                gSequenceNumber, rtlog::SingleRealtimeWriterQueueType>
      logger;

  auto status = rtlog::Status::Success;

  while (status == rtlog::Status::Success) {
    status = logger.Log({ExampleLogLevel::Debug, ExampleLogRegion::Engine},
                        FMT_STRING("Hello, {}!"), "world");
  }

  EXPECT_EQ(status, rtlog::Status::Error_QueueFull);
}

TEST(LoggerTest, MultipleWriterLogHandlesNeverReturnsFull) {
  const auto maxNumMessages = 16;
  rtlog::Logger<ExampleLogData, maxNumMessages, MAX_LOG_MESSAGE_LENGTH,
                gSequenceNumber, rtlog::MultiRealtimeWriterQueueType>
      logger;

  auto status = rtlog::Status::Success;

  int messageCount = 0;

  while (status == rtlog::Status::Success &&
         messageCount < maxNumMessages + 10) {
    status = logger.Log({ExampleLogLevel::Debug, ExampleLogRegion::Engine},
                        FMT_STRING("Hello, {} {}!"), "world", messageCount);
    messageCount++;
  }

  // We can never report full on a multi-writer queue, it is not realtime safe
  // We will just happily spin forever in this loop unless we break
  EXPECT_EQ(status, rtlog::Status::Success);
}

#endif // RTLOG_USE_FMTLIB
