#include "extension.h"
#include "bindings.h"

TimerExtension extension;

SMEXT_LINK(&extension);

IdentityToken_t *g_pCoreToken;

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
    for (unsigned int i = 0; i < arr->n; i = i + 1)
    {
        free_timer_data_handle(&arr->arr[i]);
    }
}

static cell_t CreateTimerEx(IPluginContext *pCtx, const cell_t *params)
{
    cell_t userData = params[3];
    cell_t flags = params[4];
    cell_t channel = params[5];

    IPluginFunction *pFunc = pCtx->GetFunctionById(params[2]);
    if (!pFunc)
    {
        return pCtx->ThrowNativeError("Invalid function id (%X)", params[2]);
    }

    cell_t interval = static_cast<int>(sp_ctof(params[1]) * 1000);
    create_timer(pFunc, pCtx, pCtx->GetIdentity(), interval, userData, flags, channel);

    return 1;
}

static cell_t PauseTimer(IPluginContext *pCtx, const cell_t *params)
{
    cell_t *array = NULL;
    cell_t length = params[2];
    if (length < 1)
    {
        return 0;
    }
    pCtx->LocalToPhysAddr(params[1], &array);
    pause_timer(array, length);

    return 1;
}

static cell_t PauseTimerChannel(IPluginContext *pCtx, const cell_t *params)
{
    cell_t channel = params[1];
    pause_channel(channel);

    return 1;
}

static cell_t ResumeTimer(IPluginContext *pCtx, const cell_t *params)
{
    cell_t *array = NULL;
    cell_t length = params[2];
    if (length < 1)
    {
        return 0;
    }
    pCtx->LocalToPhysAddr(params[1], &array);
    resume_timer(array, length);

    return 1;
}

static cell_t ResumeTimerChannel(IPluginContext *pCtx, const cell_t *params)
{
    cell_t channel = params[1];
    resume_channel(channel);

    return 1;
}

static cell_t ResumeTimerAll(IPluginContext *pCtx, const cell_t *params)
{
    resume_timer_all();

    return 1;
}

static cell_t RemoveTimerChannel(IPluginContext *pCtx, const cell_t *params)
{
    cell_t channel = params[1];
    timer_arr timers = remove_channel(channel);
    kill_timer_arr(&timers);
    drop_timer_arr(&timers);

    return 1;
}

const sp_nativeinfo_t MyNatives[] = {
    {"CreateTimerEx", CreateTimerEx},
    {"PauseTimer", PauseTimer},
    {"PauseTimerChannel", PauseTimerChannel},
    {"ResumeTimer", ResumeTimer},
    {"ResumeTimerChannel", ResumeTimerChannel},
    {"ResumeTimerAll", ResumeTimerAll},
    {"RemoveTimerChannel", RemoveTimerChannel},
    {NULL, NULL},
};

void TimerExtension::OnCoreMapEnd()
{
    timer_arr arr = timer_mapchange();
    kill_timer_arr(&arr);
    drop_timer_arr(&arr);
}

void ExecFunc(TimerInfo *info)
{
    IPluginFunction *pFunc = (IPluginFunction *)info->hook;
    if (!pFunc || !pFunc->IsRunnable())
    {
        free_timer_data_handle(info);
        return;
    }

    cell_t res = static_cast<cell_t>(Pl_Continue);
    pFunc->PushCell(info->user_data);
    pFunc->Execute(&res);
    ResultType result = static_cast<ResultType>(res);

    if (info->flags & TIMER_REPEAT && result == Pl_Continue)
    {
        create_timer(info->hook, info->context, info->identity, info->interval, info->user_data, info->flags, info->channel);
    }
    else
    {
        free_timer_data_handle(info);
    }
}

void RunTimer(bool simulating)
{
    timer_arr timers = update_timer();
    for (unsigned int i = 0; i < timers.n; i = i + 1)
    {
        TimerInfo timerinfo = timers.arr[i];
        ExecFunc(&timerinfo);
    }
    drop_timer_arr(&timers);
}

bool TimerExtension::SDK_OnLoad(char *error, size_t maxlen, bool late)
{
    g_pCoreToken = sharesys->CreateIdentity(sharesys->FindIdentType("CORE"), this);

    return true;
}

void TimerExtension::SDK_OnUnload()
{
    smutils->RemoveGameFrameHook(RunTimer);

    timer_arr timers = clear_timer();
    kill_timer_arr(&timers);
    drop_timer_arr(&timers);
}

void TimerExtension::SDK_OnAllLoaded()
{
    plsys->AddPluginsListener(this);
    smutils->AddGameFrameHook(RunTimer);
    sharesys->AddNatives(myself, MyNatives);
}

void ResetTimer(SourceMod::IdentityToken_t *identity)
{
    timer_arr timers = timer_pluginload(identity);
    kill_timer_arr(&timers);
    drop_timer_arr(&timers);
}

void TimerExtension::OnPluginLoaded(IPlugin *plugin)
{
    ResetTimer(plugin->GetBaseContext()->GetIdentity());
}

void TimerExtension::OnPluginUnloaded(IPlugin *plugin)
{
    ResetTimer(plugin->GetBaseContext()->GetIdentity());
}

void TimerExtension::OnPluginWillUnload(IPlugin *plugin)
{
    ResetTimer(plugin->GetBaseContext()->GetIdentity());
}
