#if defined _timerex_included
  #endinput
#endif
#define _timerex_included

#define TIMER_CHANNEL_WORLD 0

typeset TimerEx
{
  function Action(Handle hndl);

  function Action(any data);

  function Action();

  function void();

  function void(any data);

  function void(Handle hndl);
}

native void CreateTimerEx(float interval, TimerEx func, any data = INVALID_HANDLE, int flags = 0, int channel = TIMER_CHANNEL_WORLD);

stock void CreateDataTimerEx(float interval, TimerEx func, Handle &datapack, int flags = 0, int channel = TIMER_CHANNEL_WORLD)
{
	datapack = new DataPack();
	flags |= TIMER_DATA_HNDL_CLOSE;
	CreateTimerEx(interval, func, datapack, flags, channel);
}

native void PauseTimer(int[] channels, int length);

native void PauseTimerChannel(int channel);

native void ResumeTimer(int[] channels, int length);

native void ResumeTimerAll();

native void ResumeTimerChannel(int channel);

native void RemoveTimerChannel(int channel);

public Extension __ext_timerex =
{
	name = "timerex",
	file = "timerex.ext",
#if defined AUTOLOAD_EXTENSIONS
	autoload = 1,
#else
	autoload = 0,
#endif
#if defined REQUIRE_EXTENSIONS
	required = 1,
#else
	required = 0,
#endif
};
