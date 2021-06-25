#ifndef _INCLUDE_TIMEREX_SOURCEMOD_EXTENSION_
#define _INCLUDE_TIMEREX_SOURCEMOD_EXTENSION_

#include "smsdk_ext.h"

extern IdentityToken_t *g_pCoreToken;

#define TIMER_REPEAT (1 << 0)            /**< Timer will repeat until it returns Plugin_Stop */
#define TIMER_FLAG_NO_MAPCHANGE (1 << 1) /**< Timer will not carry over mapchanges */
#define TIMER_DATA_HNDL_CLOSE (1 << 9)   /**< Timer will automatically call CloseHandle() on its data when finished */

class TimerExtension : public SDKExtension,
                       public IPluginsListener
{
public:
    virtual void OnCoreMapEnd();

public: // SDKExtension
    virtual bool SDK_OnLoad(char *error, size_t maxlen, bool late);
    virtual void SDK_OnUnload();
    virtual void SDK_OnAllLoaded();

public: //IPluginsListener
    void OnPluginLoaded(IPlugin *plugin);
    void OnPluginUnloaded(IPlugin *plugin);
    void OnPluginWillUnload(IPlugin *plugin);
};

#endif // _INCLUDE_TIMEREX_SOURCEMOD_EXTENSION_
