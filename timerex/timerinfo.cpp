#include "timerinfo.h"
#include "extension.h"

TimerInfo::TimerInfo(IPluginFunction *hook, IPluginContext *context, int interval, int userData, int flags)
    : mHook(hook), mContext(context), mInterval(interval), mUserData(userData), mFlags(flags)
{
  auto start = std::chrono::system_clock::now();
  auto time = start + std::chrono::milliseconds(interval);
  mTime = time;
}

TimerInfo::~TimerInfo()
{
  if (mFlags & TIMER_DATA_HNDL_CLOSE)
  {
    HandleSecurity sec;
    Handle_t usrhndl = static_cast<Handle_t>(mUserData);

    sec.pOwner = mContext->GetIdentity();
    sec.pIdentity = g_pCoreToken;

    HandleError herr = handlesys->FreeHandle(usrhndl, &sec);
    if (herr != HandleError_None)
    {
      smutils->LogError(myself, "Invalid data handle %x (error %d) passed during timer end with TIMER_DATA_HNDL_CLOSE", usrhndl, herr);
    }
  }
}
