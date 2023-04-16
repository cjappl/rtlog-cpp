
#include <rtlog/rtlog.h>

namespace everlog
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

class PrintMessageFunctor
{
public:
    void operator()(const ExampleLogData& data, size_t sequenceNumber, const char* fstring, ...) const __attribute__ ((format (printf, 4, 5)))
    {
        std::array<char, MAX_LOG_MESSAGE_LENGTH> buffer;
        // print fstring and the varargs into a std::string
        va_list args;
        va_start(args, fstring);
        vsnprintf(buffer.data(), buffer.size(), fstring, args);
        va_end(args);

        printf("{%lu} [%s] (%s): %s\n", 
            sequenceNumber, 
            to_string(data.level), 
            to_string(data.region), 
            buffer.data());
    }
};

static const PrintMessageFunctor ExamplePrintMessage;

}

using namespace everlog;

std::atomic<bool> gRunning{ true };

int main(int argc, char** argv)
{
    rtlog::Logger<ExampleLogData, MAX_NUM_LOG_MESSAGES, MAX_LOG_MESSAGE_LENGTH, gSequenceNumber> realtimeLogger;

    rtlog::ScopedLogThread thread(realtimeLogger, ExamplePrintMessage, std::chrono::milliseconds(10));

    std::thread realtimeThread { [&realtimeLogger]() {
        while (gRunning)
        {
            for (int i = 0; i < 100; i++)
            {
                realtimeLogger.Log({ ExampleLogLevel::Debug, ExampleLogRegion::Audio }, "Hello %d from rt-thread", i);
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            }
        }
    } };

    std::thread nonRealtimeThread { [&realtimeLogger]() {
        while (gRunning)
        {
            for (int i = 0; i < 100; i++)
            {
                ExamplePrintMessage({ ExampleLogLevel::Debug, ExampleLogRegion::Audio }, ++gSequenceNumber, "Hello %d from non-rt-thread", i);
                std::this_thread::sleep_for(std::chrono::milliseconds(750));
            }
        }
    } };

    realtimeThread.join();
    nonRealtimeThread.join();
    return 0;
}

// if we receive a signal, stop the threads
void signalHandler(int signum)
{
    gRunning = false;
}
