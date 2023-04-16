


#pragma once

#include <array>
#include <cstdarg>
#include <cstdio>

#include <readerwriterqueue.h>

namespace rtlog 
{

template <typename LogData, 
          size_t MaxNumMessages, 
          size_t MaxMessageLength,
          std::atomic<std::size_t>& SequenceNumber
          >

class Logger
{
public:
    void Log(LogData inputData, const char* format, ...) __attribute__ ((format (printf, 3, 4)))
    {
        InternalLogData dataToQueue;
        dataToQueue.mLogData = inputData;
        dataToQueue.mSequenceNumber = SequenceNumber++;

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
            printLogFn(value.mLogData, value.mSequenceNumber, value.mMessage.data());
        }
    }

private:
    struct InternalLogData
    {
        LogData mLogData;
        size_t mSequenceNumber;
        std::array<char, MaxMessageLength> mMessage;
    };

    moodycamel::ReaderWriterQueue<InternalLogData> mQueue{ MaxNumMessages };
};

} // namespace rtlog
