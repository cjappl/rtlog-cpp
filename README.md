# rtlog-cpp 🔊

`rtlog-cpp` is a logging library designed specifically for logging messages from the real-time thread. This is particularly useful in the audio and embedded industries, where hard real-time requirements must be met, and logging in traditional ways from your real-time threads is unacceptable.

If you're looking for a general use logger, this probably isn't the library for you!

The design behind this logger was presented at ADCx 2023. Presentation [video](https://www.youtube.com/watch?v=4KFFMGTQIFM) and [slides](https://github.com/cjappl/Conference-Presentations/tree/main/Taming-Real-Time-Logging-ADCx-2023).

## Features

- Ability to log messages of any type and size from the real-time thread
- Statically allocated memory at compile time, no allocations in the real-time thread
- Support for printf-style format specifiers (using [a version of the printf family](https://github.com/nothings/stb/blob/master/stb_sprintf.h) that doesn't hit the `localeconv` lock)
- Efficient thread-safe logging using a [lock free queue](https://github.com/cameron314/readerwriterqueue).

## Requirements

- A C++17 compatible compiler
- The C++17 standard library
- moodycamel::ReaderWriterQueue (will be downloaded via cmake if not provided)
- stb's vsnprintf (will be downloaded via cmake if not provided)

## Installation via CMake

In CMakeLists.txt
```cmake
include(FetchContent)
FetchContent_Declare(rtlog-cpp
    GIT_REPOSITORY https://github.com/cjappl/rtlog-cpp
)
FetchContent_MakeAvailable(rtlog-cpp)

add_executable(audioapp ${SOURCES})
target_link_libraries(audioapp
     PRIVATE
         rtlog::rtlog
 )
```

To use formatlib, set the variable, either on the command line or in cmake:
```bash
cmake .. -DRTLOG_USE_FMTLIB=ON
```

## Usage

For more fleshed out fully running examples check out `examples/` and `test/`

After including via cmake:

1. Include the `rtlog/rtlog.h` header file in your source code
2. Create a `rtlog::Logger` object with the desired template parameters:
3. Process the log messages on your own thread, or via the provided `rtlog::LogProcessingThread`

```c++
#include <rtlog/rtlog.h>

struct ExampleLogData
{
    ExampleLogLevel level;
    ExampleLogRegion region;
};

constexpr auto MAX_LOG_MESSAGE_LENGTH = 256;
constexpr auto MAX_NUM_LOG_MESSAGES = 100;

std::atomic<int> gSequenceNumber{0};

using RealtimeLogger = rtlog::Logger<ExampleLogData, MAX_NUM_LOG_MESSAGES, MAX_LOG_MESSAGE_LENGTH, gSequenceNumber>;

...

RealtimeLogger logger;

void SomeRealtimeCallback()
{
    logger.Log({ExampleLogLevel::Debug, ExampleLogRegion::Audio}, "Hello, world! %i", 42);
    logger.LogFmt({ExampleLogData::Debug, ExampleLogRegion::Audio, FMT_STRING("Hello, world! {}", 42);
}

...

```

To process the logs in another thread, call `PrintAndClearLogQueue` with a function to call on the output data.

```c++

static auto PrintMessage = [](const ExampleLogData& data, size_t sequenceNumber, const char* fstring, ...) __attribute__ ((format (printf, 4, 5)))
{
    std::array<char, MAX_LOG_MESSAGE_LENGTH> buffer;
    
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

...

void LogProcessorThreadMain()
{
    while (running)
    {
        if (logger.PrintAndClearLogQueue(PrintMessage) == 0)
            std::this_thread::sleep_for(std::chrono::milliseconds(10);
    }
}

```

Or alternatively spin up a `rtlog::LogProcessingThread`

```c++
    rtlog::LogProcessingThread thread(logger, PrintMessage, std::chrono::milliseconds(10));
```
