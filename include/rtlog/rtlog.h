#pragma once

// TODO: Make a log processor thread 
// TODO: Use format lib and variadic templates
// TODO: Build an example project executable
// TODO: Documentation 
// TODO: Lower camel case?

#include <array>
#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <thread>

#include <readerwriterqueue.h>
#include <stb_sprintf.h>

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
        stbsp_vsnprintf(dataToQueue.mMessage.data(), dataToQueue.mMessage.size(), format, args);
        va_end(args);

        bool result = mQueue.try_enqueue(dataToQueue);
        assert(result); // If you're hitting this often, your queue is too small
    }

    template<typename PrintLogFn>
    int PrintAndClearLogQueue(PrintLogFn printLogFn) 
    {
        int numProcessed = 0;

        InternalLogData value;
        while (mQueue.try_dequeue(value)) 
        {
            printLogFn(value.mLogData, value.mSequenceNumber, value.mMessage.data());
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

// has a wait time, flushes the queue when done
template <typename LoggerType, typename PrintLogFn>
class ScopedLogThread
{
public:
    ScopedLogThread(LoggerType& logger, PrintLogFn& printFn, std::chrono::milliseconds waitTime) :
        mPrintFn(printFn),
        mLogger(logger),
        mWaitTime(waitTime)
    {
        mThread = std::thread(&ScopedLogThread::ThreadMain, this);
    }

    ~ScopedLogThread()
    {
        if (mThread.joinable())
        {
            Stop();
        }

        mThread.join();
    }

    ScopedLogThread(const ScopedLogThread&) = delete;
    ScopedLogThread& operator=(const ScopedLogThread&) = delete;
    ScopedLogThread(ScopedLogThread&&) = delete;
    ScopedLogThread& operator=(ScopedLogThread&&) = delete;

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
