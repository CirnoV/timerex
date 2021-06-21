#include <cstdarg>
#include <cstdint>
#include <cstdlib>
#include <ostream>
#include <new>

struct TimerInfo {
  const void *hook;
  const void *context;
  int32_t user_data;
  int32_t flags;
};

struct s_arr {
  TimerInfo *arr;
  uintptr_t n;
  uintptr_t cap;
};

extern "C" {

void create_timer(const void *hook,
                  const void *context,
                  uint32_t interval,
                  int32_t user_data,
                  int32_t flags,
                  int32_t channel);

s_arr update_timer();

void stop_timer(int32_t *channels, size_t len);

void stop_channel(int32_t channel);

} // extern "C"
