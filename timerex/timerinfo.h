#pragma once

#include <chrono>
#include "extension.h"

class TimerInfo
{
public:
  IPluginFunction* mHook;
  IPluginContext* mContext;
  std::chrono::system_clock::time_point mTime;
  int mInterval;
  int mUserData;
  int mFlags;

	TimerInfo(IPluginFunction* hook, IPluginContext* context, int interval, int userData, int flags);
	~TimerInfo();
};

