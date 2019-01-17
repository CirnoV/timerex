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
#include <queue>
#include <chrono>
#include "extension.h"
#include <am-thread-utils.h>

#include "timerinfo.h"

 /**
  * @file extension.cpp
  * @brief Implement extension code here.
  */

Extension extension;    /**< Global singleton for extension's main interface */

SMEXT_LINK(&extension);

std::vector<TimerInfo*> sTimerVector;
IdentityToken_t *g_pCoreToken;

static cell_t CreateTimerEx(IPluginContext *pCtx, const cell_t *params)
{
  IPluginFunction *pFunc;
  int flags = params[4];

  pFunc = pCtx->GetFunctionById(params[2]);
  if (!pFunc)
  {
    return pCtx->ThrowNativeError("Invalid function id (%X)", params[2]);
  }

  int interval = static_cast<int>(sp_ctof(params[1]) * 1000);
  auto start = std::chrono::system_clock::now();
  auto time = start + std::chrono::milliseconds(interval);

  TimerInfo *timer = new TimerInfo(pFunc, pCtx, interval, params[3], flags);
  sTimerVector.push_back(timer);

  return 1;
}

const sp_nativeinfo_t MyNatives[] = {
  {"CreateTimerEx", CreateTimerEx},
  {NULL, NULL},
};

void Extension::OnCoreMapEnd() {
  std::vector<TimerInfo*>::iterator it;
  for (it = sTimerVector.begin(); it != sTimerVector.end();) {
    TimerInfo *info = (*it);
    if (info->mFlags & TIMER_FLAG_NO_MAPCHANGE) {
      delete info;
      it = sTimerVector.erase(it);
    }
    else {
      ++it;
    }
  }
}

void ExecFunc(TimerInfo *info) {
  int flags = info->mFlags;
  ResultType result;

  IPluginFunction *pFunc = info->mHook;
  if (!pFunc->IsRunnable()) {
    delete info;
    return;
  }

  cell_t res = static_cast<cell_t>(Pl_Continue);
  pFunc->PushCell(info->mUserData);
  pFunc->Execute(&res);
  result = static_cast<ResultType>(res);

  if (flags & TIMER_REPEAT && result == Pl_Continue) {
    auto start = std::chrono::system_clock::now();
    auto time = start + std::chrono::milliseconds(info->mInterval);
    info->mTime = time;
    sTimerVector.push_back(info);
  }
  else {
    delete info;
  }
}

void RunTimer(bool simulating) {
  static size_t cacheSize = 0;
  static std::chrono::system_clock::time_point cacheTime = std::chrono::system_clock::now();

  auto now = std::chrono::system_clock::now();
  if (!sTimerVector.empty()) {
    auto vectorSize = sTimerVector.size();

    if (cacheSize != vectorSize || cacheTime <= now) {
      cacheTime = (std::chrono::system_clock::time_point::max)();

      std::vector<TimerInfo*>::iterator it;
      for (it = sTimerVector.begin(); it != sTimerVector.end();) {
        TimerInfo *info = (*it);
        auto time = info->mTime;
        if (time <= now) {
          ExecFunc(info);
          it = sTimerVector.erase(it);
        }
        else {
          if (time <= cacheTime) {
            cacheTime = time;
          }
          ++it;
        }
      }
      cacheSize = sTimerVector.size();
    }
  }
}

bool Extension::SDK_OnLoad(char *error, size_t maxlen, bool late) {
  g_pCoreToken = sharesys->CreateIdentity(sharesys->FindIdentType("CORE"), this);
  sTimerVector.reserve(1000);

  smutils->AddGameFrameHook(RunTimer);

  return true;
}

void Extension::SDK_OnUnload() {
  smutils->RemoveGameFrameHook(RunTimer);

  std::vector<TimerInfo*>::iterator it;
  for (it = sTimerVector.begin(); it != sTimerVector.end();) {
    TimerInfo *info = (*it);
    delete info;
    it = sTimerVector.erase(it);
  }
}

void Extension::SDK_OnAllLoaded() {
  sharesys->AddNatives(myself, MyNatives);
}
