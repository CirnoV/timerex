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

#include <vector>
#include <chrono>
#include "extension.h"

 /**
  * @file extension.cpp
  * @brief Implement extension code here.
  */

Extension extension;    /**< Global singleton for extension's main interface */

SMEXT_LINK(&extension);

struct TimerInfo
{
  IPluginFunction *Hook;
  IPluginContext *pContext;
  std::chrono::system_clock::time_point time;
  int interval;
  int UserData;
  int Flags;
};

std::vector<TimerInfo> sTimerVector;
IdentityToken_t *g_pCoreIdent;

void PushTimer(IPluginFunction *Hook, IPluginContext *pContext, std::chrono::system_clock::time_point time, int interval, int UserData, int Flags) {
  TimerInfo Info;
  Info.Hook = Hook;
  Info.pContext = pContext;
  Info.time = time;
  Info.interval = interval;
  Info.UserData = UserData;
  Info.Flags = Flags;

  sTimerVector.push_back(Info);
}

std::vector<TimerInfo>::iterator EraseTimer(std::vector<TimerInfo>::iterator it) {
  int flags = it->Flags;

  if (flags & TIMER_DATA_HNDL_CLOSE) {
    HandleSecurity sec;
    HandleError herr;
    Handle_t usrhndl = static_cast<Handle_t>(it->UserData);

    sec.pOwner = it->pContext->GetIdentity();
    sec.pIdentity = g_pCoreIdent;

    herr = handlesys->FreeHandle(usrhndl, &sec);
    if (herr != HandleError_None) {
      smutils->LogError(myself, "Invalid data handle %x (error %d) passed during timer end with TIMER_DATA_HNDL_CLOSE", usrhndl, herr);
    }
  }

  return sTimerVector.erase(it);
}

std::vector<TimerInfo>::iterator OnTimer(std::vector<TimerInfo>::iterator it) {
  int flags = it->Flags;
  ResultType result;

  IPluginFunction *pFunc = it->Hook;
  if (!pFunc->IsRunnable()) {
    return it;
  }

  cell_t res = static_cast<cell_t>(Pl_Continue);
  pFunc->PushCell(it->UserData);
  pFunc->Execute(&res);
  result = static_cast<ResultType>(res);

  if (flags & TIMER_REPEAT && result == Pl_Continue) {
    auto start = std::chrono::system_clock::now();
    auto time = start + std::chrono::milliseconds(it->interval);
    it->time = time;

    return it;
  }
  return EraseTimer(it);
}

static cell_t CreateTimerEx(IPluginContext *pCtx, const cell_t *params)
{
  IPluginFunction *pFunc;
  TimerInfo Info;
  int flags = params[4];

  pFunc = pCtx->GetFunctionById(params[2]);
  if (!pFunc)
  {
    return pCtx->ThrowNativeError("Invalid function id (%X)", params[2]);
  }

  int interval = static_cast<int>(sp_ctof(params[1]) * 1000);
  auto start = std::chrono::system_clock::now();
  auto time = start + std::chrono::milliseconds(interval);

  PushTimer(pFunc, pCtx, time, interval, params[3], flags);

  return 1;
}

const sp_nativeinfo_t MyNatives[] = {
  {"CreateTimerEx", CreateTimerEx},
  {NULL, NULL},
};

void Extension::OnCoreMapEnd() {
  std::vector<TimerInfo>::iterator it;
  for (it = sTimerVector.begin(); it != sTimerVector.end();) {
    if (it->Flags & TIMER_FLAG_NO_MAPCHANGE) {
      it = EraseTimer(it);
    }
    else {
      ++it;
    }
  }
}

void RunTimer(bool simulating) {
  if (!sTimerVector.empty()) {
    auto now = std::chrono::system_clock::now();
    std::vector<TimerInfo>::iterator it;
    for (it = sTimerVector.begin(); it != sTimerVector.end();) {
      if (it->time <= now) {
        it = OnTimer(it);
      }
      else {
        ++it;
      }
    }
  }
}

bool Extension::SDK_OnLoad(char *error, size_t maxlen, bool late) {
  g_pCoreIdent = sharesys->CreateIdentity(sharesys->FindIdentType("CORE"), this);
  sTimerVector.reserve(1000);

  smutils->AddGameFrameHook(RunTimer);

  return true;
}

void Extension::SDK_OnUnload() {

}

void Extension::SDK_OnAllLoaded() {
  sharesys->AddNatives(myself, MyNatives);
}
