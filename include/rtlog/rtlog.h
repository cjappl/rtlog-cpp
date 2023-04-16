


#pragma once

#include <array>
#include <cstdarg>
#include <cstdio>

#include <readerwriterqueue.h>

namespace rtlog 
{


template <typename LogData, 
          size_t MaxNumMessages, 
          size_t MaxMessageLength>
class Logger
{
public:
    void Log(LogData inputData, const char* format, ...)
    {
        InternalLogData dataToQueue;
        dataToQueue.mLogData = inputData;

        va_list args;
        va_start(args, format);
        vsnprintf(dataToQueue.mMessage.data(), dataToQueue.mMessage.size(), format, args);
        va_end(args);

        bool result = mQueue.try_enqueue(dataToQueue);
        assert(result); // If you're hitting this often, your queue is too small
    }

    template<typename PrintLogFn>
    void ProcessLog(PrintLogFn printLogFn) 
    {
        InternalLogData value;
        while (mQueue.try_dequeue(value)) 
        {
            printLogFn(value.mLogData, value.mMessage.data());
        }
    }

private:
    struct InternalLogData
    {
        LogData mLogData;
        std::array<char, MaxMessageLength> mMessage;
    };

    moodycamel::ReaderWriterQueue<InternalLogData> mQueue{ MaxNumMessages };
};

}
