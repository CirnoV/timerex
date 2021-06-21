#include <cstdarg>
#include <cstdint>
#include <cstdlib>
#include <ostream>
#include <new>

template<typename T = void>
struct Vec;

struct TimerInfo {
  const void *hook;
  const void *context;
};

extern "C" {

void create_timer(const void *hook,
                  const void *context,
                  uint32_t interval,
                  int32_t user_data,
                  int32_t flags,
                  int32_t channel);

Vec<TimerInfo> update_timer();

} // extern "C"
