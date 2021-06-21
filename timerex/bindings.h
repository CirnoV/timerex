#include <cstdarg>
#include <cstdint>
#include <cstdlib>
#include <ostream>
#include <new>

struct TimerInfo {
  void *hook;
  void *context;
  int32_t user_data;
  int32_t flags;
};

struct timer_arr {
  TimerInfo *arr;
  uintptr_t n;
  uintptr_t cap;
};

extern "C" {

void create_timer(void *hook,
                  void *context,
                  void *identity,
                  uint32_t interval,
                  int32_t user_data,
                  int32_t flags,
                  int32_t channel);

void drop_timer_arr(timer_arr *arr);

timer_arr update_timer();

void pause_timer(int32_t *channels, size_t len);

void resume_timer(int32_t *channels, size_t len);

timer_arr remove_channel(int32_t channel);

timer_arr clear_timer();

timer_arr timer_mapchange();

timer_arr timer_pluginload(void *identity);

} // extern "C"
