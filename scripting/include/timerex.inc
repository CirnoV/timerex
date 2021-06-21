#if defined _timerex_included
  #endinput
#endif
#define _timerex_included

#define TIMER_EX_CHANNEL_WORLD 0

typeset TimerEx
{
  function Action(Handle hndl);

  function Action(any data);

  function Action();
}

native void CreateTimerEx(float interval, TimerEx func, any data=INVALID_HANDLE, int flags=0, int channel = TIMER_CHANNEL_WORLD);

stock void CreateDataTimerEx(float interval, TimerEx func, Handle &datapack, int flags=0)
{
	datapack = new DataPack();
	flags |= TIMER_DATA_HNDL_CLOSE;
	CreateTimerEx(interval, func, datapack, flags);
}

native void PauseTimer(int[] channels, int length);

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