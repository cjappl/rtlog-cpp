#include <rtlog/rtlog.h>

#include <fstream>

namespace evr
{
constexpr auto MAX_LOG_MESSAGE_LENGTH = 256;
constexpr auto MAX_NUM_LOG_MESSAGES = 100;

enum class LogLevel
{
    Debug,
    Info,
    Warning,
    Critical
};

const char* to_string(LogLevel level)
{
    switch (level)
    {
    case LogLevel::Debug:
        return "DEBG";
    case LogLevel::Info:
        return "INFO";
    case LogLevel::Warning:
        return "WARN";
    case LogLevel::Critical:
        return "CRIT";
    default:
        return "Unknown";
    }
}

enum class LogRegion
{
    Engine,
    Game,
    Network,
    Audio
};

const char* to_string(LogRegion region)
{
    switch (region)
    {
    case LogRegion::Engine:
        return "ENGIN";
    case LogRegion::Game:
        return "GAME ";
    case LogRegion::Network:
        return "NETWK";
    case LogRegion::Audio:
        return "AUDIO";
    default:
        return "UNKWN";
    }
}

struct LogData
{
    LogLevel level;
    LogRegion region;
};

class PrintMessageFunctor
{
public:
    explicit PrintMessageFunctor(const std::string& filename) : mFile(filename) {
        mFile.open(filename);
        mFile.clear();
    }

    ~PrintMessageFunctor() {
        if (mFile.is_open()) {
            mFile.close();
        }
    }

    PrintMessageFunctor(const PrintMessageFunctor&) = delete;
    PrintMessageFunctor(PrintMessageFunctor&&) = delete;
    PrintMessageFunctor& operator=(const PrintMessageFunctor&) = delete;
    PrintMessageFunctor& operator=(PrintMessageFunctor&&) = delete;

    void operator()(const LogData& data, size_t sequenceNumber, const char* fstring, ...) __attribute__ ((format (printf, 4, 5)))
    {
        std::array<char, MAX_LOG_MESSAGE_LENGTH> buffer;

        va_list args;
        va_start(args, fstring);
        vsnprintf(buffer.data(), buffer.size(), fstring, args);
        va_end(args);

        printf("{%lu} [%s] (%s): %s\n", 
            sequenceNumber, 
            to_string(data.level), 
            to_string(data.region), 
            buffer.data());

        mFile << "{" << sequenceNumber << "} [" << to_string(data.level) << "] (" << to_string(data.region) << "): " << buffer.data() << std::endl;
    }
    std::ofstream mFile;
};

static PrintMessageFunctor PrintMessage("everlog.txt");

template <typename LoggerType>
void RealtimeBusyWait(int milliseconds, LoggerType& logger) 
{
    auto start = std::chrono::high_resolution_clock::now();
    while (true) 
    {
        auto now = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start);
        if (elapsed.count() >= milliseconds) 
        {
            logger.Log({ LogLevel::Debug, LogRegion::Engine }, "Done!!");
            break;
        }
    }
}

}

using namespace evr;

std::atomic<bool> gRunning{ true };

std::atomic<std::size_t> gSequenceNumber{ 0 };
static rtlog::Logger<LogData, MAX_NUM_LOG_MESSAGES, MAX_LOG_MESSAGE_LENGTH, gSequenceNumber> gRealtimeLogger;

#define EVR_LOG_DEBUG(Region, fstring, ...) PrintMessage({ LogLevel::Debug, Region}, ++gSequenceNumber, fstring, ##__VA_ARGS__)
#define EVR_LOG_INFO(Region, fstring, ...) PrintMessage({ LogLevel::Info, Region}, ++gSequenceNumber, fstring, ##__VA_ARGS__)
#define EVR_LOG_WARNING(Region, fstring, ...) PrintMessage({ LogLevel::Warning, Region}, ++gSequenceNumber, fstring, ##__VA_ARGS__)
#define EVR_LOG_CRITICAL(Region, ...) PrintMessage({ LogLevel::Critical, Region}, ++gSequenceNumber, fstring, ##__VA_ARGS__)

#define EVR_RTLOG_DEBUG(Region, fstring, ...) gRealtimeLogger.Log({ LogLevel::Debug, Region}, fstring, ##__VA_ARGS__)
#define EVR_RTLOG_INFO(Region, fstring, ...) gRealtimeLogger.Log({ LogLevel::Info, Region}, fstring, ##__VA_ARGS__)
#define EVR_RTLOG_WARNING(Region, fstring, ...) gRealtimeLogger.Log({ LogLevel::Warning, Region}, fstring, ##__VA_ARGS__)
#define EVR_RTLOG_CRITICAL(Region, fstring, ...) gRealtimeLogger.Log({ LogLevel::Critical, Region}, fstring, ##__VA_ARGS__)



#ifdef RTLOG_USE_FMTLIB

#define EVR_RTLOG_FMT_DEBUG(Region, fstring, ...) gRealtimeLogger.LogFmt({ LogLevel::Debug, Region}, FMT_STRING(fstring), ##__VA_ARGS__)
#define EVR_RTLOG_FMT_INFO(Region, fstring, ...) gRealtimeLogger.LogFmt({ LogLevel::Info, Region}, FMT_STRING(fstring), ##__VA_ARGS__)
#define EVR_RTLOG_FMT_WARNING(Region, fstring, ...) gRealtimeLogger.LogFmt({ LogLevel::Warning, Region}, FMT_STRING(fstring), ##__VA_ARGS__)
#define EVR_RTLOG_FMT_CRITICAL(Region, fstring, ...) gRealtimeLogger.LogFmt({ LogLevel::Critical, Region}, FMT_STRING(fstring), ##__VA_ARGS__)

#else

// define the above macros as no-ops
#define EVR_RTLOG_FMT_DEBUG(Region, fstring, ...) (void)0
#define EVR_RTLOG_FMT_INFO(Region, fstring, ...) (void)0
#define EVR_RTLOG_FMT_WARNING(Region, fstring, ...) (void)0
#define EVR_RTLOG_FMT_CRITICAL(Region, fstring, ...) (void)0

#endif // RTLOG_USE_FMTLIB

int main()
{
    EVR_LOG_INFO(LogRegion::Network, "Hello from main thread!");

    rtlog::LogProcessingThread thread(gRealtimeLogger, PrintMessage, std::chrono::milliseconds(10));

    std::thread realtimeThread { [&]() {
        while (gRunning)
        {
            for (int i = 99; i >= 0; i--)
            {
                EVR_RTLOG_DEBUG(LogRegion::Audio, "Hello %d from rt-thread", i);
                EVR_RTLOG_FMT_WARNING(LogRegion::Audio, "Hello {} from rt-thread - logging with {}", i, "libfmt");
                RealtimeBusyWait(10, gRealtimeLogger);
            }
        }
    } };

    std::thread nonRealtimeThread { []() {
        while (gRunning)
        {
            for (int i = 0; i < 100; i++)
            {
                EVR_LOG_WARNING(LogRegion::Network, "Hello %d from non-rt-thread", i);
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }
    } };

    realtimeThread.join();
    nonRealtimeThread.join();
    return 0;
}

void signalHandler()
{
    gRunning = false;
}
