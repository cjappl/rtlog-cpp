


#pragma once

#include <array>
#include <cstdarg>
#include <cstdio>

#include <readerwriterqueue.h>

namespace rtlog 
{

// TODO Rip this stuff out and templatize it
constexpr auto MAX_LOG_MESSAGE_LENGTH = 256;

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
    std::array<char, MAX_LOG_MESSAGE_LENGTH> message;
};

class Logger
{
public:
    void Log(ExampleLogLevel level, ExampleLogRegion region, const char* format, ...)
    {
        ExampleLogData data;
        data.level = level;
        data.region = region;

        va_list args;
        va_start(args, format);
        vsnprintf(data.message.data(), data.message.size(), format, args);
        va_end(args);

        bool result = mQueue.try_enqueue(data);
        assert(result);
    }

    void ProcessLog() 
    {
        ExampleLogData value;
        while (mQueue.try_dequeue(value)) 
        {
            ExamplePrintMessage(value);
        }
    }

    void ExamplePrintMessage(const ExampleLogData& data)
    {
        printf("[%s] (%s): %s\n", 
            rtlog::to_string(data.level), 
            rtlog::to_string(data.region), 
            data.message.data());
    }
private:
    moodycamel::ReaderWriterQueue<ExampleLogData> mQueue;
};

}
