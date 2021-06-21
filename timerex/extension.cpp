/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Sample Extension
 * Copyright (C) 2004-2008 AlliedModders LLC.  All rights reserved.
 * =============================================================================
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 3.0, as published by the
 * Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * As a special exception, AlliedModders LLC gives you permission to link the
 * code of this program (as well as its derivative works) to "Half-Life 2," the
 * "Source Engine," the "SourcePawn JIT," and any Game MODs that run on software
 * by the Valve Corporation.  You must obey the GNU General Public License in
 * all respects for all other code used.  Additionally, AlliedModders LLC grants
 * this exception to all derivative works.  AlliedModders LLC defines further
 * exceptions, found in LICENSE.txt (as of this writing, version JULY-31-2007),
 * or <http://www.sourcemod.net/license.php>.
 *
 * Version: $Id$
 */

#include "extension.h"
#include <am-thread-utils.h>

#include "timerinfo.h"

/**
  * @file extension.cpp
  * @brief Implement extension code here.
  */

Extension extension; /**< Global singleton for extension's main interface */

SMEXT_LINK(&extension);

IdentityToken_t *g_pCoreToken;

static cell_t CreateTimerEx(IPluginContext *pCtx, const cell_t *params)
{
  int userData = params[3];
  int flags = params[4];
  int channel = params[5];

  IPluginFunction *pFunc = pCtx->GetFunctionById(params[2]);
  if (!pFunc)
  {
    return pCtx->ThrowNativeError("Invalid function id (%X)", params[2]);
  }

  int interval = static_cast<int>(sp_ctof(params[1]) * 1000);
  create_timer(pFunc, pCtx, pCtx->GetIdentity(), interval, userData, flags, channel);

  return 1;
}

const sp_nativeinfo_t MyNatives[] = {
    {"CreateTimerEx", CreateTimerEx},
    {NULL, NULL},
};

void Extension::OnCoreMapEnd()
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

  free_timer_data_handle(info);
}

void RunTimer(bool simulating)
{
  timer_arr timers = update_timer();
  for (int i = 0; i < timers.n; i = i + 1)
  {
    TimerInfo timerinfo = timers.arr[i];
    ExecFunc(&timerinfo);
  }
  drop_timer_arr(&timers);
}

bool Extension::SDK_OnLoad(char *error, size_t maxlen, bool late)
{
  g_pCoreToken = sharesys->CreateIdentity(sharesys->FindIdentType("CORE"), this);

  return true;
}

void Extension::SDK_OnUnload()
{
  smutils->RemoveGameFrameHook(RunTimer);

  timer_arr timers = clear_timer();
  kill_timer_arr(&timers);
  drop_timer_arr(&timers);
}

void Extension::SDK_OnAllLoaded()
{
  plsys->AddPluginsListener(this);
  smutils->AddGameFrameHook(RunTimer);
  sharesys->AddNatives(myself, MyNatives);
}

void ResetTimer(SourceMod::IdentityToken_t *identity)
{
  timer_arr timers = clear_timer();
  kill_timer_arr(&timers);
  std::vector<TimerInfo *>::iterator it;
  for (it = sTimerVector.begin(); it != sTimerVector.end();)
  {
    TimerInfo *info = (*it);
    if (info->mContext->GetIdentity() == identity)
    {
      delete info;
      it = sTimerVector.erase(it);
    }
    else
    {
      ++it;
    }
  }
  drop_timer_arr(&timers);
}

void Extension::OnPluginLoaded(IPlugin *plugin)
{
  ResetTimer(plugin->GetBaseContext()->GetIdentity());
}

void Extension::OnPluginUnloaded(IPlugin *plugin)
{
  ResetTimer(plugin->GetBaseContext()->GetIdentity());
}

void Extension::OnPluginWillUnload(IPlugin *plugin)
{
  ResetTimer(plugin->GetBaseContext()->GetIdentity());
}
