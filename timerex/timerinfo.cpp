#include "timerinfo.h"
#include "extension.h"

void free_timer_data_handle(TimerInfo *info)
{
  if (info->flags & TIMER_DATA_HNDL_CLOSE)
  {
    HandleSecurity sec;
    Handle_t usrhndl = static_cast<Handle_t>(info->user_data);

    sec.pOwner = ((IPluginContext *)info->context)->GetIdentity();
    sec.pIdentity = g_pCoreToken;

    HandleError herr = handlesys->FreeHandle(usrhndl, &sec);
    if (herr != HandleError_None)
    {
      smutils->LogError(myself, "Invalid data handle %x (error %d) passed during timer end with TIMER_DATA_HNDL_CLOSE", usrhndl, herr);
    }
  }
}

void kill_timer_arr(timer_arr *arr)
{
  for (int i = 0; i < arr->n; i = i + 1)
  {
    free_timer_data_handle(&arr->arr[i]);
  }
}
