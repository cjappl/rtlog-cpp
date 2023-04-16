#pragma once

// TODO: Use format lib and variadic templates
// TODO: Make json logger example?
// TODO: Documentation 
// TODO: Lower camel case?
// TODO: Make atomic sequence number optional

#include <array>
#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <thread>

#include <readerwriterqueue.h>
#include <stb_sprintf.h>

namespace rtlog 
{

enum class Status
{
    Success = 0,

    Error_QueueFull = 1,
    Error_MessageTruncated = 2,
};

template <typename LogData, 
          size_t MaxNumMessages, 
          size_t MaxMessageLength,
          std::atomic<std::size_t>& SequenceNumber
          >

class Logger
{
public:
    Status Log(const LogData& inputData, const char* format, ...) __attribute__ ((format (printf, 3, 4)))
    {
        auto retVal = Status::Success;

        InternalLogData dataToQueue;
        dataToQueue.mLogData = inputData;
        dataToQueue.mSequenceNumber = ++SequenceNumber;

        va_list args;
        va_start(args, format);
        auto result = stbsp_vsnprintf(dataToQueue.mMessage.data(), dataToQueue.mMessage.size(), format, args);
        va_end(args);

        if (result < 0 || result >= dataToQueue.mMessage.size())
        {
            retVal = Status::Error_MessageTruncated;
        }

        // Even if the message was truncated, we still try to enqueue it to minimize data loss
        const bool dataWasEnqueued = mQueue.try_enqueue(dataToQueue);

        if (!dataWasEnqueued)
        {
            retVal = Status::Error_QueueFull;
        }

        return retVal;
    }

    template<typename PrintLogFn>
    int PrintAndClearLogQueue(PrintLogFn& printLogFn) 
    {
        int numProcessed = 0;

        InternalLogData value;
        while (mQueue.try_dequeue(value)) 
        {
            printLogFn(value.mLogData, value.mSequenceNumber, "%s", value.mMessage.data());
            numProcessed++;
        }

        return numProcessed;
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

template <typename LoggerType, typename PrintLogFn>
class LogProcessingThread
{
public:
    LogProcessingThread(LoggerType& logger, PrintLogFn& printFn, std::chrono::milliseconds waitTime) :
        mPrintFn(printFn),
        mLogger(logger),
        mWaitTime(waitTime)
    {
        mThread = std::thread(&LogProcessingThread::ThreadMain, this);
    }

    ~LogProcessingThread()
    {
        if (mThread.joinable())
        {
            Stop();
            mThread.join();
        }
    }

    LogProcessingThread(const LogProcessingThread&) = delete;
    LogProcessingThread& operator=(const LogProcessingThread&) = delete;
    LogProcessingThread(LogProcessingThread&&) = delete;
    LogProcessingThread& operator=(LogProcessingThread&&) = delete;

    void Stop()
    {
        mShouldRun.store(false);
    }

    void ThreadMain()
    {
        while (mShouldRun.load())
        {

            if (mLogger.PrintAndClearLogQueue(mPrintFn) == 0)
            {
                std::this_thread::sleep_for(mWaitTime);
            }

            std::this_thread::sleep_for(mWaitTime);
        }

        mLogger.PrintAndClearLogQueue(mPrintFn);
    }
private:
    PrintLogFn& mPrintFn;
    LoggerType& mLogger;
    std::thread mThread;
    std::atomic<bool> mShouldRun{ true };
    std::chrono::milliseconds mWaitTime;
};

} // namespace rtlog
