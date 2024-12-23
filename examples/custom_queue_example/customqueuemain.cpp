#include <farbot/fifo.hpp>
#include <rtlog/rtlog.h>

template <typename T> class FarbotMPSCQueueWrapper {
  farbot::fifo<T,
               farbot::fifo_options::concurrency::single,   // Consumer
               farbot::fifo_options::concurrency::multiple, // Producer
               farbot::fifo_options::full_empty_failure_mode::
                   return_false_on_full_or_empty, // consumer_failure_mode
               farbot::fifo_options::full_empty_failure_mode::
                   overwrite_or_return_default> // producer_failure_mode

      mQueue;

public:
  using value_type = T;

  FarbotMPSCQueueWrapper(int capacity) : mQueue(capacity) {}

  bool try_enqueue(T &&item) { return mQueue.push(std::move(item)); }
  bool try_dequeue(T &item) { return mQueue.pop(item); }
};

struct LogData {};

std::atomic<size_t> gSequenceNumber{0};

int main() {
  rtlog::Logger<LogData, 128, 128, gSequenceNumber, FarbotMPSCQueueWrapper>
      logger;
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
