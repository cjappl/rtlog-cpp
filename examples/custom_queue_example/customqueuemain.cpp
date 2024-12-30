#include <rtlog/rtlog.h>

#include <readerwriterqueue.h>

template <typename T> class CustomQueue {

  // technically we could use readerwriterqueue "unwrapped" but showing this off
  // in the CustomQueue wrapper for documentation purposes
  moodycamel::ReaderWriterQueue<T> mQueue;

public:
  using value_type = T;

  CustomQueue(int capacity) : mQueue(capacity) {}

  bool try_enqueue(T &&item) { return mQueue.try_enqueue(std::move(item)); }
  bool try_dequeue(T &item) { return mQueue.try_dequeue(item); }
};

struct LogData {};

std::atomic<size_t> gSequenceNumber{0};

int main() {
  rtlog::Logger<LogData, 128, 128, gSequenceNumber, CustomQueue> logger;
  logger.Log({}, "Hello, World!");

  logger.PrintAndClearLogQueue(
      [](const LogData &data, size_t sequenceNumber, const char *fstring, ...) {
        (void)data;
        std::array<char, 128> buffer;

        va_list args;
        va_start(args, fstring);
        vsnprintf(buffer.data(), buffer.size(), fstring, args);
        va_end(args);

        printf("{%zu} %s\n", sequenceNumber, buffer.data());
      });

  return 0;
}
